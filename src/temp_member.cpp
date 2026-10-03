#include "common.h"
#include "temp_member.h"
#include "temp_member_rules.h"
#include "chance_rules.h"
#include "char_key_reader.h"
#include "config.h"
#include "engine_reads.h"
#include "head_notice.h"
#include "log.h"
#include "messages.h"
#include "recruit_core.h"
#include "squad_leave.h"
#include "submission_rules.h"
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
}

std::string HomeFactionOf(const CharKey& key)
{
    Guard lock;
    std::map<CharKey, std::string>::const_iterator it = g_homeFaction.find(key);
    return it != g_homeFaction.end() ? it->second : std::string();
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
    return std::string();
}

void LogStep(Character* member, const TempMembership& m, const TempMemberStep& step)
{
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
    if (!LeaveSquad(member, HomeFactionOf(key)))
    {
        // They stay as an ordinary member rather than being asked again every tick.
        RemoveMember(key);
        LogError("[temp] " + name + " could not leave the squad; they stay as an ordinary member.");
        return;
    }

    RaiseHeadNotice(member, mistreated
                                ? Say("tempMember.notice.farewellMistreated", "Me largo", member)
                                : Say("tempMember.notice.farewell", "Me voy. Fue un honor", member));
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
        SetRejectCooldownUntil(key, RejectCooldownUntilHour(nowHours, (float)(days * 24)));

    RemoveMember(key);
    LogInfo("[temp] " + name + (mistreated ? " left: mistreated." : " left: time up."));
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
        RaiseSquadHeadNotice(member, notice);
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
    {
        Guard lock;
        g_book.Join(key, expiry);
        g_homeFaction[key] = homeFactionId;
    }
    RaiseSquadHeadNotice(npc, WithDays(Say("tempMember.notice.join", "Te acompanare {days} dias", npc),
                                       days));

    char note[96] = { 0 };
    _snprintf_s(note, sizeof(note), _TRUNCATE, " [temp] joined for %d days (until hour %d), home ",
                days, expiry);
    LogInfo(name + note + (homeFactionId.empty() ? std::string("unknown") : homeFactionId) +
            " [key " + key.ToString() + "]");
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
} // namespace totalrecruit
