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
// each guard keeps a POD-only frame (C2712): names, log lines and TaskMatch
// objects live outside them.

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

// Removes the knockout goal (all the cleanup and the sweep may always remove).
bool SafeRemoveKnockoutGoal(Character* actor, bool* hadGoal)
{
    TR_TRY
    {
        *hadGoal = false;
        OrdersReceiver* orders = actor->getOrdersReciever();
        if (orders == NULL)
        {
            return false;
        }
        *hadGoal = orders->hasGoal(STEALTH_KNOCKOUT);
        orders->removeGoal(STEALTH_KNOCKOUT);
        return true;
    }
    TR_CATCH { return false; }
}

// Unguarded: TaskMatch is a C++ object (C2712). SafeReadOrderQueue holds the guard
// (an access violation in here unwinds to it).
void ReadOrderQueueUnguarded(OrdersReceiver* orders, Character* target, KnockoutOrderQueue* out)
{
    out->ordered = (int)STEALTH_KNOCKOUT;
    out->queued = orders->hasPlayerOrder(STEALTH_KNOCKOUT);
    out->onlyOne = orders->orders.isOnlyOne();
    Tasker* first = orders->getFirstOrder();
    if (first != NULL)
    {
        const TaskMatch head(first);
        out->firstKey = (int)head.key();
        out->firstTargetIsOurs = target != NULL && head.subject.getCharacter() == target;
    }
    out->readable = true;
}

bool SafeReadOrderQueue(OrdersReceiver* orders, Character* target, KnockoutOrderQueue* out)
{
    TR_TRY
    {
        ReadOrderQueueUnguarded(orders, target, out);
        return true;
    }
    TR_CATCH { return false; }
}

bool SafeOrdersOf(Character* actor, OrdersReceiver** out)
{
    TR_TRY
    {
        *out = actor->getOrdersReciever();
        return *out != NULL;
    }
    TR_CATCH { return false; }
}

bool SafeClearOrders(OrdersReceiver* orders)
{
    TR_TRY
    {
        orders->clearOrders();
        return true;
    }
    TR_CATCH { return false; }
}

// Drops the knockout order only when clearOrders would remove exactly it
// (ShouldDropKnockoutOrder). True when it was dropped.
bool DropOwnKnockoutOrder(Character* actor, Character* target)
{
    OrdersReceiver* orders = NULL;
    if (!SafeOrdersOf(actor, &orders))
    {
        return false;
    }
    KnockoutOrderQueue queue;
    if (!SafeReadOrderQueue(orders, target, &queue) || !ShouldDropKnockoutOrder(queue))
    {
        return false;
    }
    return SafeClearOrders(orders);
}

bool SafePursuit(Character* actor, bool* orderQueued, bool* goalCurrent)
{
    TR_TRY
    {
        *orderQueued = false;
        *goalCurrent = false;
        OrdersReceiver* orders = actor->getOrdersReciever();
        if (orders == NULL)
        {
            return false;
        }
        *orderQueued = orders->hasPlayerOrder(STEALTH_KNOCKOUT);
        *goalCurrent = orders->currentGoal && orders->currentGoal.key() == STEALTH_KNOCKOUT;
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
        goal.targetInPlayerCage = IsInPlayerCage(target);
    }
    if (!ShouldClearStrayKnockoutGoal(goal))
    {
        return;
    }

    // The goal only: the actor's orders are the player's, whoever gave them.
    bool hadGoal = false;
    if (!SafeRemoveKnockoutGoal(actor, &hadGoal))
    {
        LogError("[cage] " + CharacterName(actor) + " knockout cleanup failed: stray goal unreadable");
        return;
    }
    SafeReThink(actor);
    const std::string why = "stray goal for " + CharacterName(target);
    LogCleanup(actor, hadGoal, false, false, why.c_str());
}
} // namespace

bool ReadStealthMode(Character* actor, bool* out)
{
    return actor != NULL && out != NULL && SafeStealth(actor, out);
}

bool ReadKnockoutPursuit(Character* actor, bool* orderQueued, bool* goalCurrent)
{
    return actor != NULL && orderQueued != NULL && goalCurrent != NULL &&
           SafePursuit(actor, orderQueued, goalCurrent);
}

void CleanUpKnockoutOrder(Character* actor, Character* target, bool stealthBefore,
                          const char* why)
{
    if (actor == NULL)
    {
        return;
    }

    bool hadGoal = false;
    if (!SafeRemoveKnockoutGoal(actor, &hadGoal))
    {
        LogError("[cage] " + CharacterName(actor) + " knockout cleanup failed: orders unreadable");
        return;
    }
    const bool clearedOrder = DropOwnKnockoutOrder(actor, target);

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
