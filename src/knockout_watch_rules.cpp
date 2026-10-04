#include "knockout_watch_rules.h"

namespace totalrecruit
{
const unsigned int kKnockoutLandWindowMs = kOrderedKnockoutWindowMs;
const unsigned int kStrayGoalSweepMs = 3000;

KnockoutWatchStep NextKnockoutWatchStep(const KnockoutWatchFacts& facts,
                                        unsigned int takeTimeoutMs, unsigned int landWindowMs)
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
    return facts.elapsedMs >= landWindowMs ? KnockoutStep_CleanupTimeout : KnockoutStep_Wait;
}

bool KnockoutStepEndsWatch(KnockoutWatchStep step)
{
    return step == KnockoutStep_Fallback || step == KnockoutStep_CleanupDown ||
           step == KnockoutStep_CleanupTimeout;
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
    return goal.targetDown || goal.targetInPlayerCage;
}

bool StrayGoalSweepDue(unsigned int lastMs, unsigned int nowMs, unsigned int periodMs)
{
    return (nowMs - lastMs) >= periodMs;
}
} // namespace totalrecruit
