#include "common.h"
#include "submission_director.h"
#include "submission_rules.h"
#include "cooldown_minutes.h"
#include "wait_text.h"
#include "submission_tracker.h"
#include "approach_rules.h"
#include "approach_driver.h"
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
#include "head_notice.h"
#include "messages.h"
#include "reason_catalog.h"
#include "config.h"
#include "log.h"
#include "cheat_waits.h"
#include "offer_requests.h"
#include "offer_narration_rules.h"

#include <cstdio>
#include <cstring>

// The submission event's director: TotalSparring's challenge_director.cpp
// (state machine :91-96, paused-aware clock :50-85, walk-up :886-893, hold,
// reset :948) ported to "a beaten NPC asks to join". The pure steps live in
// submission_rules.h / approach_rules.h; the walk-up itself, its clock and the
// one-event slot in approach_driver.h (shared with the helper's offer); the engine
// calls in npc_motion.h (moving) and pacify.h (calm and hostility). This file
// reads, asks, acts, logs.
//
// proactive-offer: the sweep first serves the offers a gesture of the player asked
// for (offer_requests.h: a won duel, a heal), then the candidates it finds by
// defeats. Animals walk up too (a narrator tells it: offer_narration_rules.h), and a
// failed roll calms a hostile NPC in place for offerFailCalmSeconds (Outcome_RollFailed)
// instead of leaving it attacking.

namespace totalrecruit
{
namespace
{
const int kMaxSquad = 256;

// ----------------------------------------------------------------- Clock
EventClock g_clock;
unsigned int g_nextSweep = 0;

// ----------------------------------------------------------------- State
// One event at a time (and one walk-up across every director: approach_driver.h).
// The NPC, its key, the phase times and the calm strikes are the walker's.
struct Event
{
    SubmissionState state;
    WalkUp walk;
    bool animal;
    bool wasHostile;
    float windowSec;
    bool fleeing;
    bool fleeArrived;
    GroundPoint fleeTo;

    Event()
        : state(Submission_Idle), walk(), animal(false), wasHostile(false), windowSec(0.0f),
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

// The proactive offer's own steps, so QA can follow a gesture end to end.
void OfferLog(const std::string& line)
{
    LogInfo("[offer] " + line);
}

std::string Fmt(const char* format, double a, double b)
{
    char buffer[128] = { 0 };
    _snprintf_s(buffer, sizeof(buffer), _TRUNCATE, format, a, b);
    return std::string(buffer);
}

Character* EventNpc()
{
    return WalkUpNpc(g_event.walk);
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

bool IsAnimal(Character* npc)
{
    bool animal = false;
    return npc != NULL && SafeIsAnimal(npc, &animal) && animal;
}

// The calm, once a second (approach_driver.h ReassertCalm).
void Reassert(Character* npc)
{
    ReassertCalm(&g_event.walk, npc);
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
    ReleaseApproachSlot(ApproachOwner_Submission);
}

std::string Line(const char* key, const char* fallback, Character* npc, int hours)
{
    return Fill(Say(key, fallback, npc), CharacterName(npc), 0, hours, 0);
}

// A roller line of the event in this NPC's form (offer_narration_rules.h); {seconds}
// carries the calm window.
std::string Narration(OfferNarrationId id, Character* npc, bool animal, float seconds)
{
    const OfferNarration line = OfferNarrationFor(id, animal);
    std::string text = Line(line.key, line.fallback, npc, 0);
    char number[16] = { 0 };
    _snprintf_s(number, sizeof(number), _TRUNCATE, "%.0f", (double)seconds);
    const std::string key = "{seconds}";
    std::string::size_type at = 0;
    while ((at = text.find(key, at)) != std::string::npos)
    {
        text.replace(at, key.size(), number);
        at += std::strlen(number);
    }
    return text;
}

// ----------------------------------------------------------------- Outcomes
void EnterForgiven(Character* npc, SubmissionOutcome outcome, bool flee)
{
    const Config& config = GetConfig();
    g_event.state = Submission_Forgiven;
    g_event.walk.phaseStart = g_clock.Now();
    g_event.walk.lastReassert = g_clock.Now() - kSubmissionReassertSec;
    g_event.windowSec =
        ForgivenWindowSec(outcome, config.submission.forgiveSeconds, config.offer.failCalmSeconds);
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
    Log(g_event.walk.name + Fmt(": calm for %.0f s", g_event.windowSec, 0.0) +
        (flee ? (g_event.fleeing ? ", running off" : ", running off FAILED (stays)") : ", in place"));
    if (outcome == Outcome_RollFailed)
    {
        OfferLog("calm: " + g_event.walk.name + Fmt(" stops attacking for %.0f s in place",
                                                     g_event.windowSec, 0.0));
    }
}

// Acts on an outcome with the plan PlanFor gives it, then either enters the
// forgive window or ends the event.
void Apply(SubmissionOutcome outcome)
{
    Character* npc = EventNpc();
    const bool up = NpcMotion::IsUp(npc);
    Character* leader = Leader();
    const RestorePlan plan = PlanFor(outcome, g_event.wasHostile, up);
    const std::string name = g_event.walk.name;

    // Whatever the outcome (an aborted walk-up included), nobody is coming to talk.
    DismissHeadNotice(npc);

    Log(name + ": " + SubmissionOutcomeName(outcome) + " (from " +
        SubmissionStateName(g_event.state) + ", hostile before " +
        (g_event.wasHostile ? "yes" : "no") + ")");

    if (plan.next == Submission_Forgiven && npc != NULL)
    {
        if (outcome == Outcome_Forgiven)
        {
            if (OfferSpeaks(g_event.animal))
            {
                SpeakAloud(npc, Say("submissionEvent.forgiven", "Gracias... no lo olvidar\xc3\xa9.", npc));
            }
            TellPlayer(Narration(OfferNarration_IgnoredNotice, npc, g_event.animal, 0.0f), false);
            OfferLog("outcome: " + name + " ignored: runs off and stays calm");
        }
        else if (outcome == Outcome_RollFailed)
        {
            TellPlayer(Narration(OfferNarration_CalmNotice, npc, g_event.animal,
                                 GetConfig().offer.failCalmSeconds),
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
            until = CooldownUntilMinute(now, RejectCooldownHours(config));
            SetRejectCooldownUntil(key, until);
        }
        // What is really left, to the minute (cooldown_minutes.h): Cheat's minute
        // reads "1 min", a Balanced wait the whole hours it ends on.
        double left = 0.0;
        CooldownActiveAt(until, now, &left);
        Log(name + Fmt(": reject cooldown until game minute %.0f (%.2f h)", (double)until, left));
        SpeakAloud(npc, Say("submissionEvent.rejected", "Entonces nos volveremos a ver.", npc));
        int hours = 0;
        const std::string notice =
            SayWait("submissionEvent.rejectedNotice",
                    "{name}: lo rechazaste. No querr\xc3\xa1 saber nada de ti en {hours} h.",
                    "{name}: lo rechazaste. No querr\xc3\xa1 saber nada de ti en {minutes} min.",
                    npc, (float)left, &hours);
        TellPlayer(Fill(notice, CharacterName(npc), 0, hours, 0), false);
    }

    if (outcome == Outcome_Rejected)
    {
        OfferLog("outcome: " + name + " rejected: nothing changes (no defeats wiped, no cooldown" +
                 (plan.markHostile ? ", hostile again as before)" : ")"));
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
            TellPlayer(Narration(OfferNarration_JoinedNotice, npc, g_event.animal, 0.0f), false);
            Log(name + ": joined the squad");
            OfferLog("outcome: " + name + " accepted: joined the squad");
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
    // the way: read it again, so someone who cannot leave is never offered IGNORAR.
    bool caged = false;
    const bool inCage = SafeInPrison(npc, &caged) && caged;
    const OfferView view =
        g_event.animal ? BuildAnimalSubmissionOfferView(g_event.walk.name, inCage, OfferWording())
                       : BuildSubmissionOfferView(g_event.walk.name, inCage, OfferWording());
    if (!ShowOffer(npc, view))
    {
        return false;
    }
    // Marked only now that the window is up: opening it cleared any older mark.
    g_onScreen.Begin(g_event.walk.key);
    // The window names them: the "walks up to talk" notice has done its job.
    DismissHeadNotice(npc);
    return true;
}

void StepApproach()
{
    const Config& config = GetConfig();
    const ApproachAction action =
        StepWalkUp(&g_event.walk, g_clock.Now(),
                   SubmissionApproachLimits(config.submission.approachSeconds), "[submission]");
    Character* npc = EventNpc();
    switch (action.kind)
    {
    case Approach_KeepWalking:
    case Approach_Repath:
    case Approach_Stand:
        return;

    case Approach_Arrive:
        if (!OpenPopup(npc))
        {
            Log(g_event.walk.name + ": the popup could not open");
            Apply(Outcome_Aborted);
            return;
        }
        g_event.state = Submission_Holding;
        g_event.walk.phaseStart = g_clock.Now();
        g_event.walk.lastReassert = g_clock.Now();
        return;

    case Approach_Abort:
        Log(g_event.walk.name + ": approach aborted: " + action.text);
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
    in.calmBroken = CalmBroken(g_event.walk.strikes);
    in.popupStillOurs = IsOfferOpen() && g_onScreen.IsPending(g_event.walk.key);
    in.sinceLastReassertSec = g_clock.Now() - g_event.walk.lastReassert;

    const HoldAction action = HoldStep(in);
    switch (action.kind)
    {
    case Hold_Keep:
        return;

    case Hold_Reassert:
        Reassert(npc);
        NpcMotion::Stand(npc, leader);
        g_event.walk.lastReassert = g_clock.Now();
        return;

    case Hold_Release:
        Log(g_event.walk.name + ": hold released: " + HoldReleaseText(action.reason));
        if (action.reason == HoldRelease_Closed)
        {
            Apply(Outcome_Dismissed);
            return;
        }
        // The window must not outlive the event it asks about.
        if (g_onScreen.IsPending(g_event.walk.key))
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
    in.elapsedSec = g_clock.Now() - g_event.walk.phaseStart;
    in.windowSec = g_event.windowSec;
    in.sinceLastReassertSec = g_clock.Now() - g_event.walk.lastReassert;
    in.npcValid = NpcMotion::IsUp(npc);
    in.joined = IsMine(npc);
    in.calmBroken = CalmBroken(g_event.walk.strikes);

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
                Log(g_event.walk.name + ": ran off; stays calm");
            }
            else
            {
                NpcMotion::MoveToPoint(npc, g_event.fleeTo.x, g_event.fleeTo.y, g_event.fleeTo.z);
                NpcMotion::ReapplyRun(npc);
            }
        }
        g_event.walk.lastReassert = g_clock.Now();
        return;

    case Forgiven_Release:
        Log(g_event.walk.name + Fmt(": forgive window over after %.0f s (", in.elapsedSec, 0.0) +
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
    const Config& config = GetConfig();
    f->beaten = SubmissionBeaten(defeats, config.submission.defeatsToBreak, HasYieldedToPlayer(npc),
                                 IsAnimal(npc), config.preset == Preset_Cheat);
}

// `request` is the gesture that asked for this offer, or NULL for the defeats.
bool Start(Character* npc, Character* leader, int defeats, const Verdict& verdict,
           const OfferRequest* request)
{
    if (!ClaimApproachSlot(ApproachOwner_Submission))
    {
        return false;
    }
    g_event = Event();
    g_event.animal = IsAnimal(npc);
    // Read before the walk-up's calm clears it.
    g_event.wasHostile = HostileBefore(npc);
    bool moving = false;
    if (!BeginWalkUp(&g_event.walk, npc, leader, g_clock.Now(), &moving))
    {
        g_event = Event();
        ReleaseApproachSlot(ApproachOwner_Submission);
        return false;
    }
    const CharKey key = g_event.walk.key;
    MarkSubmissionAsked(key, defeats);
    g_event.state = Submission_Approaching;

    // An animal does not speak: no words and no notice over its head, a narrator line.
    if (OfferSpeaks(g_event.animal))
    {
        SpeakAloud(npc, Line("submissionEvent.approach",
                             "{name}: Espera... no quiero pelear m\xc3\xa1s.", npc, 0));
        RaiseHeadNotice(npc, Say("headNotice.approaching", "Se acerca para hablar contigo", npc));
    }
    TellPlayer(Narration(OfferNarration_ApproachNotice, npc, g_event.animal, 0.0f), false);

    float meters = -1.0f;
    NpcMotion::DistanceMeters(npc, leader, &meters);
    const std::string why = request != NULL
                                ? std::string("offer requested by ") + OfferTriggerName(request->trigger)
                                : ReasonMessageKey(verdict.reasonKey, "");
    Log(g_event.walk.name + " submitted (" + why + "), walks up" +
        Fmt(" from %.1f m, defeats %.0f", meters, (double)defeats) + "; hostile before " +
        (g_event.wasHostile ? "yes" : "no") + (g_event.animal ? "; animal" : "") +
        (moving ? "" : "; move order FAILED") + " [key " + key.ToString() + "]");
    if (request != NULL)
    {
        OfferLog("walk-up: " + g_event.walk.name + " comes to offer (" +
                 OfferTriggerName(request->trigger) + ")" + (g_event.animal ? " (animal)" : ""));
    }
    return true;
}

// A failed roll: no window, the NPC stops attacking where it stands for
// offerFailCalmSeconds (Outcome_RollFailed), then goes back to what it was.
bool StartCalm(Character* npc, const std::string& why)
{
    if (!ClaimApproachSlot(ApproachOwner_Submission))
    {
        return false;
    }
    g_event = Event();
    g_event.animal = IsAnimal(npc);
    g_event.wasHostile = HostileBefore(npc);
    if (!BeginWalkerInPlace(&g_event.walk, npc, g_clock.Now()))
    {
        g_event = Event();
        ReleaseApproachSlot(ApproachOwner_Submission);
        return false;
    }
    OfferLog("calm: " + g_event.walk.name + ": " + why + "; hostile before " +
             (g_event.wasHostile ? "yes" : "no") + (g_event.animal ? "; animal" : "") + " [key " +
             g_event.walk.key.ToString() + "]");
    Apply(Outcome_RollFailed);
    return true;
}

// One requested offer (offer_requests.h), screened already: its plan, then the walk-up
// or the calm. True when an event started (the request is then used up).
bool ServeRequest(Character* npc, Character* leader, const OfferRequest& request)
{
    const std::string name = CharacterName(npc);
    Verdict verdict;
    bool joinsFree = false;
    if (request.verdict == OfferVerdict_Roll)
    {
        // The same decision Hablar makes, with its key and its roll.
        verdict = Evaluate(npc);
        joinsFree = JoinsForFree(verdict.willJoin, verdict.currency, verdict.cats, verdict.temporary);
        OfferLog("roll: " + name + " -> " + (verdict.willJoin ? "joins" : "no") +
                 Fmt(" (%.0f%%)", (double)verdict.chancePercent, 0.0) + " " +
                 ReasonMessageKey(verdict.reasonKey, "") +
                 (verdict.willJoin && !joinsFree ? " (not free or not lasting: talk to it)" : ""));
        if (verdict.willJoin && verdict.viaRescue && GetConfig().rescueEnabled)
        {
            // The rescue route offers by itself when the patient gets up (rescue.cpp).
            ConsumeRequestedOffer(npc, "the rescue route offers when it wakes");
            return false;
        }
    }
    const bool hostile = HostileBefore(npc);
    const RequestedOfferPlan plan = PlanRequestedOffer(request.verdict, joinsFree, hostile);
    OfferLog("serve: " + name + " (" + OfferTriggerName(request.trigger) + ", " +
             OfferVerdictName(request.verdict) + ", hostile " + (hostile ? "yes" : "no") + ") -> " +
             RequestedPlanName(plan));
    if (plan == RequestedPlan_Nothing)
    {
        ConsumeRequestedOffer(npc, "a no from someone not hostile");
        return false;
    }
    const bool started = plan == RequestedPlan_WalkUp
                             ? Start(npc, leader, DefeatCount(npc), verdict, &request)
                             : StartCalm(npc, "the roll failed");
    if (started)
    {
        ConsumeRequestedOffer(npc, "served");
    }
    return started;
}

// The requested offers first: anyone with a pending request within the event's reach.
// A skip that can never clear (already the player's, excluded, caged) uses it up; one
// that can (down, busy) leaves it waiting.
bool SweepRequested(Character* leader, float lx, float ly, float lz)
{
    std::vector<Character*> nearby;
    if (!CharactersNear(Ogre::Vector3(lx, ly, lz), MetersToWorldUnits(kSubmissionAbortMeters),
                        &nearby))
    {
        return false;
    }
    for (unsigned int i = 0; i < nearby.size(); ++i)
    {
        Character* npc = nearby[i];
        OfferRequest request;
        if (npc == NULL || npc == leader || !FindRequestedOffer(npc, &request))
        {
            continue;
        }
        CandidateFacts facts = CheapFacts(npc);
        facts.requested = true;
        facts.recruitable = IsRecruitable(npc, NULL);
        const CandidateSkip skip = ScreenCandidate(facts);
        if (skip != CandidateSkip_None)
        {
            if (RequestSkipConsumes(skip))
            {
                ConsumeRequestedOffer(npc, CandidateSkipText(skip));
            }
            continue;
        }
        if (ServeRequest(npc, leader, request))
        {
            return true;
        }
    }
    return false;
}

void Sweep()
{
    const Config& config = GetConfig();
    const bool requests = HasRequestedOffers();
    if ((!config.submission.eventEnabled && !requests) || IsOfferOpen() ||
        !ApproachSlotFreeFor(ApproachOwner_Submission))
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

    // A gesture's request is served before anyone found by defeats.
    if (requests && SweepRequested(leader, lx, ly, lz))
    {
        return;
    }
    if (!config.submission.eventEnabled)
    {
        return;
    }

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
                // proactive-offer: a bad roll leaves it neutral for a while, not attacking.
                if (HostileBefore(npc) && StartCalm(npc, "the field yield roll failed"))
                {
                    return;
                }
                continue;
            }
        }

        if (Start(npc, leader, defeats, verdict, NULL))
        {
            return;
        }
    }
}
} // namespace

void SubmissionTick()
{
    g_clock.Advance();
    OfferRequestsTick();

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
                      SafeFindCharKey(npc, &key) && key == g_event.walk.key;
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
    g_clock.Reset();
    g_nextSweep = 0;
    ForgetOfferRequests();
}
} // namespace totalrecruit
