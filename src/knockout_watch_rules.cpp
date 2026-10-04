#include "knockout_watch_rules.h"

namespace totalrecruit
{
const unsigned int kKnockoutLandWindowMs = kOrderedKnockoutWindowMs;
const unsigned int kStrayGoalSweepMs = 3000;
// A few seconds: long enough to ride out the frames between a task ending and the
// next one starting, short enough to let a re-ordered actor go at once.
const unsigned int kKnockoutAbandonMs = 3000;
const unsigned int kWatchMaxFrameMs = 1000;

unsigned int WatchFrameMs(unsigned int realDeltaMs, bool paused)
{
    if (paused)
    {
        return 0;
    }
    return realDeltaMs < kWatchMaxFrameMs ? realDeltaMs : kWatchMaxFrameMs;
}

KnockoutWatchStep NextKnockoutWatchStep(const KnockoutWatchFacts& facts,
                                        unsigned int takeTimeoutMs, unsigned int landWindowMs,
                                        unsigned int abandonMs)
{
    if (facts.targetDown || facts.targetUnreadable)
    {
        return KnockoutStep_CleanupDown;
    }
    if (facts.phase == KnockoutPhase_AwaitingTake)
    {
        switch (JudgeOrderWatch(facts.taskBefore, facts.ordered, facts.readTask, facts.taskNow,
                                facts.elapsedMs, takeTimeoutMs))
        {
        case OrderWatch_Taken:    return KnockoutStep_Taken;
        case OrderWatch_TimedOut: return KnockoutStep_Fallback;
        case OrderWatch_Pending:
        default:                  return KnockoutStep_Wait;
        }
    }
    if (facts.abandonedMs >= abandonMs)
    {
        return KnockoutStep_Abandoned;
    }
    return facts.elapsedMs >= landWindowMs ? KnockoutStep_CleanupTimeout : KnockoutStep_Wait;
}

bool ActorAbandonedKnockout(const KnockoutWatchFacts& facts)
{
    return facts.phase == KnockoutPhase_AwaitingDown && facts.readTask &&
           facts.taskNow != facts.ordered && !facts.orderQueued && !facts.goalCurrent;
}

unsigned int NextAbandonedMs(unsigned int abandonedMs, bool abandonedNow, unsigned int deltaMs)
{
    return abandonedNow ? abandonedMs + deltaMs : 0;
}

bool ShouldDropKnockoutOrder(const KnockoutOrderQueue& queue)
{
    return queue.readable && queue.queued && queue.onlyOne && queue.firstKey == queue.ordered &&
           queue.firstTargetIsOurs;
}

bool KnockoutStepEndsWatch(KnockoutWatchStep step)
{
    return step == KnockoutStep_Fallback || step == KnockoutStep_CleanupDown ||
           step == KnockoutStep_CleanupTimeout || step == KnockoutStep_Abandoned;
}

const char* KnockoutStepName(KnockoutWatchStep step)
{
    switch (step)
    {
    case KnockoutStep_Wait:           return "wait";
    case KnockoutStep_Taken:          return "taken";
    case KnockoutStep_Fallback:       return "fallback";
    case KnockoutStep_CleanupDown:    return "target down";
    case KnockoutStep_CleanupTimeout: return "land window passed";
    case KnockoutStep_Abandoned:      return "order abandoned";
    }
    return "?";
}

bool ShouldEndStealthAfterKnockout(bool stealthBeforeOrder, bool stealthNow)
{
    return !stealthBeforeOrder && stealthNow;
}

bool ShouldClearStrayKnockoutGoal(const StrayKnockoutGoal& goal)
{
    if (!goal.hasGoal || goal.ownedByWatch || !goal.targetKnown)
    {
        return false;
    }
    return goal.targetInPlayerCage;
}

bool StrayGoalSweepDue(unsigned int lastMs, unsigned int nowMs, unsigned int periodMs)
{
    return (nowMs - lastMs) >= periodMs;
}
} // namespace totalrecruit
