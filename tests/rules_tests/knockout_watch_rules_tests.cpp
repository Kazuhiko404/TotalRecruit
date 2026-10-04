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
    f.orderQueued = true;
    f.goalCurrent = false;
    f.abandonedMs = 0;
    return f;
}

void AnUntakenOrderWaitsThenFallsBack()
{
    const char* name = "knockout watch: unchanged task waits, then the fallback runs at the timeout";
    KnockoutWatchFacts f = Fresh();
    f.elapsedMs = 1499;
    CHECK(NextKnockoutWatchStep(f, 1500, 120000, 3000) == KnockoutStep_Wait);
    f.elapsedMs = 1500;
    CHECK(NextKnockoutWatchStep(f, 1500, 120000, 3000) == KnockoutStep_Fallback);
}

// The bug: the watch closed on "taken" and the goal it left behind kept the actor
// knocking the prisoner out every time he stood up.
void ATakenOrderKeepsTheWatchUntilTheTargetIsDown()
{
    const char* name = "knockout watch: taken is not the end, the watch waits for the target to go down";
    KnockoutWatchFacts f = Fresh();
    f.taskNow = kWalk;
    f.elapsedMs = 40;
    CHECK(NextKnockoutWatchStep(f, 1500, 120000, 3000) == KnockoutStep_Taken);

    f.phase = KnockoutPhase_AwaitingDown;
    f.elapsedMs = 30000;
    CHECK(NextKnockoutWatchStep(f, 1500, 120000, 3000) == KnockoutStep_Wait);
    // Taken again or idle again: still waiting for the knockout to land.
    f.taskNow = kIdle;
    CHECK(NextKnockoutWatchStep(f, 1500, 120000, 3000) == KnockoutStep_Wait);

    f.targetDown = true;
    CHECK(NextKnockoutWatchStep(f, 1500, 120000, 3000) == KnockoutStep_CleanupDown);
}

void ATargetAlreadyDownIsCleanedUpInEitherPhase()
{
    const char* name = "knockout watch: a target down (or unreadable) ends the watch with a cleanup";
    KnockoutWatchFacts f = Fresh();
    f.targetDown = true;
    CHECK(NextKnockoutWatchStep(f, 1500, 120000, 3000) == KnockoutStep_CleanupDown);

    f = Fresh();
    f.targetUnreadable = true;
    CHECK(NextKnockoutWatchStep(f, 1500, 120000, 3000) == KnockoutStep_CleanupDown);

    f = Fresh();
    f.phase = KnockoutPhase_AwaitingDown;
    f.targetUnreadable = true;
    CHECK(NextKnockoutWatchStep(f, 1500, 120000, 3000) == KnockoutStep_CleanupDown);
}

void AKnockoutThatNeverLandsIsAbandonedAtTheWindow()
{
    const char* name = "knockout watch: taken but never landed -> cleanup at the land window";
    KnockoutWatchFacts f = Fresh();
    f.phase = KnockoutPhase_AwaitingDown;
    f.taskNow = kKnockout;
    f.elapsedMs = 119999;
    CHECK(NextKnockoutWatchStep(f, 1500, 120000, 3000) == KnockoutStep_Wait);
    f.elapsedMs = 120000;
    CHECK(NextKnockoutWatchStep(f, 1500, 120000, 3000) == KnockoutStep_CleanupTimeout);
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
    CHECK(KnockoutStepEndsWatch(KnockoutStep_Abandoned));
    CHECK(KnockoutStepName(KnockoutStep_Abandoned)[0] != '?');
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
    g.targetInPlayerCage = true;
    return g;
}

// Already-stuck saves: the goal is removed only when it chases a prisoner in the
// player's own cage (the only place the mod gives this goal). A live watch's own
// goal, a goal whose target is unknown, and a goal on anyone else -- down or not:
// a vanilla knockout from Kenshi's own menu -- are left alone.
void AStrayGoalIsClearedOnlyWhenItCannotBeWanted()
{
    const char* name = "stray goal: cleared only for a prisoner in the player's cage, never for the watch's own";
    CHECK(ShouldClearStrayKnockoutGoal(Stray()));

    StrayKnockoutGoal g = Stray();
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

KnockoutOrderQueue OurOrderAlone()
{
    KnockoutOrderQueue q;
    q.readable = true;
    q.queued = true;
    q.onlyOne = true;
    q.firstKey = kKnockout;
    q.ordered = kKnockout;
    q.firstTargetIsOurs = true;
    return q;
}

// The bug: any knockout order in the queue wiped the whole queue, shift-queued
// orders and a newer order the player gave included. Only our own order, alone in
// the queue, may go -- then clearOrders removes exactly that one.
void OnlyOurOwnKnockoutOrderAloneIsDropped()
{
    const char* name = "knockout cleanup: the order goes only when it is ours and alone in the queue";
    CHECK(ShouldDropKnockoutOrder(OurOrderAlone()));

    KnockoutOrderQueue q = OurOrderAlone();
    q.onlyOne = false;   // something shift-queued behind it
    CHECK(!ShouldDropKnockoutOrder(q));

    q = OurOrderAlone();
    q.firstTargetIsOurs = false;   // a knockout on someone else
    CHECK(!ShouldDropKnockoutOrder(q));

    q = OurOrderAlone();
    q.firstKey = kWalk;   // a newer, different order first
    CHECK(!ShouldDropKnockoutOrder(q));

    q = OurOrderAlone();
    q.queued = false;
    CHECK(!ShouldDropKnockoutOrder(q));

    q = OurOrderAlone();
    q.readable = false;
    CHECK(!ShouldDropKnockoutOrder(q));
}

// The actor gave up on the order (the player sent him elsewhere, the AI moved on):
// the wait ends early with a cleanup and no fallback.
void AnAbandonedOrderEndsTheWaitEarly()
{
    const char* name = "knockout watch: an actor off the order for a while ends the wait, no fallback";
    KnockoutWatchFacts f = Fresh();
    f.phase = KnockoutPhase_AwaitingDown;
    f.taskNow = kIdle;
    f.orderQueued = false;
    f.goalCurrent = false;
    CHECK(ActorAbandonedKnockout(f));

    // Still on it in any way: not abandoned.
    f.taskNow = kKnockout;
    CHECK(!ActorAbandonedKnockout(f));
    f.taskNow = kIdle;
    f.orderQueued = true;   // walking to it under the order
    CHECK(!ActorAbandonedKnockout(f));
    f.orderQueued = false;
    f.goalCurrent = true;   // the AI still pursues the knockout goal
    CHECK(!ActorAbandonedKnockout(f));
    f.goalCurrent = false;
    f.readTask = false;     // unreadable is not proof
    CHECK(!ActorAbandonedKnockout(f));
    f.readTask = true;
    f.phase = KnockoutPhase_AwaitingTake;   // the take phase has its own timeout
    CHECK(!ActorAbandonedKnockout(f));

    // The streak only grows while abandoned, and any sign of the order resets it.
    CHECK(NextAbandonedMs(0, true, 40) == 40);
    CHECK(NextAbandonedMs(2990, true, 40) == 3030);
    CHECK(NextAbandonedMs(2990, false, 40) == 0);

    f.phase = KnockoutPhase_AwaitingDown;
    f.elapsedMs = 10000;
    f.abandonedMs = 2999;
    CHECK(NextKnockoutWatchStep(f, 1500, 120000, 3000) == KnockoutStep_Wait);
    f.abandonedMs = 3000;
    CHECK(NextKnockoutWatchStep(f, 1500, 120000, 3000) == KnockoutStep_Abandoned);
    // A target down still wins: that is the knockout landing.
    f.targetDown = true;
    CHECK(NextKnockoutWatchStep(f, 1500, 120000, 3000) == KnockoutStep_CleanupDown);
    CHECK(kKnockoutAbandonMs == 3000);
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

// The watch's own clock: a frame adds its real time, nothing while the game is
// paused (a pause must not force the fallback or run out the land window), and at
// most kWatchMaxFrameMs (a hitch or a long load is not a minute of watching).
void TheWatchClockStopsWhileThePauseIsOn()
{
    const char* name = "knockout watch: the clock adds frame time, nothing while paused, capped";
    CHECK(WatchFrameMs(16, false) == 16);
    CHECK(WatchFrameMs(16, true) == 0);
    CHECK(WatchFrameMs(600000, true) == 0);
    CHECK(WatchFrameMs(5000, false) == kWatchMaxFrameMs);
    CHECK(kWatchMaxFrameMs == 1000);

    // A long pause on a fresh order: still waiting, no fallback.
    unsigned int elapsed = 0;
    for (int i = 0; i < 1000; ++i)
    {
        elapsed += WatchFrameMs(16, true);
    }
    KnockoutWatchFacts f = Fresh();
    f.elapsedMs = elapsed;
    CHECK(NextKnockoutWatchStep(f, 1500, 120000, 3000) == KnockoutStep_Wait);
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
    OnlyOurOwnKnockoutOrderAloneIsDropped();
    AnAbandonedOrderEndsTheWaitEarly();
    TheWatchClockStopsWhileThePauseIsOn();
    TheSweepRunsEveryPeriod();
}
