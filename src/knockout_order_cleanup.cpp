#include "common.h"
#include "knockout_order_cleanup.h"
#include "knockout_watch_rules.h"
#include "config.h"
#include "engine_reads.h"
#include "log.h"

#include <kenshi/AI/AITaskSystem.h>

namespace totalrecruit
{
namespace
{
// Engine pointers can already be freed, so every dereference is guarded, and
// each guard keeps a POD-only frame (C2712): names and log lines are built
// outside them.

const int kSweepMaxCharacters = 256;

DWORD g_lastSweepMs = 0;

bool SafeStealth(Character* actor, bool* out)
{
    TR_TRY
    {
        *out = actor->isStealthMode();
        return true;
    }
    TR_CATCH { return false; }
}

// Drops the knockout goal, and the knockout order while it is still queued (a
// newer order the player gave since is not this module's to drop).
bool SafeDropKnockoutGoal(Character* actor, bool* hadGoal, bool* clearedOrder)
{
    TR_TRY
    {
        *hadGoal = false;
        *clearedOrder = false;
        OrdersReceiver* orders = actor->getOrdersReciever();
        if (orders == NULL)
        {
            return false;
        }
        *hadGoal = orders->hasGoal(STEALTH_KNOCKOUT);
        orders->removeGoal(STEALTH_KNOCKOUT);
        if (orders->hasPlayerOrder(STEALTH_KNOCKOUT))
        {
            orders->clearOrders();
            *clearedOrder = true;
        }
        return true;
    }
    TR_CATCH { return false; }
}

bool SafeEndStealth(Character* actor)
{
    TR_TRY
    {
        actor->setStealthMode(false);
        return true;
    }
    TR_CATCH { return false; }
}

bool SafeReThink(Character* actor)
{
    TR_TRY
    {
        actor->reThinkCurrentAIAction();
        return true;
    }
    TR_CATCH { return false; }
}

// Whether `actor` carries the knockout goal, and whom it is after when that goal
// is the one the AI is pursuing now (its current goal names the subject). The
// target stays NULL when the goal is there but not current: unknown.
bool SafeKnockoutGoalOf(Character* actor, bool* hasGoal, Character** target)
{
    TR_TRY
    {
        *hasGoal = false;
        *target = NULL;
        OrdersReceiver* orders = actor->getOrdersReciever();
        if (orders == NULL)
        {
            return false;
        }
        *hasGoal = orders->hasGoal(STEALTH_KNOCKOUT);
        if (*hasGoal && orders->currentGoal && orders->currentGoal.key() == STEALTH_KNOCKOUT)
        {
            *target = orders->currentGoal.subject.getCharacter();
        }
        return true;
    }
    TR_CATCH { return false; }
}

void LogCleanup(Character* actor, bool hadGoal, bool clearedOrder, bool stealthOff,
                const char* why)
{
    if (!GetConfig().probeOnClick)
    {
        return;
    }
    std::string line = "[cage] " + CharacterName(actor) + " knockout cleanup (" +
                       (hadGoal ? "goal removed" : "goal already gone") +
                       (clearedOrder ? ", order cleared" : "") + ", stealth " +
                       (stealthOff ? "off" : "kept") + ")";
    if (why != NULL && why[0] != '\0')
    {
        line += " ";
        line += why;
    }
    LogInfo(line);
}

bool TargetDown(Character* target)
{
    bool out = false;
    bool dead = false;
    return (SafeIsKnockedOut(target, &out) && out) || (SafeIsDead(target, &dead) && dead);
}

void HealOne(Character* actor, Character* watchedActor)
{
    StrayKnockoutGoal goal;
    Character* target = NULL;
    if (!SafeKnockoutGoalOf(actor, &goal.hasGoal, &target) || !goal.hasGoal)
    {
        return;
    }
    goal.ownedByWatch = actor == watchedActor;
    goal.targetKnown = target != NULL;
    if (goal.targetKnown)
    {
        goal.targetDown = TargetDown(target);
        goal.targetInPlayerCage = IsInPlayerCage(target);
    }
    if (!ShouldClearStrayKnockoutGoal(goal))
    {
        return;
    }

    bool hadGoal = false;
    bool clearedOrder = false;
    if (!SafeDropKnockoutGoal(actor, &hadGoal, &clearedOrder))
    {
        LogError("[cage] " + CharacterName(actor) + " knockout cleanup failed: stray goal unreadable");
        return;
    }
    SafeReThink(actor);
    const std::string why = "stray goal for " + CharacterName(target);
    LogCleanup(actor, hadGoal, clearedOrder, false, why.c_str());
}
} // namespace

bool ReadStealthMode(Character* actor, bool* out)
{
    return actor != NULL && out != NULL && SafeStealth(actor, out);
}

void CleanUpKnockoutOrder(Character* actor, bool stealthBefore, const char* why)
{
    if (actor == NULL)
    {
        return;
    }

    bool hadGoal = false;
    bool clearedOrder = false;
    if (!SafeDropKnockoutGoal(actor, &hadGoal, &clearedOrder))
    {
        LogError("[cage] " + CharacterName(actor) + " knockout cleanup failed: orders unreadable");
        return;
    }

    bool stealthNow = false;
    bool stealthOff = false;
    if (SafeStealth(actor, &stealthNow) && ShouldEndStealthAfterKnockout(stealthBefore, stealthNow))
    {
        stealthOff = SafeEndStealth(actor);
    }
    SafeReThink(actor);
    LogCleanup(actor, hadGoal, clearedOrder, stealthOff, why);
}

void SweepStrayKnockoutGoals(Character* watchedActor)
{
    const DWORD now = GetTickCount();
    if (!StrayGoalSweepDue(g_lastSweepMs, now, kStrayGoalSweepMs))
    {
        return;
    }
    g_lastSweepMs = now;

    Character* characters[kSweepMaxCharacters];
    const int count = SafePlayerCharacters(characters, kSweepMaxCharacters);
    for (int i = 0; i < count; ++i)
    {
        if (characters[i] != NULL)
        {
            HealOne(characters[i], watchedActor);
        }
    }
}
} // namespace totalrecruit
