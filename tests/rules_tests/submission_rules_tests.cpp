// Tests for src/approach_rules.cpp and src/submission_rules.cpp: the walk-up of
// a submitted NPC (ported from TotalSparring's ApproachStep), the hold while the
// popup is up, the forgive window, the trigger, the calm, the restore plans, the
// flight and the reject cooldown.

#include "test_support.h"
#include "submission_rules.h"

#include <cmath>

using namespace totalrecruit;

namespace
{
ApproachInput Walking(float distance, float elapsed, float sinceRepath)
{
    ApproachInput in;
    in.npcValid = true;
    in.targetValid = true;
    in.distanceMeters = distance;
    in.elapsedSec = elapsed;
    in.sinceLastRepathSec = sinceRepath;
    return in;
}

bool Near(float a, float b)
{
    return std::fabs(a - b) < 0.01f;
}

// ------------------------------------------------------------------ Approach
void ApproachOrderMatchesTheReference()
{
    const char* name = "approach: invalid, combat, arrive, too far, timeout, repath, keep";
    const ApproachLimits limits = SubmissionApproachLimits(60.0f);

    ApproachInput in = Walking(2.0f, 0.0f, 0.0f);
    in.npcValid = false;
    CHECK(ApproachStep(in, limits).kind == Approach_Abort);
    CHECK(ApproachStep(in, limits).reason == ApproachAbort_Invalid);

    in = Walking(2.0f, 0.0f, 0.0f);
    in.targetValid = false;
    CHECK(ApproachStep(in, limits).reason == ApproachAbort_Invalid);

    in = Walking(2.0f, 0.0f, 0.0f);
    in.combatStarted = true;
    CHECK(ApproachStep(in, limits).reason == ApproachAbort_Combat);

    CHECK(ApproachStep(Walking(3.0f, 0.0f, 0.0f), limits).kind == Approach_Arrive);
    CHECK(ApproachStep(Walking(121.0f, 0.0f, 0.0f), limits).reason == ApproachAbort_TooFar);
    CHECK(ApproachStep(Walking(-1.0f, 0.0f, 0.0f), limits).reason == ApproachAbort_TooFar);
    CHECK(ApproachStep(Walking(10.0f, 60.0f, 0.0f), limits).reason == ApproachAbort_Timeout);
    CHECK(ApproachStep(Walking(10.0f, 5.0f, 1.0f), limits).kind == Approach_Repath);
    CHECK(ApproachStep(Walking(10.0f, 5.0f, 0.5f), limits).kind == Approach_KeepWalking);
}

void ArrivingWinsOverATimeoutOnTheSameFrame()
{
    const char* name = "approach: arriving on the timeout frame still asks";
    const ApproachLimits limits = SubmissionApproachLimits(60.0f);
    CHECK(ApproachStep(Walking(2.5f, 61.0f, 0.0f), limits).kind == Approach_Arrive);
}

void ABusyWindowMakesTheNpcWaitNotAbort()
{
    const char* name = "approach: arrived while a popup is up -> stand and wait, then time out";
    const ApproachLimits limits = SubmissionApproachLimits(60.0f);
    ApproachInput in = Walking(2.0f, 10.0f, 1.0f);
    in.targetBusy = true;
    CHECK(ApproachStep(in, limits).kind == Approach_Stand);
    in.sinceLastRepathSec = 0.2f;
    CHECK(ApproachStep(in, limits).kind == Approach_KeepWalking);
    in.elapsedSec = 60.0f;
    CHECK(ApproachStep(in, limits).reason == ApproachAbort_Timeout);

    // Still far: a busy window changes nothing about the walk.
    in = Walking(20.0f, 10.0f, 1.0f);
    in.targetBusy = true;
    CHECK(ApproachStep(in, limits).kind == Approach_Repath);
}

void TheApproachWindowIsTheTimeout()
{
    const char* name = "approach: the timeout is submissionApproachSeconds, metres are 10 units";
    const ApproachLimits limits = SubmissionApproachLimits(30.0f);
    CHECK(Near(limits.timeoutSec, 30.0f));
    CHECK(Near(limits.arriveMeters, kSubmissionArriveMeters));
    CHECK(Near(limits.abortDistanceMeters, kSubmissionAbortMeters));
    CHECK(Near(WorldUnitsToMeters(30.0f), 3.0f));
    CHECK(Near(MetersToWorldUnits(80.0f), 800.0f));
    CHECK(ApproachAbortText(ApproachAbort_Timeout)[0] != '\0');
}

// --------------------------------------------------------------------- Clock
void TheClockStopsWhilePausedAndCapsAHitch()
{
    const char* name = "clock: real seconds, frozen on pause, one frame adds at most 1 s";
    CHECK(Near(AdvanceEventClock(10.0f, 250, false), 10.25f));
    CHECK(Near(AdvanceEventClock(10.0f, 250, true), 10.0f));
    CHECK(Near(AdvanceEventClock(10.0f, 15000, false), 11.0f));
    CHECK(Near(AdvanceEventClock(0.0f, 0, false), 0.0f));
}

// ------------------------------------------------------------------- Trigger
CandidateFacts Ready()
{
    CandidateFacts f;
    f.eventEnabled = true;
    f.recruitable = true;
    f.up = true;
    f.mayAsk = true;
    f.beaten = true;
    return f;
}

void ACandidateIsScreenedCheapestFirst()
{
    const char* name = "trigger: disabled, busy, mine, excluded, down, caged, cooldown, asked, rolled, unbeaten";
    CHECK(ScreenCandidate(Ready()) == CandidateSkip_None);

    CandidateFacts f = Ready();
    f.eventEnabled = false;
    CHECK(ScreenCandidate(f) == CandidateSkip_Disabled);
    f = Ready(); f.eventRunning = true;
    CHECK(ScreenCandidate(f) == CandidateSkip_Busy);
    f = Ready(); f.popupOpen = true;
    CHECK(ScreenCandidate(f) == CandidateSkip_Busy);
    f = Ready(); f.mine = true;
    CHECK(ScreenCandidate(f) == CandidateSkip_Mine);
    f = Ready(); f.recruitable = false;
    CHECK(ScreenCandidate(f) == CandidateSkip_Excluded);
    f = Ready(); f.up = false;
    CHECK(ScreenCandidate(f) == CandidateSkip_NotUp);
    f = Ready(); f.caged = true;
    CHECK(ScreenCandidate(f) == CandidateSkip_Caged);
    f = Ready(); f.onRejectCooldown = true;
    CHECK(ScreenCandidate(f) == CandidateSkip_Cooldown);
    f = Ready(); f.mayAsk = false;
    CHECK(ScreenCandidate(f) == CandidateSkip_AlreadyAsked);
    f = Ready(); f.rolledNoToday = true;
    CHECK(ScreenCandidate(f) == CandidateSkip_RolledNoToday);
    f = Ready(); f.beaten = false;
    CHECK(ScreenCandidate(f) == CandidateSkip_NotBeaten);
    CHECK(CandidateSkipText(CandidateSkip_Cooldown)[0] != '\0');
}

void ThePeekFiresRollsOrSkips()
{
    const char* name = "trigger: surrender fires, an open field yield rolls once, a no skips";
    CHECK(JudgePeek(Reason_Surrendered, true, false, 0) == Peek_Fire);
    CHECK(JudgePeek(Reason_FieldYieldJoins, true, true, 60) == Peek_Fire);
    CHECK(JudgePeek(Reason_FieldYieldNotYet, false, true, 41) == Peek_Roll);
    CHECK(JudgePeek(Reason_FieldYieldNotYet, false, true, 0) == Peek_Skip);
    CHECK(JudgePeek(Reason_FieldYieldNotYet, false, false, 41) == Peek_Skip);
    CHECK(JudgePeek(Reason_InOthersCage, false, false, 0) == Peek_Skip);
    CHECK(JudgePeek(Reason_Insistence, false, false, 0) == Peek_Skip);
    CHECK(JudgePeek(Reason_CatsAsk, false, true, 50) == Peek_Skip);
    CHECK(JudgePeek(Reason_RejectCooldown, false, false, 0) == Peek_Skip);
}

// proactive-offer P1: a requested offer (a won duel, a heal) is the gesture itself, so
// the checks that stand for "beaten and not asked since" do not apply; what still
// applies (the event, mine, excluded, up, caged) does.
void ARequestedOfferSkipsTheDefeatChecks()
{
    const char* name = "trigger: a requested offer skips cooldown, asked, rolled and unbeaten";
    CandidateFacts f = Ready();
    f.requested = true;
    f.onRejectCooldown = true;
    f.mayAsk = false;
    f.rolledNoToday = true;
    f.beaten = false;
    CHECK(ScreenCandidate(f) == CandidateSkip_None);
    f.up = false;
    CHECK(ScreenCandidate(f) == CandidateSkip_NotUp);
    f.up = true; f.caged = true;
    CHECK(ScreenCandidate(f) == CandidateSkip_Caged);
    f.caged = false; f.mine = true;
    CHECK(ScreenCandidate(f) == CandidateSkip_Mine);
    f.mine = false; f.recruitable = false;
    CHECK(ScreenCandidate(f) == CandidateSkip_Excluded);
    f.recruitable = true; f.popupOpen = true;
    CHECK(ScreenCandidate(f) == CandidateSkip_Busy);
}

// animal-duel-submission / proactive-offer: Cheat takes a free animal with one defeat
// (DecideCheat), Balanced the same break count as a person.
void AnAnimalInCheatIsBeatenAfterOneDefeat()
{
    const char* name = "trigger: beaten = defeats at the break count (Cheat animal: one) or yielded";
    CHECK(SubmissionBeaten(2, 2, false, false, false));
    CHECK(!SubmissionBeaten(1, 2, false, false, false));
    CHECK(SubmissionBeaten(0, 2, true, false, false));
    CHECK(!SubmissionBeaten(1, 2, false, true, false));
    CHECK(SubmissionBeaten(2, 2, false, true, false));
    CHECK(SubmissionBeaten(1, 2, false, true, true));
    CHECK(!SubmissionBeaten(0, 2, false, true, true));
    // A person in Cheat keeps the break count.
    CHECK(!SubmissionBeaten(1, 2, false, false, true));
}

void TheAnimalVerdictsFireOrRoll()
{
    const char* name = "trigger: an animal's yield fires or rolls like a person's; Cheat's defeat fires";
    CHECK(JudgePeek(Reason_AnimalYieldJoins, true, true, 38) == Peek_Fire);
    CHECK(JudgePeek(Reason_AnimalYieldNotYet, false, true, 38) == Peek_Roll);
    CHECK(JudgePeek(Reason_AnimalYieldNotYet, false, true, 0) == Peek_Skip);
    CHECK(JudgePeek(Reason_CheatAnimalDefeated, true, false, 0) == Peek_Fire);
    CHECK(JudgePeek(Reason_CheatAnimalDefeated, false, false, 0) == Peek_Skip);
    CHECK(JudgePeek(Reason_TameNotFed, false, false, 0) == Peek_Skip);
}

void AnNpcAsksOnceUntilBeatenAgain()
{
    const char* name = "trigger: asked once; only a new defeat opens the question again";
    CHECK(MayAskAgain(false, 0, 0));
    CHECK(!MayAskAgain(true, 3, 3));
    CHECK(!MayAskAgain(true, 3, 2));
    CHECK(MayAskAgain(true, 3, 4));
}

// ---------------------------------------------------------------------- Calm
void TheCalmBreaksAfterThreeStrikesInARow()
{
    const char* name = "calm: three re-asserts in a row in combat break it; one calm one resets";
    int strikes = 0;
    strikes = NextCombatStrikes(strikes, true);
    strikes = NextCombatStrikes(strikes, true);
    CHECK(!CalmBroken(strikes));
    CHECK(NextCombatStrikes(strikes, false) == 0);
    strikes = NextCombatStrikes(strikes, true);
    CHECK(CalmBroken(strikes));
}

// ---------------------------------------------------------------------- Hold
HoldInput Holding()
{
    HoldInput in;
    in.npcValid = true;
    in.leaderValid = true;
    in.popupStillOurs = true;
    return in;
}

void TheHoldLastsWhileThePopupIsOurs()
{
    const char* name = "hold: invalid, combat, closed, reassert each second, keep";
    HoldInput in = Holding();
    CHECK(HoldStep(in).kind == Hold_Keep);
    in.sinceLastReassertSec = 1.0f;
    CHECK(HoldStep(in).kind == Hold_Reassert);

    in = Holding(); in.npcValid = false;
    CHECK(HoldStep(in).reason == HoldRelease_Invalid);
    in = Holding(); in.leaderValid = false;
    CHECK(HoldStep(in).reason == HoldRelease_Invalid);
    in = Holding(); in.calmBroken = true;
    CHECK(HoldStep(in).reason == HoldRelease_Combat);
    in = Holding(); in.popupStillOurs = false;
    CHECK(HoldStep(in).kind == Hold_Release);
    CHECK(HoldStep(in).reason == HoldRelease_Closed);
    CHECK(HoldReleaseText(HoldRelease_Closed)[0] != '\0');
}

// ------------------------------------------------------------------ Forgiven
ForgivenInput Forgiving(float elapsed)
{
    ForgivenInput in;
    in.npcValid = true;
    in.elapsedSec = elapsed;
    in.windowSec = 300.0f;
    return in;
}

void TheForgiveWindowEndsOnTimeOrEarlier()
{
    const char* name = "forgiven: joined, invalid, combat, time up, reassert, keep";
    CHECK(ForgivenStep(Forgiving(10.0f)).kind == Forgiven_Keep);
    ForgivenInput in = Forgiving(10.0f);
    in.sinceLastReassertSec = 1.0f;
    CHECK(ForgivenStep(in).kind == Forgiven_Reassert);
    CHECK(ForgivenStep(Forgiving(300.0f)).reason == ForgivenRelease_TimeUp);
    in = Forgiving(10.0f); in.joined = true;
    CHECK(ForgivenStep(in).reason == ForgivenRelease_Joined);
    in = Forgiving(10.0f); in.npcValid = false;
    CHECK(ForgivenStep(in).reason == ForgivenRelease_Invalid);
    in = Forgiving(10.0f); in.calmBroken = true;
    CHECK(ForgivenStep(in).reason == ForgivenRelease_Combat);
    // Recruited on the last second: joined, not "time up" (no hostility put back).
    in = Forgiving(400.0f); in.joined = true;
    CHECK(ForgivenStep(in).reason == ForgivenRelease_Joined);
    CHECK(ForgivenReleaseText(ForgivenRelease_TimeUp)[0] != '\0');
}

// ------------------------------------------------------------------ Restore
void AcceptRecruitsAndRestoresTheSpeed()
{
    const char* name = "plan: accept joins them, with their pace back and no hostility";
    const RestorePlan p = PlanFor(Outcome_Accepted, true, true);
    CHECK(p.next == Submission_Done);
    CHECK(p.recruit && p.restoreSpeed && p.resumeRoutine);
    CHECK(!p.markHostile && !p.clearDefeats && !p.startCooldown && !p.flee);
}

void ForgiveRunsOffAndTheWindowCloseStaysPut()
{
    const char* name = "plan: forgive runs off calm; closing the window stays calm in place";
    RestorePlan p = PlanFor(Outcome_Forgiven, true, true);
    CHECK(p.next == Submission_Forgiven && p.flee);
    CHECK(!p.markHostile && !p.recruit && !p.clearDefeats && !p.restoreSpeed);
    p = PlanFor(Outcome_Dismissed, true, true);
    CHECK(p.next == Submission_Forgiven && !p.flee);
    CHECK(!p.markHostile && !p.recruit);
    CHECK(Near(ForgivenWindowSec(Outcome_Forgiven, 300.0f, 20.0f), 300.0f));
    CHECK(Near(ForgivenWindowSec(Outcome_Dismissed, 300.0f, 20.0f), 300.0f));
}

// proactive-offer: a bad roll after a gesture opens no window; the NPC stops attacking
// where it stands for offerFailCalmSeconds, then goes back to what it was.
void ABadRollCalmsInPlaceForItsOwnWindow()
{
    const char* name = "plan: a failed roll calms in place for failCalmSeconds, no flight, no window";
    const RestorePlan p = PlanFor(Outcome_RollFailed, true, true);
    CHECK(p.next == Submission_Forgiven && !p.flee);
    CHECK(!p.markHostile && !p.recruit && !p.clearDefeats && !p.startCooldown);
    CHECK(Near(ForgivenWindowSec(Outcome_RollFailed, 300.0f, 20.0f), 20.0f));
    CHECK(SubmissionOutcomeName(Outcome_RollFailed)[0] != '\0');
}

void TheForgiveWindowEndingPutsBackWhatWasThere()
{
    const char* name = "plan: after the window a hostile NPC is hostile again, a calm one stays calm";
    RestorePlan p = PlanFor(Outcome_ForgiveOver, true, true);
    CHECK(p.next == Submission_Done && p.restoreSpeed && p.resumeRoutine && p.markHostile);
    p = PlanFor(Outcome_ForgiveOver, false, true);
    CHECK(p.resumeRoutine && !p.markHostile);
    p = PlanFor(Outcome_ForgiveOver, true, false);
    CHECK(!p.resumeRoutine && !p.markHostile && p.restoreSpeed);
}

// proactive-offer (user request): "rechazar no hace nada: que el npc o animal haga lo
// que estaba haciendo, limpiar todos los flags y dismiss el popup".
void RejectOnlyPutsBackWhatWasThere()
{
    const char* name = "plan: reject does nothing but end the event: no wipe, no cooldown, hostile only if it was";
    RestorePlan p = PlanFor(Outcome_Rejected, false, true);
    CHECK(p.next == Submission_Done);
    CHECK(p.resumeRoutine && p.restoreSpeed);
    CHECK(!p.clearDefeats && !p.startCooldown && !p.markHostile);
    CHECK(!p.recruit && !p.flee);
    p = PlanFor(Outcome_Rejected, true, true);
    CHECK(p.markHostile && !p.clearDefeats && !p.startCooldown);
    p = PlanFor(Outcome_Rejected, true, false);
    CHECK(!p.markHostile && !p.resumeRoutine && p.restoreSpeed);
}

void AbortsAndLoadsOnlyGiveBack()
{
    const char* name = "plan: an abort restores what was there; a load or a fight only the pace";
    RestorePlan p = PlanFor(Outcome_Aborted, true, true);
    CHECK(p.next == Submission_Done && p.restoreSpeed && p.resumeRoutine && p.markHostile);
    CHECK(!p.clearDefeats && !p.startCooldown && !p.recruit);
    p = PlanFor(Outcome_Aborted, true, false);
    CHECK(!p.resumeRoutine && !p.markHostile);
    p = PlanFor(Outcome_Unloaded, true, true);
    CHECK(p.restoreSpeed && p.resumeRoutine && !p.markHostile);
    p = PlanFor(Outcome_CombatTookOver, true, true);
    CHECK(p.restoreSpeed && !p.resumeRoutine && !p.markHostile);
    p = PlanFor(Outcome_JoinedLater, true, true);
    CHECK(p.restoreSpeed && !p.resumeRoutine && !p.markHostile && !p.recruit);
    CHECK(SubmissionOutcomeName(Outcome_Rejected)[0] != '\0');
    CHECK(SubmissionStateName(Submission_Forgiven)[0] != '\0');
}

// --------------------------------------------------------------------- Flight
void TheFlightGoesStraightAwayOnTheGround()
{
    const char* name = "flight: away from the player on the ground plane, height kept";
    GroundPoint p = FleePoint(GroundPoint(100.0f, 5.0f, 0.0f), GroundPoint(0.0f, 0.0f, 0.0f), 800.0f);
    CHECK(Near(p.x, 900.0f) && Near(p.z, 0.0f) && Near(p.y, 5.0f));
    p = FleePoint(GroundPoint(0.0f, 0.0f, 30.0f), GroundPoint(0.0f, 50.0f, 0.0f), 800.0f);
    CHECK(Near(p.x, 0.0f) && Near(p.z, 830.0f) && Near(p.y, 0.0f));
    p = FleePoint(GroundPoint(3.0f, 0.0f, 4.0f), GroundPoint(0.0f, 0.0f, 0.0f), 10.0f);
    CHECK(Near(p.x, 9.0f) && Near(p.z, 12.0f));
    // Same spot: runs along +X rather than nowhere.
    p = FleePoint(GroundPoint(7.0f, 1.0f, 7.0f), GroundPoint(7.0f, 9.0f, 7.0f), 800.0f);
    CHECK(Near(p.x, 807.0f) && Near(p.z, 7.0f));
}

// ------------------------------------------------------------------ Cooldown
void TheCooldownEndsOnAWholeHour()
{
    const char* name = "cooldown: ends on the next whole hour after now + hours; 0 hours = none";
    CHECK(RejectCooldownUntilHour(100.0, 72.0f) == 172);
    CHECK(RejectCooldownUntilHour(100.2, 72.0f) == 173);
    CHECK(RejectCooldownUntilHour(100.0, 0.0f) == 0);
    CHECK(RejectCooldownUntilHour(100.0, -5.0f) == 0);
}

void TheCooldownRunsUntilItsHour()
{
    const char* name = "cooldown: runs before the end hour, with the hours left; none at 0";
    double left = -1.0;
    CHECK(RejectCooldownActive(172, 100.0, &left));
    CHECK(Near((float)left, 72.0f));
    CHECK(!RejectCooldownActive(172, 172.0, &left));
    CHECK(Near((float)left, 0.0f));
    CHECK(!RejectCooldownActive(0, 0.0, &left));
    CHECK(!RejectCooldownActive(172, 500.0, NULL));
    CHECK(SanitiseSavedCooldown(-4) == 0);
    CHECK(SanitiseSavedCooldown(172) == 172);
}

void TheCooldownRefusesEveryRoute()
{
    const char* name = "cooldown: one predicate refuses with its own reason; off leaves the verdict alone";
    Verdict v;
    v.willJoin = true;
    CHECK(!DecideRejectCooldown(false, &v));
    CHECK(v.willJoin);
    CHECK(DecideRejectCooldown(true, &v));
    CHECK(!v.willJoin);
    CHECK(v.currency == Currency_Refused);
    CHECK(v.reasonKey == Reason_RejectCooldown);
    CHECK(!v.rolled);
}
} // namespace

void RunSubmissionRulesTests()
{
    ApproachOrderMatchesTheReference();
    ArrivingWinsOverATimeoutOnTheSameFrame();
    ABusyWindowMakesTheNpcWaitNotAbort();
    TheApproachWindowIsTheTimeout();
    TheClockStopsWhilePausedAndCapsAHitch();
    ACandidateIsScreenedCheapestFirst();
    ThePeekFiresRollsOrSkips();
    ARequestedOfferSkipsTheDefeatChecks();
    AnAnimalInCheatIsBeatenAfterOneDefeat();
    TheAnimalVerdictsFireOrRoll();
    AnNpcAsksOnceUntilBeatenAgain();
    TheCalmBreaksAfterThreeStrikesInARow();
    TheHoldLastsWhileThePopupIsOurs();
    TheForgiveWindowEndsOnTimeOrEarlier();
    AcceptRecruitsAndRestoresTheSpeed();
    ForgiveRunsOffAndTheWindowCloseStaysPut();
    ABadRollCalmsInPlaceForItsOwnWindow();
    TheForgiveWindowEndingPutsBackWhatWasThere();
    RejectOnlyPutsBackWhatWasThere();
    AbortsAndLoadsOnlyGiveBack();
    TheFlightGoesStraightAwayOnTheGround();
    TheCooldownEndsOnAWholeHour();
    TheCooldownRunsUntilItsHour();
    TheCooldownRefusesEveryRoute();
}
