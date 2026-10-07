#pragma once

// The submission event, without the engine: a beaten NPC walks up to the
// player and asks to join, and the player answers Aceptar / Ignorar (was
// Perdonar) / Rechazar. Since proactive-offer it also carries the offers a
// gesture of the player asked for (a won duel, a heal: offer_request_rules.h),
// animals included, and a failed roll calms in place (Outcome_RollFailed).
//
// The shape is TotalSparring's NPC challenge (src/duel/challenge/
// challenge_director.cpp: Idle -> Approaching -> Holding, a paused-aware
// clock, a hold step), with one more state for the forgiven NPC that runs off
// and stays calm for a while:
//
//   Idle -> Approaching -> Holding (popup) -> Forgiven (fleeing, calm) -> Done
//              |               |                                  ^
//              +-- abort ------+--- accept / reject --------------+
//
// The director (submission_director.cpp) reads the engine into the plain inputs
// below, asks the step what to do and acts; nothing here touches the game, so
// every rule is tested in tests/rules_tests/submission_rules_tests.cpp.
//
// "Calm" is per character, never a faction change: the director clears the
// NPC's temporary-enemy marks toward the squad and ends its combat once a
// second while the event holds it (pacify.h SafeCalmToward). The faction
// relation is never touched.

#include "approach_rules.h"
#include "verdict.h"

namespace totalrecruit
{
enum SubmissionState
{
    Submission_Idle = 0,
    Submission_Approaching,   // walking up, calm, for at most the approach window
    Submission_Holding,       // standing in front of the player, the popup is up
    Submission_Forgiven,      // forgiven (or the window closed): calm for the forgive window
    Submission_Done           // over: the director restores and goes back to Idle
};

const char* SubmissionStateName(SubmissionState state);

// ---------------------------------------------------------------- Constants
// Who is looked at: characters within this of the squad leader.
const float kSubmissionTriggerMeters = 40.0f;
// The walk-up gives up past this, and asks at this.
const float kSubmissionAbortMeters = 120.0f;
const float kSubmissionArriveMeters = 3.0f;
// How far a forgiven NPC runs, away from the player.
const float kSubmissionFleeMeters = 80.0f;
// Calm, stance and run are re-asserted this often (event seconds).
const float kSubmissionReassertSec = 1.0f;
// The sweep for a newly submitted NPC runs this often (wall milliseconds).
const unsigned int kSubmissionSweepMs = 2000;
// One frame never adds more than this to the event clock: a hitch or a load
// screen is not time the player had (TotalSparring kMaxFrameSeconds).
const float kSubmissionMaxFrameSec = 1.0f;
// The calm is judged broken after this many re-asserts in a row found the NPC
// fighting anyway (about this many seconds).
const int kCalmStrikesToBreak = 3;

// ------------------------------------------------------------------- Clock
// The event clock: real seconds that stop while the game is paused. One frame
// adds at most kSubmissionMaxFrameSec.
float AdvanceEventClock(float clockSec, unsigned int elapsedMs, bool paused);

// ----------------------------------------------------------------- Trigger
// What the sweep knows about one nearby character before asking the decision
// engine anything (cheapest first).
struct CandidateFacts
{
    bool eventEnabled;      // submissionEventEnabled
    bool eventRunning;      // another submission is already in progress
    bool popupOpen;         // the recruit popup (or a mercy plea) is on screen
    bool mine;              // already the player's
    bool recruitable;       // the exclusion list allows them (IsRecruitable)
    bool up;                // alive, conscious, on their feet, not carried
    bool caged;             // in anyone's cage, shackled or carried off
    bool onRejectCooldown;  // the player rejected them and the cooldown runs
    bool mayAsk;            // not asked before, or beaten again since (MayAskAgain)
    bool rolledNoToday;     // their field-yield roll already failed today
    bool beaten;            // defeats >= the break count, or yielded to the player
    // proactive-offer: a gesture of the player (a won duel, a heal) asked for this
    // offer (offer_request_rules.h). The gesture stands for "beaten and not asked
    // since", so the cooldown, asked, rolled and beaten checks do not apply.
    bool requested;

    CandidateFacts();
};

enum CandidateSkip
{
    CandidateSkip_None = 0,   // worth a peek at the verdict
    CandidateSkip_Disabled,
    CandidateSkip_Busy,
    CandidateSkip_Mine,
    CandidateSkip_Excluded,
    CandidateSkip_NotUp,
    CandidateSkip_Caged,
    CandidateSkip_Cooldown,
    CandidateSkip_AlreadyAsked,
    CandidateSkip_RolledNoToday,
    CandidateSkip_NotBeaten
};

// PURE: why this character is not a candidate, in the order above, or
// CandidateSkip_None when the verdict is worth a peek. Animals are candidates too
// (proactive-offer): they walk up and the roller narrates for them.
CandidateSkip ScreenCandidate(const CandidateFacts& facts);

// PURE: whether `defeats` (or a yield to the player) make this NPC beaten enough to
// be asked: the break count, except a free animal in Cheat, which DecideCheat takes
// after one defeat (animal-duel-submission D4).
bool SubmissionBeaten(int defeats, int defeatsToBreak, bool yielded, bool animal, bool cheat);
const char* CandidateSkipText(CandidateSkip skip);

// What the peek at the verdict (rules.h EvaluatePeek: no roll key outside the
// rescue, no id minted, no clocks) says about firing the event.
enum SubmissionPeek
{
    Peek_Skip = 0,   // not submitted: no event
    Peek_Fire,       // submitted with no roll needed (surrender, or a joined field yield)
    Peek_Roll        // field yield that could go either way: ask once, with the key
};

// PURE: Reason_Surrendered (or a joining field yield) fires; a field yield
// that rolled with a chance above 0 and only lacked the key rolls; anything
// else (a refusal, an access no, 0%) skips. An animal's field yield
// (Reason_AnimalYield*) reads the same, and Cheat's defeated animal fires.
SubmissionPeek JudgePeek(ReasonKey reason, bool willJoin, bool rolled, int chancePercent);

// PURE: an NPC is asked once; after that only a new defeat (a higher count than
// when they were asked) opens the question again.
bool MayAskAgain(bool askedBefore, int defeatsWhenAsked, int defeatsNow);

// ---------------------------------------------------------------- The calm
// PURE: the strike count after one re-assert found the NPC in combat or not.
int NextCombatStrikes(int strikes, bool inCombat);
bool CalmBroken(int strikes);

// --------------------------------------------------------------- Approach
// The walk-up's limits for this event: arrive at kSubmissionArriveMeters,
// give up past kSubmissionAbortMeters or after `approachSeconds`.
ApproachLimits SubmissionApproachLimits(float approachSeconds);

// --------------------------------------------------------------------- Hold
// While the popup is up the NPC stands in front of the player (re-pinned and
// calmed once a second). It ends when the popup closes without an answer, the
// calm breaks, or someone can no longer be read. An answer ends it directly.
enum HoldActionKind
{
    Hold_Keep = 0,
    Hold_Reassert,
    Hold_Release
};

enum HoldReleaseReason
{
    HoldRelease_None = 0,
    HoldRelease_Invalid,   // the NPC or the leader can no longer be read / is down
    HoldRelease_Combat,    // the calm broke
    HoldRelease_Closed     // the popup closed with no answer (X, hidden, replaced)
};

struct HoldInput
{
    bool npcValid;
    bool leaderValid;
    bool calmBroken;
    bool popupStillOurs;
    float sinceLastReassertSec;

    HoldInput();
};

struct HoldAction
{
    HoldActionKind kind;
    HoldReleaseReason reason;
};

// PURE: Invalid, Combat, Closed, then Reassert every kSubmissionReassertSec.
HoldAction HoldStep(const HoldInput& in);
const char* HoldReleaseText(HoldReleaseReason reason);

// ----------------------------------------------------------------- Forgiven
// Forgiven (or let be): calm for the forgive window, running off first when
// forgiven. The player can still talk to them meanwhile.
enum ForgivenActionKind
{
    Forgiven_Keep = 0,
    Forgiven_Reassert,
    Forgiven_Release
};

enum ForgivenReleaseReason
{
    ForgivenRelease_None = 0,
    ForgivenRelease_Joined,    // the player recruited them meanwhile
    ForgivenRelease_Invalid,   // gone, dead or down
    ForgivenRelease_Combat,    // the calm broke: a fight owns them now
    ForgivenRelease_TimeUp
};

struct ForgivenInput
{
    float elapsedSec;
    float windowSec;
    float sinceLastReassertSec;
    bool npcValid;
    bool joined;
    bool calmBroken;

    ForgivenInput();
};

struct ForgivenAction
{
    ForgivenActionKind kind;
    ForgivenReleaseReason reason;
};

// PURE: Joined, Invalid, Combat, TimeUp, then Reassert every
// kSubmissionReassertSec.
ForgivenAction ForgivenStep(const ForgivenInput& in);
const char* ForgivenReleaseText(ForgivenReleaseReason reason);

// ------------------------------------------------------ Outcomes and restore
// How an event (or one of its phases) ended.
enum SubmissionOutcome
{
    Outcome_Accepted = 0,   // ACEPTAR: they join
    Outcome_Forgiven,       // IGNORAR (was PERDONAR): they run off, calm for the forgive window
    Outcome_Dismissed,      // the popup closed with no answer: calm for the window, in place
    Outcome_Rejected,       // RECHAZAR: nothing; back to what they were doing (proactive-offer)
    Outcome_Aborted,        // the walk-up or the hold gave up
    Outcome_ForgiveOver,    // the forgive window ran out
    Outcome_JoinedLater,    // recruited by hand during the forgive window
    Outcome_CombatTookOver, // the calm broke: the fight owns the NPC
    Outcome_Unloaded,       // a load began mid-event
    Outcome_RollFailed      // a gesture's roll failed: calm in place for failCalmSeconds, no window
};

const char* SubmissionOutcomeName(SubmissionOutcome outcome);

// What the director does to the NPC for an outcome.
struct RestorePlan
{
    SubmissionState next;   // Submission_Forgiven or Submission_Done
    bool flee;              // entering Forgiven: run kSubmissionFleeMeters away
    bool restoreSpeed;      // give back the speed order the run replaced
    bool resumeRoutine;     // clear the event's orders and ask the AI to re-think
    bool markHostile;       // the NPC treats the player leader as an enemy again
    bool clearDefeats;      // the player's defeats over them are wiped
    bool startCooldown;     // the reject cooldown starts
    bool recruit;           // they join the squad

    RestorePlan();
};

// PURE: the plan for `outcome`. `wasHostile` = the NPC was fighting or held a
// temporary-enemy mark toward the squad when the event began; `npcUp` = it can
// still be read and is on its feet. Restoring "what it was before" means only
// that: a hostile NPC is hostile again, a calm one stays calm.
RestorePlan PlanFor(SubmissionOutcome outcome, bool wasHostile, bool npcUp);

// PURE: the window the Forgiven state lasts, by how it was entered: a failed roll
// gets `failCalmSeconds` (offerFailCalmSeconds), anything else `forgiveSeconds`.
float ForgivenWindowSec(SubmissionOutcome entered, float forgiveSeconds, float failCalmSeconds);

// --------------------------------------------------------------- The flight
struct GroundPoint
{
    float x;
    float y;
    float z;

    GroundPoint() : x(0.0f), y(0.0f), z(0.0f) {}
    GroundPoint(float px, float py, float pz) : x(px), y(py), z(pz) {}
};

// PURE: `distanceUnits` from the NPC, straight away from the player on the
// ground plane (Y is up in Kenshi; the NPC's height is kept). Standing on the
// same spot, it runs along +X.
GroundPoint FleePoint(const GroundPoint& npc, const GroundPoint& player, float distanceUnits);

// ------------------------------------------------------------ The cooldown
// Whole-hour cooldown ends, for the helper's own reject book (helper.cpp, saved
// as "total_recruit.helper_reject_until"). The submission reject cooldown and the
// duel cooldown are kept to the minute instead (cooldown_minutes.h,
// priority-one B). 0 = none.

// PURE: the hour a cooldown started now ends on (rounded up); 0 when the
// cooldown is 0 hours or less.
int RejectCooldownUntilHour(double nowHours, float cooldownHours);

// A value read back from a save, made safe: negative is 0 (none).
int SanitiseSavedCooldown(int raw);

// PURE: the one recruit-route predicate. An NPC on the reject cooldown refuses
// every route with Reason_RejectCooldown ("no quiere saber nada de ti"); `out`
// is filled and true returned. False (and `out` untouched) otherwise.
bool DecideRejectCooldown(bool onCooldown, Verdict* out);
} // namespace totalrecruit
