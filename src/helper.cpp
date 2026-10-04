#include "common.h"
#include "helper.h"
#include "helper_rules.h"
#include "temp_member_rules.h"
#include "approach_driver.h"
#include "submission_rules.h"
#include "npc_motion.h"
#include "pacify.h"
#include "plea_tracker.h"
#include "engine_reads.h"
#include "char_key_reader.h"
#include "character_facts.h"
#include "chance_rules.h"
#include "recruit_core.h"
#include "offer_popup.h"
#include "offer_view.h"
#include "npc_speech.h"
#include "dialogue_authoring.h"
#include "head_notice.h"
#include "messages.h"
#include "config.h"
#include "log.h"

#include <cstdio>
#include <map>

// See helper.h for what this does and why it is code-driven. The decisions are
// helper_rules.h; the walk-up is approach_driver.h; this file reads, asks, acts, logs.

namespace totalrecruit
{
namespace
{
const int kMaxSquad = 256;
// How often an offer is looked for, and how often each contract is watched and its
// escort order re-issued (real milliseconds).
const unsigned int kOfferSweepMs = 5000;
const unsigned int kEscortTickMs = 3000;
// The walk rung re-issues its move order only while farther than this.
const float kWalkRungMeters = 6.0f;

// ------------------------------------------------- The locked table
HelperBook g_book;

struct BookMutex
{
    CRITICAL_SECTION value;
    BookMutex() { InitializeCriticalSection(&value); }
    ~BookMutex() { DeleteCriticalSection(&value); }
} g_mutex;

struct Lock
{
    Lock() { EnterCriticalSection(&g_mutex.value); }
    ~Lock() { LeaveCriticalSection(&g_mutex.value); }
private:
    Lock(const Lock&);
    Lock& operator=(const Lock&);
};

bool FindContract(const CharKey& key, HelperContract* out)
{
    Lock lock;
    const HelperContract* c = g_book.Find(key);
    if (c == NULL)
        return false;
    *out = *c;
    return true;
}

bool IsHelperKey(const CharKey& key)
{
    Lock lock;
    return g_book.Find(key) != NULL;
}

std::vector<CharKey> ContractKeys()
{
    Lock lock;
    return g_book.Keys();
}

// ------------------------------------------------- Session state per helper
// The pointer is only used while the key read back from it still matches.
struct Live
{
    Character* npc;
    EscortRung rung;
    int farChecks;
    Character* assistTarget;
    int lastTask;
    bool ordered;

    Live() : npc(NULL), rung(EscortRung_Bodyguard), farChecks(0), assistTarget(NULL), lastTask(-1),
             ordered(false)
    {
    }
};

std::map<CharKey, Live> g_live;

// ------------------------------------------------- The offer in progress
enum OfferState
{
    Offer_Idle,
    Offer_Approaching,
    Offer_Holding
};

struct Offer
{
    OfferState state;
    WalkUp walk;
    int days;
    Offer() : state(Offer_Idle), walk(), days(0) {}
};

Offer g_offer;
PleaTracker g_onScreen;
EventClock g_clock;
DWORD g_nextSweep = 0;
DWORD g_nextEscort = 0;
bool g_debugKeyDown = false;

// ------------------------------------------------------------- Small reads
void Log(const std::string& line)
{
    LogInfo("[helper] " + line);
}

std::string Num(const char* format, double value)
{
    char buffer[64] = { 0 };
    _snprintf_s(buffer, sizeof(buffer), _TRUNCATE, format, value);
    return std::string(buffer);
}

Character* Leader()
{
    Character* leader = NULL;
    return SafePlayerLeader(&leader) ? leader : NULL;
}

bool IsMine(Character* npc)
{
    bool mine = false;
    return npc != NULL && SafeIsPlayerCharacter(npc, &mine) && mine;
}

std::string WithDays(const std::string& text, int days)
{
    std::string out(text);
    const std::string token("{days}");
    const std::string number = Num("%.0f", (double)days);
    std::string::size_type at;
    while ((at = out.find(token)) != std::string::npos)
        out.replace(at, token.size(), number);
    return out;
}

// A line for the roller, with {name} and {days} filled.
std::string Line(const char* key, const char* fallback, Character* npc, int days)
{
    return WithDays(Fill(Say(key, fallback, npc), CharacterName(npc), 0, 0), days);
}

Character* LiveNpc(const CharKey& key)
{
    std::map<CharKey, Live>::iterator it = g_live.find(key);
    if (it == g_live.end() || it->second.npc == NULL)
        return NULL;
    CharKey read;
    if (!SafeFindCharKey(it->second.npc, &read) || !(read == key))
    {
        it->second.npc = NULL;
        return NULL;
    }
    return it->second.npc;
}

// ------------------------------------------------------------- Candidates
// The screen's facts. The cheap ones first: whoever fails them costs no more reads.
HelperCandidate ReadCandidate(Character* npc, Character* leader, double nowHours, int day,
                              bool* complete)
{
    const Config& config = GetConfig();
    HelperCandidate c;
    c.enabled = config.enabled && config.helpers.enabled;
    c.balanced = config.preset == Preset_Balanced;
    c.mine = IsMine(npc);
    bool animal = false;
    c.animal = SafeIsAnimal(npc, &animal) && animal;
    c.up = NpcMotion::IsUp(npc);
    bool caged = false;
    c.caged = SafeInPrison(npc, &caged) && caged;
    c.inCombat = SafeInCombat(npc);
    float meters = -1.0f;
    if (NpcMotion::DistanceMeters(npc, leader, &meters))
        c.distanceMeters = meters;

    // With the relation still unread, a candidate that passed every cheap check is
    // screened out as NotAllied at the earliest; anything before that is a cheap fail.
    const HelperSkip cheap = ScreenHelperCandidate(c, config.helpers, config.friendlyRelation);
    *complete = cheap == HelperSkip_None || cheap >= HelperSkip_NotAllied;
    if (!*complete)
        return c;

    CharKey key;
    if (SafeFindCharKey(npc, &key))
    {
        c.busy = IsHelperKey(key);
        Lock lock;
        c.onCooldown = g_book.OnRejectCooldown(key, nowHours);
        c.rolledToday = g_book.RolledOn(key, day);
    }
    c.onCooldown = c.onCooldown || ReadRejectCooldown(npc, NULL);

    const RescueExtra relation = GatherRescueExtra(npc, Gather_Peek);
    c.relationKnown = relation.relation.known;
    c.relation = relation.relation.value;

    CoreFacts core;
    GatherCoreFacts(npc, &core);
    c.temperament = core.temperament;
    c.npcLevel = core.npcLevel;
    c.leaderLevel = core.playerLevel;
    return c;
}

std::string CandidateText(const HelperCandidate& c)
{
    char buffer[200] = { 0 };
    _snprintf_s(buffer, sizeof(buffer), _TRUNCATE,
                "level %d vs leader %d, relation %s%.0f, temperament %d, %.1f m",
                c.npcLevel, c.leaderLevel, c.relationKnown ? "" : "unknown ", c.relation,
                (int)c.temperament, c.distanceMeters);
    return std::string(buffer);
}

// ------------------------------------------------------------- The offer
void ResetOffer()
{
    g_offer = Offer();
    g_onScreen.Clear();
    ReleaseApproachSlot(ApproachOwner_Helper);
}

bool StartOffer(Character* npc, Character* leader, int days, const char* why)
{
    if (!ClaimApproachSlot(ApproachOwner_Helper))
        return false;
    g_offer = Offer();
    bool moving = false;
    if (!BeginWalkUp(&g_offer.walk, npc, leader, g_clock.Now(), &moving))
    {
        ResetOffer();
        return false;
    }
    g_offer.state = Offer_Approaching;
    g_offer.days = days;

    SpeakAloud(npc, Line("helperEvent.approach", "{name}: Eh, espera. Puedo ayudarte.", npc, days));
    TellPlayer(Line("helperEvent.approachNotice", "{name} se acerca para ofrecerte ayuda.", npc, days),
               false);
    RaiseHeadNotice(npc, Say("headNotice.helperOffer", "Quiere ofrecerte ayuda", npc));

    float meters = -1.0f;
    NpcMotion::DistanceMeters(npc, leader, &meters);
    Log(g_offer.walk.name + " offers " + Num("%.0f", (double)days) + " days (" + why +
        "), walks up from " + Num("%.1f", (double)meters) + " m" + (moving ? "" : "; move order FAILED") +
        " [key " + g_offer.walk.key.ToString() + "]");
    return true;
}

// ------------------------------------------------------------- The contract
void IssueEscort(Character* npc, Character* leader, Live* live);

void StartContract(Character* npc, int days)
{
    const std::string name = CharacterName(npc);
    CharKey key;
    double now = 0.0;
    if (!SafeCharKey(npc, &key) || !SafeGameHours(&now))
    {
        LogError("[helper] " + name + " accepted but could not be tracked; no escort.");
        NpcMotion::ResumeRoutine(npc);
        return;
    }
    const int expiry = TempExpiryHour(now, days);
    {
        Lock lock;
        g_book.Start(key, expiry);
    }
    Live live;
    live.npc = npc;
    g_live[key] = live;

    RaiseHeadNotice(npc, Say("helper.notice.start", "Te cubro las espaldas", npc), HeadNoticeKind_Info);
    TellPlayer(Line("helper.started", "{name} te acompanara {days} dias. No acepta ordenes.", npc, days),
               false);
    Log(name + " escorts the player for " + Num("%.0f", (double)days) + " days (until hour " +
        Num("%.0f", (double)expiry) + ") [key " + key.ToString() + "]");

    IssueEscort(npc, Leader(), &g_live[key]);
}

void EndContract(Character* npc, const CharKey& key, HelperEnd end, double nowHours)
{
    const std::string name = npc != NULL ? CharacterName(npc) : key.ToString();
    {
        Lock lock;
        g_book.End(key);
    }
    g_live.erase(key);
    Log(name + " stops helping: " + HelperEndText(end));

    if (end == HelperEnd_Died || end == HelperEnd_Joined || npc == NULL)
        return;

    if (HelperEndIsBetrayal(end))
    {
        RaiseHeadNotice(npc, Say("helper.notice.betrayed", "No vuelvo a ayudarte", npc), HeadNoticeKind_Info);
        TellPlayer(Line("helper.betrayed", "{name} dejo de ayudarte: tu grupo lo ataco.", npc, 0), false);
        const int days = GetConfig().helpers.rejectCooldownDays;
        if (days > 0)
        {
            Lock lock;
            g_book.SetRejectedUntil(key, RejectCooldownUntilHour(nowHours, (float)(days * 24)));
        }
    }
    else
    {
        RaiseHeadNotice(npc, Say("helper.notice.farewell", "Hasta aqui llego. Suerte", npc), HeadNoticeKind_Info);
        TellPlayer(Line("helper.left", "{name} cumplio su palabra y sigue su camino.", npc, 0), false);
    }
    NpcMotion::ResumeRoutine(npc);
}

// ------------------------------------------------------------- Escort
void LogTaskChange(Character* npc, Live* live)
{
    int task = -1;
    if (!SafeCurrentTask(npc, &task) || task == live->lastTask)
        return;
    live->lastTask = task;
    Log(CharacterName(npc) + ": engine task now " + Num("%.0f", (double)task) + " on rung " +
        EscortRungName(live->rung));
}

// Fights the leader's target when ShouldAssist says so. True when it did.
bool Assist(Character* npc, Character* leader, Live* live)
{
    Character* target = NULL;
    AssistFacts a;
    a.leaderInCombat = SafeInCombat(leader);
    a.haveTarget = a.leaderInCombat && SafeAttackTargetOf(leader, &target);
    if (a.haveTarget)
    {
        a.targetIsSquad = IsMine(target);
        a.targetIsHelper = target == npc;
        const std::string theirs = FactionStringId(target);
        a.targetSameFaction = !theirs.empty() && theirs == FactionStringId(npc);
        a.targetDown = !NpcMotion::IsUp(target);
    }
    if (!ShouldAssist(a))
    {
        live->assistTarget = NULL;
        return false;
    }
    if (target != live->assistTarget || !SafeInCombat(npc))
    {
        const bool ordered = NpcMotion::AttackFor(npc, target);
        if (target != live->assistTarget)
            Log(CharacterName(npc) + ": fights for the leader against " + CharacterName(target) +
                (ordered ? "" : " (order FAILED)"));
        live->assistTarget = target;
    }
    return true;
}

void IssueEscort(Character* npc, Character* leader, Live* live)
{
    if (npc == NULL || leader == NULL)
        return;

    // At peace with the squad: no temporary-enemy marks either way.
    Character* squad[kMaxSquad];
    const int count = SafePlayerCharacters(squad, kMaxSquad);
    SafeClearTempEnemies(npc, squad, count);

    if (Assist(npc, leader, live))
    {
        LogTaskChange(npc, live);
        return;
    }
    const bool fighting = SafeInCombat(npc);
    float meters = -1.0f;
    NpcMotion::DistanceMeters(npc, leader, &meters);
    live->farChecks = NextFarChecks(live->farChecks, meters, fighting);
    const EscortRung next = NextEscortRung(live->rung, live->farChecks);
    if (next != live->rung)
    {
        Log(CharacterName(npc) + ": left behind (" + Num("%.1f", (double)meters) + " m), escort " +
            EscortRungName(live->rung) + " -> " + EscortRungName(next));
        live->rung = next;
        live->farChecks = 0;
    }
    if (fighting)
    {
        // Its own fight (someone went for it, or for the leader): let it finish.
        LogTaskChange(npc, live);
        return;
    }

    bool ok = true;
    switch (live->rung)
    {
    case EscortRung_Bodyguard:
        ok = NpcMotion::GuardLeader(npc, leader, NpcMotion::BodyguardTask());
        break;
    case EscortRung_FollowOrder:
        ok = NpcMotion::GuardLeader(npc, leader, NpcMotion::FollowPlayerTask());
        break;
    case EscortRung_Walk:
        if (meters < 0.0f || meters > kWalkRungMeters)
            ok = NpcMotion::MoveTo(npc, leader);
        break;
    }
    if (!live->ordered || !ok)
    {
        Log(CharacterName(npc) + ": escort order " + EscortRungName(live->rung) + (ok ? " given" : " FAILED"));
        live->ordered = ok;
    }
    LogTaskChange(npc, live);
}

// After a load the table is back but the pointers are not: whoever is near again
// gets their escort back.
void Rearm(Character* leader)
{
    const std::vector<CharKey> keys = ContractKeys();
    bool missing = false;
    for (unsigned int i = 0; i < keys.size() && !missing; ++i)
        missing = LiveNpc(keys[i]) == NULL;
    if (!missing)
        return;

    float x = 0.0f, y = 0.0f, z = 0.0f;
    std::vector<Character*> nearby;
    if (!NpcMotion::Position(leader, &x, &y, &z) ||
        !CharactersNear(Ogre::Vector3(x, y, z), MetersToWorldUnits(kHelperRearmMeters), &nearby))
        return;
    for (unsigned int i = 0; i < nearby.size(); ++i)
    {
        CharKey key;
        if (nearby[i] == NULL || !SafeFindCharKey(nearby[i], &key) || !IsHelperKey(key) ||
            LiveNpc(key) != NULL)
            continue;
        Live live;
        live.npc = nearby[i];
        g_live[key] = live;
        RaiseHeadNotice(nearby[i], Say("helper.notice.rearmed", "Sigo contigo", nearby[i]),
                        HeadNoticeKind_Info);
        Log(CharacterName(nearby[i]) + ": re-armed after a load [key " + key.ToString() + "]");
    }
}

HelperSignals ReadSignals(Character* npc, double nowHours)
{
    HelperSignals s;
    s.nowHours = nowHours;
    bool dead = false;
    s.dead = SafeIsDead(npc, &dead) && dead;
    s.joinedSquad = IsMine(npc);
    SafeAttackedByPlayer(npc, &s.attackedBySquad);
    bool down = false;
    if (SafeIsKnockedOut(npc, &down) && down)
    {
        bool byPlayer = false;
        s.knockedOutBySquad = SafeDefeatedByPlayer(npc, &byPlayer) && byPlayer;
    }
    return s;
}

void WatchContracts()
{
    Character* leader = Leader();
    double now = 0.0;
    if (!SafeGameHours(&now))
        return;
    if (leader != NULL)
        Rearm(leader);

    const Helpers& config = GetConfig().helpers;
    const std::vector<CharKey> keys = ContractKeys();
    for (unsigned int i = 0; i < keys.size(); ++i)
    {
        Character* npc = LiveNpc(keys[i]);
        if (npc == NULL)
            continue;
        HelperContract c;
        if (!FindContract(keys[i], &c))
            continue;
        const HelperStep step = StepHelper(&c, ReadSignals(npc, now), config);
        {
            Lock lock;
            g_book.Update(keys[i], c);
        }
        if (step.end != HelperEnd_None)
        {
            EndContract(npc, keys[i], step.end, now);
            continue;
        }
        if (step.expiryWarn)
        {
            RaiseHeadNotice(npc, Say("helper.notice.expiring", "Me ire pronto", npc), HeadNoticeKind_Info);
            Log(CharacterName(npc) + ": expiry warning (" + Num("%.0f", (double)c.expiryHour - now) + " h left)");
        }
        std::map<CharKey, Live>::iterator live = g_live.find(keys[i]);
        if (live != g_live.end() && NpcMotion::IsUp(leader))
            IssueEscort(npc, leader, &live->second);
    }
}

// ------------------------------------------------------------- Phases
void EndOffer(const char* outcome)
{
    Character* npc = WalkUpNpc(g_offer.walk);
    DismissHeadNotice(npc);
    Log(g_offer.walk.name + ": offer " + outcome);
    if (npc != NULL)
        NpcMotion::ResumeRoutine(npc);
    ResetOffer();
}

bool OpenPopup(Character* npc)
{
    const OfferView view = BuildHelperOfferView(g_offer.walk.name, g_offer.days, OfferWording());
    if (!ShowOffer(npc, view))
        return false;
    g_onScreen.Begin(g_offer.walk.key);
    DismissHeadNotice(npc);
    return true;
}

void StepApproach()
{
    const ApproachAction action =
        StepWalkUp(&g_offer.walk, g_clock.Now(),
                   SubmissionApproachLimits(GetConfig().submission.approachSeconds), "[helper]");
    if (action.kind == Approach_Arrive)
    {
        if (!OpenPopup(WalkUpNpc(g_offer.walk)))
        {
            EndOffer("aborted: the popup could not open");
            return;
        }
        g_offer.state = Offer_Holding;
        g_offer.walk.phaseStart = g_clock.Now();
        g_offer.walk.lastReassert = g_clock.Now();
    }
    else if (action.kind == Approach_Abort)
    {
        EndOffer((std::string("aborted: ") + action.text).c_str());
    }
}

void StepHold()
{
    Character* npc = WalkUpNpc(g_offer.walk);
    Character* leader = Leader();
    HoldInput in;
    in.npcValid = NpcMotion::IsUp(npc);
    in.leaderValid = NpcMotion::IsUp(leader);
    in.calmBroken = CalmBroken(g_offer.walk.strikes);
    in.popupStillOurs = IsOfferOpen() && g_onScreen.IsPending(g_offer.walk.key);
    in.sinceLastReassertSec = g_clock.Now() - g_offer.walk.lastReassert;

    const HoldAction action = HoldStep(in);
    if (action.kind == Hold_Reassert)
    {
        ReassertCalm(&g_offer.walk, npc);
        NpcMotion::Stand(npc, leader);
        g_offer.walk.lastReassert = g_clock.Now();
    }
    else if (action.kind == Hold_Release)
    {
        if (action.reason != HoldRelease_Closed && g_onScreen.IsPending(g_offer.walk.key))
            HideOffer();
        EndOffer((std::string("released: ") + HoldReleaseText(action.reason)).c_str());
    }
}

// ------------------------------------------------------------- The sweep
void Sweep()
{
    const Config& config = GetConfig();
    if (!config.enabled || !config.helpers.enabled || config.preset != Preset_Balanced ||
        IsOfferOpen() || !ApproachSlotFreeFor(ApproachOwner_Helper))
        return;
    bool paused = false;
    if (SafeIsPaused(&paused) && paused)
        return;

    Character* leader = Leader();
    float lx = 0.0f, ly = 0.0f, lz = 0.0f;
    double now = 0.0;
    if (!NpcMotion::IsUp(leader) || !NpcMotion::Position(leader, &lx, &ly, &lz) ||
        !SafeGameHours(&now))
        return;
    const int day = DayIndexFromHours(now);

    std::vector<Character*> nearby;
    if (!CharactersNear(Ogre::Vector3(lx, ly, lz), MetersToWorldUnits(kHelperTriggerMeters), &nearby))
        return;

    for (unsigned int i = 0; i < nearby.size(); ++i)
    {
        Character* npc = nearby[i];
        if (npc == NULL || npc == leader)
            continue;
        bool complete = false;
        const HelperCandidate c = ReadCandidate(npc, leader, now, day, &complete);
        if (!complete || ScreenHelperCandidate(c, config.helpers, config.friendlyRelation) != HelperSkip_None)
            continue;

        CharKey key;
        if (!SafeCharKey(npc, &key))
            continue;
        {
            Lock lock;
            g_book.NoteRolled(key, day);
        }
        if (!HelperOfferRoll(DailySeed(key, day, kHelperOfferSalt), config.helpers.offerChancePerDay))
        {
            Log(CharacterName(npc) + ": no offer today (" + CandidateText(c) + ", chance " +
                Num("%.2f", (double)config.helpers.offerChancePerDay) + ", day " + Num("%.0f", (double)day) + ")");
            continue;
        }
        const int days = HelperDaysFromSeed(DailySeed(key, day, kHelperDaysSalt), config.helpers);
        if (StartOffer(npc, leader, days, ("roll: " + CandidateText(c)).c_str()))
            return;
    }
}

// ------------------------------------------------------------- Debug
bool GameHasFocus()
{
    HWND window = GetForegroundWindow();
    DWORD process = 0;
    if (window == NULL)
        return false;
    GetWindowThreadProcessId(window, &process);
    return process == GetCurrentProcessId();
}

void DebugOffer()
{
    const Config& config = GetConfig();
    Character* npc = SafeSelectedCharacter();
    Character* leader = Leader();
    if (npc == NULL || IsMine(npc) || leader == NULL)
    {
        Log("debug key: select an NPC that is not yours first");
        TellPlayer(Say("helperEvent.debugSelect", "[TotalRecruit] Selecciona un NPC que no sea tuyo.", NULL),
                   true);
        return;
    }
    double now = 0.0;
    SafeGameHours(&now);
    const int day = DayIndexFromHours(now);
    bool complete = false;
    const HelperCandidate c = ReadCandidate(npc, leader, now, day, &complete);
    const HelperSkip skip = ScreenHelperCandidate(c, config.helpers, config.friendlyRelation);
    Log("debug key on " + CharacterName(npc) + ": screen says " + HelperSkipText(skip) +
        (complete ? " (" + CandidateText(c) + ")" : std::string(" (cheap checks only)")));

    if (g_offer.state != Offer_Idle || IsOfferOpen() || !ApproachSlotFreeFor(ApproachOwner_Helper))
    {
        Log("debug key: another event or window is in the way");
        return;
    }
    if (c.mine || c.animal || !c.up || c.busy)
    {
        Log("debug key: cannot offer (mine, animal, down or already helping)");
        return;
    }
    CharKey key;
    if (!SafeCharKey(npc, &key))
        return;
    const int days = HelperDaysFromSeed(DailySeed(key, day, kHelperDaysSalt), config.helpers);
    StartOffer(npc, leader, days, "debug key");
}

void PollDebugKey()
{
    const int vk = GetConfig().helpers.debugKey;
    if (vk <= 0)
    {
        g_debugKeyDown = false;
        return;
    }
    const bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
    const bool pressed = down && !g_debugKeyDown;
    g_debugKeyDown = down;
    if (pressed && GameHasFocus())
        DebugOffer();
}

bool AnswersTheOffer(Character* npc, const char* answer)
{
    CharKey key;
    const bool ours = g_offer.state == Offer_Holding && npc != NULL && SafeFindCharKey(npc, &key) &&
                      key == g_offer.walk.key;
    if (!ours)
        Log(std::string(answer) + " ignored: no helper offer in progress for " + CharacterName(npc));
    return ours;
}
} // namespace

void HelperTick()
{
    const Config& config = GetConfig();
    if (!config.enabled)
        return;
    g_clock.Advance();
    PollDebugKey();

    switch (g_offer.state)
    {
    case Offer_Approaching: StepApproach(); break;
    case Offer_Holding:     StepHold(); break;
    default: break;
    }

    const DWORD now = GetTickCount();
    if ((LONG)(now - g_nextEscort) >= 0)
    {
        g_nextEscort = now + kEscortTickMs;
        bool paused = false;
        if (!(SafeIsPaused(&paused) && paused))
            WatchContracts();
    }
    if (g_offer.state == Offer_Idle && (LONG)(now - g_nextSweep) >= 0)
    {
        g_nextSweep = now + kOfferSweepMs;
        Sweep();
    }
}

bool IsHelperOfferPending(Character* npc)
{
    CharKey key;
    return npc != NULL && g_onScreen.HasPending() && SafeFindCharKey(npc, &key) &&
           g_onScreen.IsPending(key);
}

void ForgetHelperOffer()
{
    g_onScreen.Clear();
}

void AcceptHelperOffer(Character* npc)
{
    if (!AnswersTheOffer(npc, "accept"))
        return;
    const int days = g_offer.days;
    DismissHeadNotice(npc);
    Log(g_offer.walk.name + ": offer accepted");
    ResetOffer();
    StartContract(npc, days);
}

void RejectHelperOffer(Character* npc)
{
    if (!AnswersTheOffer(npc, "reject"))
        return;
    const int days = GetConfig().helpers.rejectCooldownDays;
    CharKey key;
    double now = 0.0;
    if (days > 0 && SafeCharKey(npc, &key) && SafeGameHours(&now))
    {
        const int until = RejectCooldownUntilHour(now, (float)(days * 24));
        Lock lock;
        g_book.SetRejectedUntil(key, until);
    }
    SpeakAloud(npc, Say("helperEvent.rejected", "Como quieras. Suerte en el camino.", npc));
    TellPlayer(Line("helperEvent.rejectedNotice", "{name}: rechazaste su ayuda.", npc, days), false);
    EndOffer(("rejected; no offer for " + Num("%.0f", (double)days) + " days").c_str());
}

void DismissHelperOffer(Character* npc)
{
    if (AnswersTheOffer(npc, "dismiss"))
        EndOffer("dismissed (closed with no answer)");
}

bool HelperDaysLeft(Character* npc, int* daysLeft)
{
    CharKey key;
    HelperContract c;
    double now = 0.0;
    if (npc == NULL || !SafeFindCharKey(npc, &key) || !FindContract(key, &c) || !SafeGameHours(&now))
        return false;
    *daysLeft = TempDaysLeft(c.expiryHour, now);
    return true;
}

void ForgetHelpers()
{
    if (g_offer.state != Offer_Idle)
    {
        Character* npc = WalkUpNpc(g_offer.walk);
        if (npc != NULL)
            NpcMotion::ResumeRoutine(npc);
    }
    ResetOffer();
    g_clock.Reset();
    g_live.clear();
    g_nextSweep = 0;
    g_nextEscort = 0;
    Lock lock;
    g_book.Clear();
}

int SavedHelperExpiryHour(const std::string& persistedId)
{
    if (persistedId.empty())
        return 0;
    Lock lock;
    return g_book.SavedExpiry(CharKey::FromId(persistedId));
}

void RestoreSavedHelperExpiryHour(const std::string& persistedId, int expiryHour)
{
    if (persistedId.empty())
        return;
    Lock lock;
    g_book.RestoreExpiry(CharKey::FromId(persistedId), expiryHour);
}

int SavedHelperRejectUntil(const std::string& persistedId)
{
    if (persistedId.empty())
        return 0;
    Lock lock;
    return g_book.SavedRejectedUntil(CharKey::FromId(persistedId));
}

void RestoreSavedHelperRejectUntil(const std::string& persistedId, int untilHour)
{
    if (persistedId.empty())
        return;
    Lock lock;
    g_book.RestoreRejectedUntil(CharKey::FromId(persistedId), untilHour);
}
} // namespace totalrecruit
