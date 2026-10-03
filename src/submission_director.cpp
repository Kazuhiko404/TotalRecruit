#include "common.h"
#include "submission_director.h"
#include "submission_rules.h"
#include "submission_tracker.h"
#include "approach_rules.h"
#include "npc_motion.h"
#include "pacify.h"
#include "plea_tracker.h"
#include "engine_reads.h"
#include "char_key_reader.h"
#include "character_facts.h"
#include "chance_rules.h"
#include "defeats.h"
#include "surrender.h"
#include "rules.h"
#include "recruit_core.h"
#include "offer_popup.h"
#include "offer_view.h"
#include "npc_speech.h"
#include "dialogue_authoring.h"
#include "messages.h"
#include "reason_catalog.h"
#include "config.h"
#include "log.h"

#include <cstdio>

// The submission event's director: TotalSparring's challenge_director.cpp
// (state machine :91-96, paused-aware clock :50-85, walk-up :886-893, hold,
// reset :948) ported to "a beaten NPC asks to join". The pure steps live in
// submission_rules.h / approach_rules.h; the engine calls in npc_motion.h
// (moving) and pacify.h (calm and hostility). This file reads, asks, acts, logs.

namespace totalrecruit
{
namespace
{
const int kMaxSquad = 256;

// ----------------------------------------------------------------- Clock
float g_clock = 0.0f;
DWORD g_lastTick = 0;
bool g_haveLastTick = false;
unsigned int g_nextSweep = 0;

void AdvanceClock()
{
    const DWORD now = GetTickCount();
    if (!g_haveLastTick)
    {
        g_haveLastTick = true;
        g_lastTick = now;
        return;
    }
    const DWORD elapsed = now - g_lastTick;
    g_lastTick = now;
    bool paused = false;
    SafeIsPaused(&paused);
    g_clock = AdvanceEventClock(g_clock, (unsigned int)elapsed, paused);
}

// ----------------------------------------------------------------- State
// One event at a time. The NPC is held as a pointer plus its key, and the
// pointer is only used after the key read back from it still matches: an
// address is not an identity (char_key.h).
struct Event
{
    SubmissionState state;
    Character* npc;
    CharKey key;
    std::string name;
    bool wasHostile;
    int strikes;
    float phaseStart;
    float lastReassert;
    bool standing;
    float windowSec;
    bool fleeing;
    bool fleeArrived;
    GroundPoint fleeTo;

    Event()
        : state(Submission_Idle), npc(NULL), key(), name(), wasHostile(false), strikes(0),
          phaseStart(0.0f), lastReassert(0.0f), standing(false), windowSec(0.0f),
          fleeing(false), fleeArrived(false), fleeTo()
    {
    }
};

Event g_event;

// Whose submission popup is on screen (the PleaTracker shape, mercy.cpp).
PleaTracker g_onScreen;

void Log(const std::string& line)
{
    LogInfo("[submission] " + line);
}

std::string Fmt(const char* format, double a, double b)
{
    char buffer[128] = { 0 };
    _snprintf_s(buffer, sizeof(buffer), _TRUNCATE, format, a, b);
    return std::string(buffer);
}

Character* EventNpc()
{
    if (g_event.npc == NULL)
    {
        return NULL;
    }
    CharKey key;
    if (!SafeFindCharKey(g_event.npc, &key) || !(key == g_event.key))
    {
        return NULL;
    }
    return g_event.npc;
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

// The calm, once a second: first note whether the last calm held (a fight
// that keeps coming back breaks it, CalmBroken), then clear it again toward
// every squad member.
void Reassert(Character* npc)
{
    if (npc == NULL)
    {
        return;
    }
    g_event.strikes = NextCombatStrikes(g_event.strikes, SafeInCombat(npc));
    Character* squad[kMaxSquad];
    const int count = SafePlayerCharacters(squad, kMaxSquad);
    SafeCalmToward(npc, squad, count);
}

bool HostileBefore(Character* npc)
{
    Character* squad[kMaxSquad];
    const int count = SafePlayerCharacters(squad, kMaxSquad);
    return SafeHostileToward(npc, squad, count);
}

void ResetEvent()
{
    g_event = Event();
    g_onScreen.Clear();
}

std::string Line(const char* key, const char* fallback, Character* npc, int hours)
{
    return Fill(Say(key, fallback, npc), CharacterName(npc), 0, hours, 0);
}

// ----------------------------------------------------------------- Outcomes
void EnterForgiven(Character* npc, SubmissionOutcome outcome, bool flee)
{
    const Config& config = GetConfig();
    g_event.state = Submission_Forgiven;
    g_event.phaseStart = g_clock;
    g_event.lastReassert = g_clock - kSubmissionReassertSec;
    g_event.windowSec = ForgivenWindowSec(outcome, config.submission.forgiveSeconds);
    g_event.fleeing = false;
    g_event.fleeArrived = false;

    Character* leader = Leader();
    float nx = 0.0f, ny = 0.0f, nz = 0.0f;
    float lx = 0.0f, ly = 0.0f, lz = 0.0f;
    if (flee && NpcMotion::Position(npc, &nx, &ny, &nz) &&
        NpcMotion::Position(leader, &lx, &ly, &lz))
    {
        g_event.fleeTo = FleePoint(GroundPoint(nx, ny, nz), GroundPoint(lx, ly, lz),
                                   MetersToWorldUnits(kSubmissionFleeMeters));
        NpcMotion::BeginRun(npc);
        g_event.fleeing = NpcMotion::MoveToPoint(npc, g_event.fleeTo.x, g_event.fleeTo.y,
                                                 g_event.fleeTo.z);
    }
    else
    {
        // Let be in place: the stand order goes, the calm stays.
        NpcMotion::ResumeRoutine(npc);
    }
    Log(g_event.name + Fmt(": calm for %.0f s", g_event.windowSec, 0.0) +
        (flee ? (g_event.fleeing ? ", running off" : ", running off FAILED (stays)") : ", in place"));
}

// Acts on an outcome with the plan PlanFor gives it, then either enters the
// forgive window or ends the event.
void Apply(SubmissionOutcome outcome)
{
    Character* npc = EventNpc();
    const bool up = NpcMotion::IsUp(npc);
    Character* leader = Leader();
    const RestorePlan plan = PlanFor(outcome, g_event.wasHostile, up);
    const std::string name = g_event.name;

    Log(name + ": " + SubmissionOutcomeName(outcome) + " (from " +
        SubmissionStateName(g_event.state) + ", hostile before " +
        (g_event.wasHostile ? "yes" : "no") + ")");

    if (plan.next == Submission_Forgiven && npc != NULL)
    {
        if (outcome == Outcome_Forgiven)
        {
            SpeakAloud(npc, Say("submissionEvent.forgiven", "Gracias... no lo olvidar\xc3\xa9.", npc));
            TellPlayer(Line("submissionEvent.forgivenNotice", "{name}: lo perdonaste y se aleja.",
                            npc, 0),
                       false);
        }
        EnterForgiven(npc, outcome, plan.flee);
        return;
    }

    if (plan.restoreSpeed)
    {
        NpcMotion::EndRun(npc);
    }

    if (npc != NULL && plan.clearDefeats)
    {
        ClearDefeats(npc);
    }

    if (npc != NULL && plan.startCooldown)
    {
        const Config& config = GetConfig();
        CharKey key;
        double now = 0.0;
        int until = 0;
        if (SafeCharKey(npc, &key) && SafeGameHours(&now))
        {
            until = RejectCooldownUntilHour(now, config.submission.rejectCooldownHours);
            SetRejectCooldownUntil(key, until);
        }
        const int hours = (int)(config.submission.rejectCooldownHours + 0.5f);
        Log(name + Fmt(": reject cooldown until game hour %.0f (%.0f h)", (double)until,
                       (double)hours));
        SpeakAloud(npc, Say("submissionEvent.rejected", "Entonces nos volveremos a ver.", npc));
        TellPlayer(Line("submissionEvent.rejectedNotice",
                        "{name}: lo rechazaste. No querr\xc3\xa1 saber nada de ti en {hours} h.",
                        npc, hours),
                   false);
    }

    if (npc != NULL && plan.markHostile)
    {
        SafeMarkHostile(npc, leader);
    }

    if (npc != NULL && plan.resumeRoutine)
    {
        NpcMotion::ResumeRoutine(npc);
    }

    if (npc != NULL && plan.recruit)
    {
        std::string message;
        if (Recruit(npc, &message))
        {
            TellPlayer(Line("submissionEvent.joinedNotice", "{name}: se une a ti.", npc, 0), false);
            Log(name + ": joined the squad");
        }
        else
        {
            TellPlayer(name + ": " + message, true);
            Log(name + ": recruit FAILED: " + message);
        }
    }

    ResetEvent();
}

// --------------------------------------------------------------- Approach
bool OpenPopup(Character* npc)
{
    // ScreenCandidate kept the caged out, but the player can pick the NPC up on
    // the way: read it again, so someone who cannot leave is never offered PERDONAR.
    bool caged = false;
    const bool inCage = SafeInPrison(npc, &caged) && caged;
    const OfferView view = BuildSubmissionOfferView(g_event.name, inCage, OfferWording());
    if (!ShowOffer(npc, view))
    {
        return false;
    }
    // Marked only now that the window is up: opening it cleared any older mark.
    g_onScreen.Begin(g_event.key);
    return true;
}

void StepApproach()
{
    const Config& config = GetConfig();
    Character* npc = EventNpc();
    Character* leader = Leader();

    ApproachInput in;
    in.npcValid = NpcMotion::IsUp(npc);
    in.targetValid = NpcMotion::IsUp(leader);
    in.elapsedSec = g_clock - g_event.phaseStart;
    in.sinceLastRepathSec = g_clock - g_event.lastReassert;
    in.combatStarted = CalmBroken(g_event.strikes);
    in.targetBusy = IsOfferOpen() || SafeInCombat(leader);
    float meters = -1.0f;
    if (in.npcValid && in.targetValid && NpcMotion::DistanceMeters(npc, leader, &meters))
    {
        in.distanceMeters = meters;
    }

    const ApproachAction action =
        ApproachStep(in, SubmissionApproachLimits(config.submission.approachSeconds));
    switch (action.kind)
    {
    case Approach_KeepWalking:
        return;

    case Approach_Repath:
        Reassert(npc);
        g_event.standing = false;
        if (!NpcMotion::MoveTo(npc, leader))
        {
            Log(g_event.name + ": move order FAILED");
        }
        g_event.lastReassert = g_clock;
        return;

    case Approach_Stand:
        Reassert(npc);
        if (g_event.standing)
        {
            NpcMotion::Stand(npc, leader);
        }
        else
        {
            NpcMotion::StopWalking(npc, true);
            g_event.standing = NpcMotion::BeginStand(npc, leader);
            Log(g_event.name + ": arrived, waits for the window to be free");
        }
        g_event.lastReassert = g_clock;
        return;

    case Approach_Arrive:
        NpcMotion::StopWalking(npc, true);
        Reassert(npc);
        NpcMotion::BeginStand(npc, leader);
        Log(g_event.name + ": " + action.text + Fmt(" (%.1f s)", in.elapsedSec, 0.0));
        if (!OpenPopup(npc))
        {
            Log(g_event.name + ": the popup could not open");
            Apply(Outcome_Aborted);
            return;
        }
        g_event.state = Submission_Holding;
        g_event.phaseStart = g_clock;
        g_event.lastReassert = g_clock;
        return;

    case Approach_Abort:
        Log(g_event.name + ": approach aborted: " + action.text);
        Apply(Outcome_Aborted);
        return;
    }
}

// ------------------------------------------------------------------- Hold
void StepHold()
{
    Character* npc = EventNpc();
    Character* leader = Leader();

    HoldInput in;
    in.npcValid = NpcMotion::IsUp(npc);
    in.leaderValid = NpcMotion::IsUp(leader);
    in.calmBroken = CalmBroken(g_event.strikes);
    in.popupStillOurs = IsOfferOpen() && g_onScreen.IsPending(g_event.key);
    in.sinceLastReassertSec = g_clock - g_event.lastReassert;

    const HoldAction action = HoldStep(in);
    switch (action.kind)
    {
    case Hold_Keep:
        return;

    case Hold_Reassert:
        Reassert(npc);
        NpcMotion::Stand(npc, leader);
        g_event.lastReassert = g_clock;
        return;

    case Hold_Release:
        Log(g_event.name + ": hold released: " + HoldReleaseText(action.reason));
        if (action.reason == HoldRelease_Closed)
        {
            Apply(Outcome_Dismissed);
            return;
        }
        // The window must not outlive the event it asks about.
        if (g_onScreen.IsPending(g_event.key))
        {
            HideOffer();
        }
        Apply(action.reason == HoldRelease_Combat ? Outcome_CombatTookOver : Outcome_Aborted);
        return;
    }
}

// --------------------------------------------------------------- Forgiven
bool NearFleePoint(Character* npc)
{
    float x = 0.0f, y = 0.0f, z = 0.0f;
    if (!NpcMotion::Position(npc, &x, &y, &z))
    {
        return false;
    }
    const float dx = x - g_event.fleeTo.x;
    const float dz = z - g_event.fleeTo.z;
    const float units = MetersToWorldUnits(kSubmissionArriveMeters);
    return dx * dx + dz * dz <= units * units;
}

void StepForgiven()
{
    Character* npc = EventNpc();

    ForgivenInput in;
    in.elapsedSec = g_clock - g_event.phaseStart;
    in.windowSec = g_event.windowSec;
    in.sinceLastReassertSec = g_clock - g_event.lastReassert;
    in.npcValid = NpcMotion::IsUp(npc);
    in.joined = IsMine(npc);
    in.calmBroken = CalmBroken(g_event.strikes);

    const ForgivenAction action = ForgivenStep(in);
    switch (action.kind)
    {
    case Forgiven_Keep:
        return;

    case Forgiven_Reassert:
        Reassert(npc);
        if (g_event.fleeing && !g_event.fleeArrived)
        {
            if (NearFleePoint(npc))
            {
                g_event.fleeArrived = true;
                NpcMotion::StopWalking(npc, false);
                Log(g_event.name + ": ran off; stays calm");
            }
            else
            {
                NpcMotion::MoveToPoint(npc, g_event.fleeTo.x, g_event.fleeTo.y, g_event.fleeTo.z);
                NpcMotion::ReapplyRun(npc);
            }
        }
        g_event.lastReassert = g_clock;
        return;

    case Forgiven_Release:
        Log(g_event.name + Fmt(": forgive window over after %.0f s (", in.elapsedSec, 0.0) +
            ForgivenReleaseText(action.reason) + ")");
        switch (action.reason)
        {
        case ForgivenRelease_Joined: Apply(Outcome_JoinedLater); return;
        case ForgivenRelease_Combat: Apply(Outcome_CombatTookOver); return;
        default:                     Apply(Outcome_ForgiveOver); return;
        }
    }
}

// ------------------------------------------------------------------ Sweep
// Cheap facts first; the verdict only for someone who passed them.
CandidateFacts CheapFacts(Character* npc)
{
    CandidateFacts f;
    f.eventEnabled = true;
    f.recruitable = true;
    f.mayAsk = true;
    f.beaten = true;
    f.mine = IsMine(npc);
    bool animal = false;
    f.animal = SafeIsAnimal(npc, &animal) && animal;
    f.up = NpcMotion::IsUp(npc);
    bool caged = false;
    f.caged = SafeInPrison(npc, &caged) && caged;
    return f;
}

void CostlyFacts(Character* npc, int day, int defeats, CandidateFacts* f)
{
    f->recruitable = IsRecruitable(npc, NULL);
    f->onRejectCooldown = ReadRejectCooldown(npc, NULL);
    CharKey key;
    if (SafeFindCharKey(npc, &key))
    {
        f->mayAsk = MaySubmissionAsk(key, defeats);
        f->rolledNoToday = SubmissionRollFailedOn(key, day);
    }
    f->beaten = defeats >= GetConfig().submission.defeatsToBreak || HasYieldedToPlayer(npc);
}

bool Start(Character* npc, Character* leader, int defeats, const Verdict& verdict)
{
    CharKey key;
    if (!SafeCharKey(npc, &key))
    {
        return false;
    }
    MarkSubmissionAsked(key, defeats);

    g_event = Event();
    g_event.state = Submission_Approaching;
    g_event.npc = npc;
    g_event.key = key;
    g_event.name = CharacterName(npc);
    g_event.wasHostile = HostileBefore(npc);
    g_event.phaseStart = g_clock;
    g_event.lastReassert = g_clock;

    Reassert(npc);
    g_event.strikes = 0;
    const bool moving = NpcMotion::MoveTo(npc, leader);

    SpeakAloud(npc, Line("submissionEvent.approach",
                         "{name}: Espera... no quiero pelear m\xc3\xa1s.", npc, 0));
    TellPlayer(Line("submissionEvent.approachNotice", "{name} se acerca para hablar contigo.",
                    npc, 0),
               false);

    float meters = -1.0f;
    NpcMotion::DistanceMeters(npc, leader, &meters);
    Log(g_event.name + " submitted (" + ReasonMessageKey(verdict.reasonKey, "") + "), walks up" +
        Fmt(" from %.1f m, defeats %.0f", meters, (double)defeats) + "; hostile before " +
        (g_event.wasHostile ? "yes" : "no") + (moving ? "" : "; move order FAILED") + " [key " +
        key.ToString() + "]");
    return true;
}

void Sweep()
{
    const Config& config = GetConfig();
    if (!config.submission.eventEnabled || IsOfferOpen())
    {
        return;
    }
    bool paused = false;
    if (SafeIsPaused(&paused) && paused)
    {
        return;
    }

    Character* leader = Leader();
    float lx = 0.0f, ly = 0.0f, lz = 0.0f;
    double now = 0.0;
    if (!NpcMotion::IsUp(leader) || !NpcMotion::Position(leader, &lx, &ly, &lz) ||
        !SafeGameHours(&now))
    {
        return;
    }
    const int day = DayIndexFromHours(now);

    std::vector<Character*> nearby;
    if (!CharactersNear(Ogre::Vector3(lx, ly, lz),
                        MetersToWorldUnits(kSubmissionTriggerMeters), &nearby))
    {
        return;
    }

    for (unsigned int i = 0; i < nearby.size(); ++i)
    {
        Character* npc = nearby[i];
        if (npc == NULL || npc == leader)
        {
            continue;
        }

        CandidateFacts facts = CheapFacts(npc);
        if (ScreenCandidate(facts) != CandidateSkip_None)
        {
            continue;
        }
        const int defeats = DefeatCount(npc);
        CostlyFacts(npc, day, defeats, &facts);
        if (ScreenCandidate(facts) != CandidateSkip_None)
        {
            continue;
        }

        // A bystander's question first: no id, no clock, no roll.
        Verdict verdict = EvaluatePeek(npc, NULL);
        const SubmissionPeek peek =
            JudgePeek(verdict.reasonKey, verdict.willJoin, verdict.rolled, verdict.chancePercent);
        if (peek == Peek_Skip)
        {
            continue;
        }
        if (peek == Peek_Roll)
        {
            // The day's draw, once, with the key: the same roll Hablar would make.
            verdict = Evaluate(npc);
            if (!verdict.willJoin)
            {
                CharKey key;
                if (SafeCharKey(npc, &key))
                {
                    NoteSubmissionRollFailed(key, day);
                }
                Log(CharacterName(npc) + Fmt(": field yield not today (%.0f%%, day %.0f)",
                                             (double)verdict.chancePercent, (double)day));
                continue;
            }
        }

        if (Start(npc, leader, defeats, verdict))
        {
            return;
        }
    }
}
} // namespace

void SubmissionTick()
{
    AdvanceClock();

    switch (g_event.state)
    {
    case Submission_Approaching: StepApproach(); break;
    case Submission_Holding:     StepHold(); break;
    case Submission_Forgiven:    StepForgiven(); break;
    default: break;
    }

    if (g_event.state != Submission_Idle)
    {
        return;
    }
    const unsigned int now = GetTickCount();
    if (now < g_nextSweep)
    {
        return;
    }
    g_nextSweep = now + kSubmissionSweepMs;
    Sweep();
}

bool IsSubmissionPending(Character* npc)
{
    CharKey key;
    return npc != NULL && g_onScreen.HasPending() && SafeFindCharKey(npc, &key) &&
           g_onScreen.IsPending(key);
}

void ForgetSubmissionOffer()
{
    g_onScreen.Clear();
}

namespace
{
// An answer counts only for the NPC whose popup it was, while it is up.
bool AnswersTheEvent(Character* npc, const char* answer)
{
    CharKey key;
    const bool ours = g_event.state == Submission_Holding && npc != NULL &&
                      SafeFindCharKey(npc, &key) && key == g_event.key;
    if (!ours)
    {
        Log(std::string(answer) + " ignored: no submission in progress for " + CharacterName(npc));
    }
    return ours;
}
} // namespace

void AcceptSubmission(Character* npc)
{
    if (AnswersTheEvent(npc, "accept"))
    {
        Apply(Outcome_Accepted);
    }
}

void ForgiveSubmission(Character* npc)
{
    if (AnswersTheEvent(npc, "forgive"))
    {
        Apply(Outcome_Forgiven);
    }
}

void RejectSubmission(Character* npc)
{
    if (AnswersTheEvent(npc, "reject"))
    {
        Apply(Outcome_Rejected);
    }
}

void DismissSubmission(Character* npc)
{
    if (AnswersTheEvent(npc, "dismiss"))
    {
        Apply(Outcome_Dismissed);
    }
}

void ForgetSubmission()
{
    if (g_event.state != Submission_Idle)
    {
        Apply(Outcome_Unloaded);
    }
    else
    {
        NpcMotion::EndRun(NULL);
    }
    ResetEvent();
    g_clock = 0.0f;
    g_haveLastTick = false;
    g_nextSweep = 0;
}
} // namespace totalrecruit
