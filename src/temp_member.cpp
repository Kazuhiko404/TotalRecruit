#include "common.h"
#include "temp_member.h"
#include "temp_member_rules.h"
#include "chance_rules.h"
#include "char_key_reader.h"
#include "config.h"
#include "engine_reads.h"
#include "head_notice.h"
#include "leave_homing.h"
#include "leave_homing_rules.h"
#include "log.h"
#include "messages.h"
#include "npc_motion.h"
#include "recruit_core.h"
#include "squad_leave.h"
#include "submission_rules.h"
#include "cooldown_minutes.h"
#include "submission_tracker.h"

#include <cstdio>
#include <map>

namespace totalrecruit
{
namespace
{
// The watch runs this often (real milliseconds): the signals change over game
// hours, and each member costs a few guarded reads.
const unsigned int kTickIntervalMs = 2000;
// The most squad members looked at per tick.
const int kMaxSquad = 256;

// ------------------------------------------------- The locked table
TempMemberBook g_book;
// The faction each member goes back to, by the same key.
std::map<CharKey, std::string> g_homeFaction;
// Where each member stood when they joined, "x,z" in world units (leave_homing_rules.h).
std::map<CharKey, std::string> g_joinSpot;

struct BookMutex
{
    CRITICAL_SECTION value;
    BookMutex() { InitializeCriticalSection(&value); }
    ~BookMutex() { DeleteCriticalSection(&value); }
} g_mutex;

struct Guard
{
    Guard() { EnterCriticalSection(&g_mutex.value); }
    ~Guard() { LeaveCriticalSection(&g_mutex.value); }
private:
    Guard(const Guard&);
    Guard& operator=(const Guard&);
};

DWORD g_nextTick = 0;

bool FindMember(const CharKey& key, TempMembership* out)
{
    Guard lock;
    const TempMembership* m = g_book.Find(key);
    if (m == NULL)
        return false;
    *out = *m;
    return true;
}

void UpdateMember(const CharKey& key, const TempMembership& m)
{
    Guard lock;
    g_book.Update(key, m);
}

void RemoveMember(const CharKey& key)
{
    Guard lock;
    g_book.Remove(key);
    g_homeFaction.erase(key);
    g_joinSpot.erase(key);
}

std::string HomeFactionOf(const CharKey& key)
{
    Guard lock;
    std::map<CharKey, std::string>::const_iterator it = g_homeFaction.find(key);
    return it != g_homeFaction.end() ? it->second : std::string();
}

// The saved join spot, false when none was kept (joined before this was saved).
bool JoinSpotOf(const CharKey& key, int* x, int* z)
{
    std::string text;
    {
        Guard lock;
        std::map<CharKey, std::string>::const_iterator it = g_joinSpot.find(key);
        if (it == g_joinSpot.end())
            return false;
        text = it->second;
    }
    return ParseJoinSpot(text, x, z);
}

bool HaveMembers()
{
    Guard lock;
    return g_book.Count() > 0;
}

// ------------------------------------------------------- What they say
std::string WithDays(const std::string& text, int days)
{
    char number[16] = { 0 };
    _snprintf_s(number, sizeof(number), _TRUNCATE, "%d", days);
    std::string out(text);
    const std::string token("{days}");
    std::string::size_type at;
    while ((at = out.find(token)) != std::string::npos)
        out.replace(at, token.size(), number);
    return out;
}

// The notice for this step, empty when there is nothing to say. The warning says
// more than the strike that brought it, and both more than "soon".
std::string StepNotice(Character* member, const TempMemberStep& step)
{
    if (step.warn)
        return Say("tempMember.notice.warning", "Si sigues asi me voy", member);
    if (step.Struck())
    {
        if (step.struckMask == (1 << TempStrike_Hunger))
            return Say("tempMember.notice.hungry", "Tengo hambre...", member);
        return Say("tempMember.notice.mistreated", "No me trates asi", member);
    }
    if (step.expiryWarn)
        return Say("tempMember.notice.expiring", "Me ire pronto", member);
    // Once per game day (temp_member_rules.h ShouldAnnounceDaysLeft): one line for the
    // last day, another for the rest.
    if (step.dailyNotice)
    {
        if (step.dailyDays == 1)
            return Say("tempMember.notice.dailyOne", "Me queda 1 dia", member);
        return WithDays(Say("tempMember.notice.daily", "Me quedan {days} dias", member), step.dailyDays);
    }
    return std::string();
}

void LogStep(Character* member, const TempMembership& m, const TempMemberStep& step)
{
    if (step.dailyNotice)
    {
        char daily[96] = { 0 };
        _snprintf_s(daily, sizeof(daily), _TRUNCATE, " [temp] daily notice: %d day(s) left (day %d)",
                    step.dailyDays, m.lastDailyDay);
        LogInfo(CharacterName(member) + daily);
    }
    if (!step.Struck() && !step.warn && !step.expiryWarn)
        return;
    char note[160] = { 0 };
    _snprintf_s(note, sizeof(note), _TRUNCATE,
                " [temp] strikes %d (hunger %d, attacked %d, knocked out %d)%s%s",
                m.strikes, step.StruckBy(TempStrike_Hunger) ? 1 : 0,
                step.StruckBy(TempStrike_Attacked) ? 1 : 0,
                step.StruckBy(TempStrike_KnockedOut) ? 1 : 0, step.warn ? " warned" : "",
                step.expiryWarn ? " expiry soon" : "");
    LogInfo(CharacterName(member) + note);
}

// ---------------------------------------------------------- The signals
TempMemberSignals ReadSignals(Character* member, double nowHours, const TempMembers& config)
{
    TempMemberSignals s;
    s.nowHours = nowHours;

    float rawHunger = 0.0f;
    bool starving = false;
    if (SafeHunger(member, &rawHunger, &starving))
        s.hungry = IsTempMemberHungry(HungerToFullness01(rawHunger), starving, config.hungerThreshold);

    SafeAttackedByPlayer(member, &s.attackedBySquad);

    bool down = false;
    if (SafeIsKnockedOut(member, &down) && down)
    {
        s.knockedOut = true;
        bool byPlayer = false;
        // A knockout from hunger is not the squad's doing, whoever beat them last.
        s.knockedOutBySquad = SafeDefeatedByPlayer(member, &byPlayer) && byPlayer && !starving;
    }
    return s;
}

// ------------------------------------------------------------ The leave
void Leave(Character* member, const CharKey& key, TempLeave why, double nowHours)
{
    const std::string name = CharacterName(member);
    const bool mistreated = why == TempLeave_Mistreated;
    int joinX = 0, joinZ = 0;
    const bool haveSpot = JoinSpotOf(key, &joinX, &joinZ);
    bool wander = false;
    if (!LeaveSquad(member, HomeFactionOf(key), &wander))
    {
        // They stay as an ordinary member rather than being asked again every tick.
        RemoveMember(key);
        LogError("[temp] " + name + " could not leave the squad; they stay as an ordinary member.");
        return;
    }

    RaiseHeadNotice(member, mistreated
                                ? Say("tempMember.notice.farewellMistreated", "Me largo", member)
                                : Say("tempMember.notice.farewell", "Me voy. Fue un honor", member),
                    HeadNoticeKind_Info);
    TellPlayer(Fill(mistreated ? Say("tempMember.leftMistreated",
                                     "{name} se harto del maltrato y se fue de tu grupo.", member)
                               : Say("tempMember.left",
                                     "{name} cumplio su tiempo y se fue de tu grupo.", member),
                    name, 0, 0),
               false);

    // They will not be taken on again straight away (the reject cooldown refuses
    // every route while it runs).
    const int days = GetConfig().tempMembers.rejoinCooldownDays;
    if (days > 0)
        SetRejectCooldownUntil(key, CooldownUntilMinute(nowHours, (float)(days * 24)));

    RemoveMember(key);
    LogInfo("[temp] " + name + (mistreated ? " left: mistreated." : " left: time up."));

    // No wander package: they walk back to where they joined instead of standing.
    if (!wander)
    {
        if (haveSpot)
            StartLeaveHoming(member, joinX, joinZ);
        else
            LogError("[temp] " + name + " has no wander package and no saved join spot; they stay put.");
    }
}

void WatchMember(Character* member, const CharKey& key, double nowHours, const TempMembers& config)
{
    bool dead = false;
    if (SafeIsDead(member, &dead) && dead)
    {
        RemoveMember(key);
        return;
    }

    TempMembership m;
    if (!FindMember(key, &m))
        return;
    const TempMemberStep step = StepTempMember(&m, ReadSignals(member, nowHours, config), config);
    UpdateMember(key, m);
    LogStep(member, m, step);

    if (step.leave != TempLeave_Stay)
    {
        Leave(member, key, step.leave, nowHours);
        return;
    }
    const std::string notice = StepNotice(member, step);
    if (!notice.empty())
        RaiseSquadHeadNotice(member, notice, HeadNoticeKind_Info);
}
} // namespace

void OnRecruitedTemporarily(Character* npc, int days, const std::string& homeFactionId)
{
    if (npc == NULL)
        return;
    const std::string name = CharacterName(npc);
    CharKey key;
    double now = 0.0;
    if (!SafeCharKey(npc, &key) || !SafeGameHours(&now))
    {
        LogError("[temp] " + name + " joined for " + WithDays("{days}", days) +
                 " days but could not be tracked; they stay as an ordinary member.");
        return;
    }

    const int expiry = TempExpiryHour(now, days);
    float x = 0.0f, y = 0.0f, z = 0.0f;
    const bool placed = NpcMotion::Position(npc, &x, &y, &z);
    const std::string spot = placed ? FormatJoinSpot((int)x, (int)z) : std::string();
    {
        Guard lock;
        // The join day: no daily days-left notice today, the join notice said the total.
        g_book.Join(key, expiry, DayIndexFromHours(now));
        g_homeFaction[key] = homeFactionId;
        if (placed)
            g_joinSpot[key] = spot;
    }
    RaiseSquadHeadNotice(npc, WithDays(Say("tempMember.notice.join", "Te acompanare {days} dias", npc),
                                       days),
                         HeadNoticeKind_Info);

    char note[96] = { 0 };
    _snprintf_s(note, sizeof(note), _TRUNCATE, " [temp] joined for %d days (until hour %d), home ",
                days, expiry);
    LogInfo(name + note + (homeFactionId.empty() ? std::string("unknown") : homeFactionId) + ", joined at " +
            (placed ? "(" + spot + ")" : std::string("an unread spot")) + " [key " + key.ToString() + "]");
}

void TempMemberTick()
{
    const Config& config = GetConfig();
    if (!config.enabled)
        return;
    const DWORD now = GetTickCount();
    if ((LONG)(now - g_nextTick) < 0)
        return;
    g_nextTick = now + kTickIntervalMs;
    if (!HaveMembers())
        return;

    double hours = 0.0;
    if (!SafeGameHours(&hours))
        return;

    Character* squad[kMaxSquad];
    const int count = SafePlayerCharacters(squad, kMaxSquad);
    for (int i = 0; i < count; ++i)
    {
        CharKey key;
        if (SafeFindCharKey(squad[i], &key))
            WatchMember(squad[i], key, hours, config.tempMembers);
    }
}

bool TempMemberDaysLeft(Character* member, int* daysLeft)
{
    CharKey key;
    TempMembership m;
    double now = 0.0;
    if (member == NULL || !SafeFindCharKey(member, &key) || !FindMember(key, &m) ||
        !SafeGameHours(&now))
        return false;
    bool mine = false;
    if (!SafeIsPlayerCharacter(member, &mine) || !mine)
        return false;
    *daysLeft = TempDaysLeft(m.expiryHour, now);
    return true;
}

Character* SelectedTempMember()
{
    Character* selected = SafeSelectedCharacter();
    int days = 0;
    return (selected != NULL && TempMemberDaysLeft(selected, &days)) ? selected : NULL;
}

void ForgetAllTempMembers()
{
    Guard lock;
    g_book.Clear();
    g_homeFaction.clear();
    g_joinSpot.clear();
    g_nextTick = 0;
}

int SavedTempExpiryHour(const std::string& persistedId)
{
    if (persistedId.empty())
        return 0;
    Guard lock;
    return g_book.SavedExpiry(CharKey::FromId(persistedId));
}

void RestoreSavedTempExpiryHour(const std::string& persistedId, int expiryHour)
{
    if (persistedId.empty())
        return;
    Guard lock;
    g_book.RestoreExpiry(CharKey::FromId(persistedId), expiryHour);
}

int SavedTempStrikes(const std::string& persistedId)
{
    if (persistedId.empty())
        return 0;
    Guard lock;
    return g_book.SavedStrikes(CharKey::FromId(persistedId));
}

void RestoreSavedTempStrikes(const std::string& persistedId, int strikes)
{
    if (persistedId.empty())
        return;
    Guard lock;
    g_book.RestoreStrikes(CharKey::FromId(persistedId), strikes);
}

std::string SavedTempHomeFaction(const std::string& persistedId)
{
    if (persistedId.empty())
        return std::string();
    const CharKey key = CharKey::FromId(persistedId);
    Guard lock;
    if (g_book.Find(key) == NULL)
        return std::string();
    std::map<CharKey, std::string>::const_iterator it = g_homeFaction.find(key);
    return it != g_homeFaction.end() ? it->second : std::string();
}

void RestoreSavedTempHomeFaction(const std::string& persistedId, const std::string& factionId)
{
    if (persistedId.empty() || factionId.empty())
        return;
    Guard lock;
    g_homeFaction[CharKey::FromId(persistedId)] = factionId;
}

std::string SavedTempJoinSpot(const std::string& persistedId)
{
    if (persistedId.empty())
        return std::string();
    const CharKey key = CharKey::FromId(persistedId);
    Guard lock;
    if (g_book.Find(key) == NULL)
        return std::string();
    std::map<CharKey, std::string>::const_iterator it = g_joinSpot.find(key);
    return it != g_joinSpot.end() ? it->second : std::string();
}

void RestoreSavedTempJoinSpot(const std::string& persistedId, const std::string& spot)
{
    int x = 0, z = 0;
    if (persistedId.empty() || !ParseJoinSpot(spot, &x, &z))
        return;
    Guard lock;
    g_joinSpot[CharKey::FromId(persistedId)] = spot;
}
} // namespace totalrecruit
