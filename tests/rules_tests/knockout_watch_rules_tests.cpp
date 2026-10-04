// Tests for src/knockout_watch_rules: the knockout order acts once. The watch stays
// on after the order is taken until the prisoner is down, then the actor is let go;
// stealth is only turned off when the order turned it on; a stray knockout goal is
// only cleared when it cannot be a knockout the player still wants.

#include "test_support.h"
#include "knockout_watch_rules.h"

using namespace totalrecruit;

namespace
{
const int kIdle = 0;
const int kWalk = 5;
const int kKnockout = 228;

KnockoutWatchFacts Fresh()
{
    KnockoutWatchFacts f;
    f.phase = KnockoutPhase_AwaitingTake;
    f.taskBefore = kIdle;
    f.ordered = kKnockout;
    f.readTask = true;
    f.taskNow = kIdle;
    f.targetDown = false;
    f.targetUnreadable = false;
    f.elapsedMs = 0;
    return f;
}

void AnUntakenOrderWaitsThenFallsBack()
{
    const char* name = "knockout watch: unchanged task waits, then the fallback runs at the timeout";
    KnockoutWatchFacts f = Fresh();
    f.elapsedMs = 1499;
    CHECK(NextKnockoutWatchStep(f, 1500, 120000) == KnockoutStep_Wait);
    f.elapsedMs = 1500;
    CHECK(NextKnockoutWatchStep(f, 1500, 120000) == KnockoutStep_Fallback);
}

// The bug: the watch closed on "taken" and the goal it left behind kept the actor
// knocking the prisoner out every time he stood up.
void ATakenOrderKeepsTheWatchUntilTheTargetIsDown()
{
    const char* name = "knockout watch: taken is not the end, the watch waits for the target to go down";
    KnockoutWatchFacts f = Fresh();
    f.taskNow = kWalk;
    f.elapsedMs = 40;
    CHECK(NextKnockoutWatchStep(f, 1500, 120000) == KnockoutStep_Taken);

    f.phase = KnockoutPhase_AwaitingDown;
    f.elapsedMs = 30000;
    CHECK(NextKnockoutWatchStep(f, 1500, 120000) == KnockoutStep_Wait);
    // Taken again or idle again: still waiting for the knockout to land.
    f.taskNow = kIdle;
    CHECK(NextKnockoutWatchStep(f, 1500, 120000) == KnockoutStep_Wait);

    f.targetDown = true;
    CHECK(NextKnockoutWatchStep(f, 1500, 120000) == KnockoutStep_CleanupDown);
}

void ATargetAlreadyDownIsCleanedUpInEitherPhase()
{
    const char* name = "knockout watch: a target down (or unreadable) ends the watch with a cleanup";
    KnockoutWatchFacts f = Fresh();
    f.targetDown = true;
    CHECK(NextKnockoutWatchStep(f, 1500, 120000) == KnockoutStep_CleanupDown);

    f = Fresh();
    f.targetUnreadable = true;
    CHECK(NextKnockoutWatchStep(f, 1500, 120000) == KnockoutStep_CleanupDown);

    f = Fresh();
    f.phase = KnockoutPhase_AwaitingDown;
    f.targetUnreadable = true;
    CHECK(NextKnockoutWatchStep(f, 1500, 120000) == KnockoutStep_CleanupDown);
}

void AKnockoutThatNeverLandsIsAbandonedAtTheWindow()
{
    const char* name = "knockout watch: taken but never landed -> cleanup at the land window";
    KnockoutWatchFacts f = Fresh();
    f.phase = KnockoutPhase_AwaitingDown;
    f.taskNow = kKnockout;
    f.elapsedMs = 119999;
    CHECK(NextKnockoutWatchStep(f, 1500, 120000) == KnockoutStep_Wait);
    f.elapsedMs = 120000;
    CHECK(NextKnockoutWatchStep(f, 1500, 120000) == KnockoutStep_CleanupTimeout);
    CHECK(kKnockoutLandWindowMs == kOrderedKnockoutWindowMs);
}

void EveryStepEndsOrKeepsTheWatch()
{
    const char* name = "knockout watch: which steps end the watch";
    CHECK(!KnockoutStepEndsWatch(KnockoutStep_Wait));
    CHECK(!KnockoutStepEndsWatch(KnockoutStep_Taken));
    CHECK(KnockoutStepEndsWatch(KnockoutStep_Fallback));
    CHECK(KnockoutStepEndsWatch(KnockoutStep_CleanupDown));
    CHECK(KnockoutStepEndsWatch(KnockoutStep_CleanupTimeout));
    CHECK(KnockoutStepName(KnockoutStep_CleanupDown)[0] != '?');
    CHECK(KnockoutStepName(KnockoutStep_CleanupTimeout)[0] != '?');
    CHECK(KnockoutStepName(KnockoutStep_Fallback)[0] != '?');
}

void StealthIsOnlyTurnedOffWhenTheOrderTurnedItOn()
{
    const char* name = "knockout cleanup: stealth off only if the actor was not sneaking before the order";
    CHECK(ShouldEndStealthAfterKnockout(false, true));
    CHECK(!ShouldEndStealthAfterKnockout(true, true));
    CHECK(!ShouldEndStealthAfterKnockout(false, false));
    CHECK(!ShouldEndStealthAfterKnockout(true, false));
}

StrayKnockoutGoal Stray()
{
    StrayKnockoutGoal g;
    g.hasGoal = true;
    g.ownedByWatch = false;
    g.targetKnown = true;
    g.targetDown = false;
    g.targetInPlayerCage = true;
    return g;
}

// Already-stuck saves: the goal is removed when it chases a prisoner of the
// player's or someone already down; a live watch's own goal and a goal whose
// target is unknown or a free, standing stranger are left alone.
void AStrayGoalIsClearedOnlyWhenItCannotBeWanted()
{
    const char* name = "stray goal: cleared for a caged prisoner or a downed target, never for the watch's own";
    CHECK(ShouldClearStrayKnockoutGoal(Stray()));

    StrayKnockoutGoal g = Stray();
    g.targetInPlayerCage = false;
    g.targetDown = true;
    CHECK(ShouldClearStrayKnockoutGoal(g));

    g = Stray();
    g.targetInPlayerCage = false;
    CHECK(!ShouldClearStrayKnockoutGoal(g));

    g = Stray();
    g.ownedByWatch = true;
    CHECK(!ShouldClearStrayKnockoutGoal(g));

    g = Stray();
    g.targetKnown = false;
    CHECK(!ShouldClearStrayKnockoutGoal(g));

    g = Stray();
    g.hasGoal = false;
    CHECK(!ShouldClearStrayKnockoutGoal(g));
}

void TheSweepRunsEveryPeriod()
{
    const char* name = "stray goal: the sweep runs once per period, across a tick wrap";
    CHECK(StrayGoalSweepDue(0, 2999, 3000) == false);
    CHECK(StrayGoalSweepDue(0, 3000, 3000));
    CHECK(StrayGoalSweepDue(0xFFFFF000u, 0x00000400u, 3000) == true);
    CHECK(StrayGoalSweepDue(0xFFFFF000u, 0xFFFFF100u, 3000) == false);
    CHECK(kStrayGoalSweepMs == 3000);
}
} // namespace

void RunKnockoutWatchRulesTests()
{
    AnUntakenOrderWaitsThenFallsBack();
    ATakenOrderKeepsTheWatchUntilTheTargetIsDown();
    ATargetAlreadyDownIsCleanedUpInEitherPhase();
    AKnockoutThatNeverLandsIsAbandonedAtTheWindow();
    EveryStepEndsOrKeepsTheWatch();
    StealthIsOnlyTurnedOffWhenTheOrderTurnedItOn();
    AStrayGoalIsClearedOnlyWhenItCannotBeWanted();
    TheSweepRunsEveryPeriod();
}
