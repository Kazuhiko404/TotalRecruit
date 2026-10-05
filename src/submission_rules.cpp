#include "submission_rules.h"

#include <cmath>

namespace totalrecruit
{
CandidateFacts::CandidateFacts()
    : eventEnabled(false), eventRunning(false), popupOpen(false), mine(false),
      recruitable(false), up(false), caged(false), onRejectCooldown(false), mayAsk(false),
      rolledNoToday(false), beaten(false), requested(false)
{
}

HoldInput::HoldInput()
    : npcValid(false), leaderValid(false), calmBroken(false), popupStillOurs(false),
      sinceLastReassertSec(0.0f)
{
}

ForgivenInput::ForgivenInput()
    : elapsedSec(0.0f), windowSec(0.0f), sinceLastReassertSec(0.0f), npcValid(false),
      joined(false), calmBroken(false)
{
}

RestorePlan::RestorePlan()
    : next(Submission_Done), flee(false), restoreSpeed(false), resumeRoutine(false),
      markHostile(false), clearDefeats(false), startCooldown(false), recruit(false)
{
}

const char* SubmissionStateName(SubmissionState state)
{
    switch (state)
    {
    case Submission_Idle:        return "idle";
    case Submission_Approaching: return "approaching";
    case Submission_Holding:     return "holding";
    case Submission_Forgiven:    return "forgiven";
    case Submission_Done:        return "done";
    }
    return "unknown";
}

// ------------------------------------------------------------------- Clock
// TotalSparring challenge_director.cpp AdvanceClock: the frame's wall time,
// capped, and nothing while paused. Not scaled by the game speed: the windows
// are the player's real seconds (an approach of 60 s is a minute to watch).
float AdvanceEventClock(float clockSec, unsigned int elapsedMs, bool paused)
{
    if (paused)
        return clockSec;
    float delta = static_cast<float>(elapsedMs) / 1000.0f;
    if (delta > kSubmissionMaxFrameSec)
        delta = kSubmissionMaxFrameSec;
    return clockSec + delta;
}

// ----------------------------------------------------------------- Trigger
CandidateSkip ScreenCandidate(const CandidateFacts& f)
{
    if (!f.eventEnabled) return CandidateSkip_Disabled;
    if (f.eventRunning || f.popupOpen) return CandidateSkip_Busy;
    if (f.mine) return CandidateSkip_Mine;
    if (!f.recruitable) return CandidateSkip_Excluded;
    if (!f.up) return CandidateSkip_NotUp;
    if (f.caged) return CandidateSkip_Caged;
    // A requested offer is the gesture itself: the verdict (Evaluate) decides the rest.
    if (f.requested) return CandidateSkip_None;
    if (f.onRejectCooldown) return CandidateSkip_Cooldown;
    if (!f.mayAsk) return CandidateSkip_AlreadyAsked;
    if (f.rolledNoToday) return CandidateSkip_RolledNoToday;
    if (!f.beaten) return CandidateSkip_NotBeaten;
    return CandidateSkip_None;
}

const char* CandidateSkipText(CandidateSkip skip)
{
    switch (skip)
    {
    case CandidateSkip_None:          return "candidate";
    case CandidateSkip_Disabled:      return "event disabled";
    case CandidateSkip_Busy:          return "busy";
    case CandidateSkip_Mine:          return "already the player's";
    case CandidateSkip_Excluded:      return "excluded";
    case CandidateSkip_NotUp:         return "not up";
    case CandidateSkip_Caged:         return "caged";
    case CandidateSkip_Cooldown:      return "reject cooldown";
    case CandidateSkip_AlreadyAsked:  return "already asked";
    case CandidateSkip_RolledNoToday: return "roll failed today";
    case CandidateSkip_NotBeaten:     return "not beaten";
    }
    return "unknown";
}

bool SubmissionBeaten(int defeats, int defeatsToBreak, bool yielded, bool animal, bool cheat)
{
    if (yielded)
        return true;
    const int needed = (animal && cheat) ? 1 : defeatsToBreak;
    return defeats >= needed;
}

SubmissionPeek JudgePeek(ReasonKey reason, bool willJoin, bool rolled, int chancePercent)
{
    if (reason == Reason_Surrendered && willJoin)
        return Peek_Fire;
    if (reason == Reason_FieldYieldJoins && willJoin)
        return Peek_Fire;
    if ((reason == Reason_AnimalYieldJoins || reason == Reason_CheatAnimalDefeated) && willJoin)
        return Peek_Fire;
    // A peek supplies no roll key, so an open field yield always reads "not yet";
    // only a chance above 0 is worth one keyed draw.
    if ((reason == Reason_FieldYieldNotYet || reason == Reason_AnimalYieldNotYet) && rolled &&
        chancePercent > 0)
        return Peek_Roll;
    return Peek_Skip;
}

bool MayAskAgain(bool askedBefore, int defeatsWhenAsked, int defeatsNow)
{
    return !askedBefore || defeatsNow > defeatsWhenAsked;
}

// ---------------------------------------------------------------- The calm
int NextCombatStrikes(int strikes, bool inCombat)
{
    return inCombat ? strikes + 1 : 0;
}

bool CalmBroken(int strikes)
{
    return strikes >= kCalmStrikesToBreak;
}

// --------------------------------------------------------------- Approach
ApproachLimits SubmissionApproachLimits(float approachSeconds)
{
    ApproachLimits limits;
    limits.arriveMeters = kSubmissionArriveMeters;
    limits.abortDistanceMeters = kSubmissionAbortMeters;
    limits.timeoutSec = approachSeconds;
    return limits;
}

// --------------------------------------------------------------------- Hold
// TotalSparring challenge_hold HoldStep, without the bout and the time limit:
// this hold lasts while the popup it opened is ours.
HoldAction HoldStep(const HoldInput& in)
{
    HoldAction a;
    a.kind = Hold_Release;
    if (!in.npcValid || !in.leaderValid)
    {
        a.reason = HoldRelease_Invalid;
        return a;
    }
    if (in.calmBroken)
    {
        a.reason = HoldRelease_Combat;
        return a;
    }
    if (!in.popupStillOurs)
    {
        a.reason = HoldRelease_Closed;
        return a;
    }
    a.reason = HoldRelease_None;
    a.kind = in.sinceLastReassertSec >= kSubmissionReassertSec ? Hold_Reassert : Hold_Keep;
    return a;
}

const char* HoldReleaseText(HoldReleaseReason reason)
{
    switch (reason)
    {
    case HoldRelease_None:    return "none";
    case HoldRelease_Invalid: return "invalid character";
    case HoldRelease_Combat:  return "combat";
    case HoldRelease_Closed:  return "popup closed without an answer";
    }
    return "unknown";
}

// ----------------------------------------------------------------- Forgiven
ForgivenAction ForgivenStep(const ForgivenInput& in)
{
    ForgivenAction a;
    a.kind = Forgiven_Release;
    if (in.joined)
    {
        a.reason = ForgivenRelease_Joined;
        return a;
    }
    if (!in.npcValid)
    {
        a.reason = ForgivenRelease_Invalid;
        return a;
    }
    if (in.calmBroken)
    {
        a.reason = ForgivenRelease_Combat;
        return a;
    }
    if (in.elapsedSec >= in.windowSec)
    {
        a.reason = ForgivenRelease_TimeUp;
        return a;
    }
    a.reason = ForgivenRelease_None;
    a.kind = in.sinceLastReassertSec >= kSubmissionReassertSec ? Forgiven_Reassert : Forgiven_Keep;
    return a;
}

const char* ForgivenReleaseText(ForgivenReleaseReason reason)
{
    switch (reason)
    {
    case ForgivenRelease_None:    return "none";
    case ForgivenRelease_Joined:  return "joined the player";
    case ForgivenRelease_Invalid: return "invalid character";
    case ForgivenRelease_Combat:  return "combat";
    case ForgivenRelease_TimeUp:  return "time up";
    }
    return "unknown";
}

// ------------------------------------------------------ Outcomes and restore
const char* SubmissionOutcomeName(SubmissionOutcome outcome)
{
    switch (outcome)
    {
    case Outcome_Accepted:       return "accepted";
    case Outcome_Forgiven:       return "forgiven";
    case Outcome_Dismissed:      return "dismissed";
    case Outcome_Rejected:       return "rejected";
    case Outcome_Aborted:        return "aborted";
    case Outcome_ForgiveOver:    return "forgive window over";
    case Outcome_JoinedLater:    return "joined later";
    case Outcome_CombatTookOver: return "combat took over";
    case Outcome_Unloaded:       return "unloaded";
    case Outcome_RollFailed:     return "roll failed";
    }
    return "unknown";
}

RestorePlan PlanFor(SubmissionOutcome outcome, bool wasHostile, bool npcUp)
{
    RestorePlan p;
    switch (outcome)
    {
    case Outcome_Accepted:
        p.restoreSpeed = true;
        p.resumeRoutine = true;
        p.recruit = true;
        break;

    case Outcome_Forgiven:
        p.next = Submission_Forgiven;
        p.flee = true;
        break;

    case Outcome_Dismissed:
        p.next = Submission_Forgiven;
        break;

    case Outcome_RollFailed:
        p.next = Submission_Forgiven;
        break;

    // proactive-offer: RECHAZAR does nothing but end the event -- no defeats wiped, no
    // reject cooldown -- so it is put back exactly like an aborted walk-up.
    case Outcome_Rejected:
    case Outcome_Aborted:
    case Outcome_ForgiveOver:
        p.restoreSpeed = true;
        p.resumeRoutine = npcUp;
        p.markHostile = npcUp && wasHostile;
        break;

    case Outcome_Unloaded:
        p.restoreSpeed = true;
        p.resumeRoutine = npcUp;
        break;

    case Outcome_JoinedLater:
    case Outcome_CombatTookOver:
        p.restoreSpeed = true;
        break;
    }
    return p;
}

float ForgivenWindowSec(SubmissionOutcome entered, float forgiveSeconds, float failCalmSeconds)
{
    if (entered == Outcome_RollFailed)
        return failCalmSeconds;
    // Forgiven and dismissed get the same window: "treat the closed window like a
    // forgive without the flight" (cage-animals-submission T3).
    return forgiveSeconds;
}

// --------------------------------------------------------------- The flight
GroundPoint FleePoint(const GroundPoint& npc, const GroundPoint& player, float distanceUnits)
{
    float dx = npc.x - player.x;
    float dz = npc.z - player.z;
    const float length = std::sqrt(dx * dx + dz * dz);
    if (length < 0.001f)
    {
        dx = 1.0f;
        dz = 0.0f;
    }
    else
    {
        dx /= length;
        dz /= length;
    }
    return GroundPoint(npc.x + dx * distanceUnits, npc.y, npc.z + dz * distanceUnits);
}

// ------------------------------------------------------------ The cooldown
int RejectCooldownUntilHour(double nowHours, float cooldownHours)
{
    if (cooldownHours <= 0.0f)
        return 0;
    const double until = std::ceil(nowHours + static_cast<double>(cooldownHours) - 1e-6);
    return until < 1.0 ? 1 : static_cast<int>(until);
}

bool RejectCooldownActive(int untilHour, double nowHours, double* hoursLeft)
{
    if (hoursLeft != NULL)
        *hoursLeft = 0.0;
    if (untilHour <= 0 || nowHours >= static_cast<double>(untilHour))
        return false;
    if (hoursLeft != NULL)
        *hoursLeft = static_cast<double>(untilHour) - nowHours;
    return true;
}

int SanitiseSavedCooldown(int raw)
{
    return raw < 0 ? 0 : raw;
}

bool DecideRejectCooldown(bool onCooldown, Verdict* out)
{
    if (!onCooldown || out == NULL)
        return false;
    Verdict v;
    v.willJoin = false;
    v.currency = Currency_Refused;
    v.reasonKey = Reason_RejectCooldown;
    *out = v;
    return true;
}
} // namespace totalrecruit
