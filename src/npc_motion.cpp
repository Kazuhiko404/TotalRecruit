#include "common.h"
#include "npc_motion.h"
#include "approach_rules.h"

#include <kenshi/AI/AITaskSystem.h>
#include <kenshi/CharMovement.h>

#include <cmath>

// Ports of TotalSparring src/duel/challenge/ (see npc_motion.h for each source).
// AI/AITaskSystem.h (OrdersReceiver) is included here and Tasker.h is not: the
// two define taskPriority, so one translation unit can take only one of them.

namespace totalrecruit
{
namespace
{
Ogre::Vector3 g_spot(0.0f, 0.0f, 0.0f);

bool g_running = false;
MoveSpeed g_savedSpeed = WALK;

// SEH frames hold PODs only (C2712): every Ogre::Vector3 lives in a plain helper
// and only the call crosses the guard.
bool ReadPositionGuarded(Character* character, float* x, float* y, float* z)
{
    const Ogre::Vector3 p = character->getPosition();
    *x = p.x;
    *y = p.y;
    *z = p.z;
    return true;
}

// challenge_director.cpp IssueMoveGuarded, aimed at a point.
bool IssueMoveGuarded(Character* npc, Building* building, float x, float y, float z)
{
    const Ogre::Vector3 destination(x, y, z);
    CharMovement* movement = npc->getMovement();
    if (movement != NULL)
        movement->setDestination(destination, HIGH_PRIORITY, true);
    npc->addOrder(building, MOVE_CUS_ORDERED, NULL, false, true, destination);
    return true;
}

bool SafeIssueMove(Character* npc, Building* building, float x, float y, float z)
{
    TR_TRY { return IssueMoveGuarded(npc, building, x, y, z); }
    TR_CATCH { return false; }
}

bool SafeBuildingOf(Character* character, Building** out)
{
    TR_TRY { *out = character->isIndoors().getBuilding(); return true; }
    TR_CATCH { return false; }
}

void StopWalkingGuarded(Character* npc, bool arrived)
{
    OrdersReceiver* orders = npc->getOrdersReciever();
    if (orders != NULL)
        orders->clearOrders();
    if (arrived)
    {
        CharMovement* movement = npc->getMovement();
        if (movement != NULL)
            movement->halt();
    }
    else
    {
        npc->reThinkCurrentAIAction();
    }
}

bool RecordSpotGuarded(Character* npc)
{
    g_spot = npc->getPosition();
    return true;
}

// challenge_stance.cpp StandGuarded: the walk-up's own move order aimed at the
// held spot, then the facing (Proving Grounds TownAnnouncer.cpp:222).
bool StandGuarded(Character* npc, Character* who)
{
    const Ogre::Vector3 target = who->getPosition();
    Building* building = npc->isIndoors().getBuilding();
    CharMovement* movement = npc->getMovement();
    if (movement != NULL)
        movement->setDestination(g_spot, HIGH_PRIORITY, true);
    npc->addOrder(building, MOVE_CUS_ORDERED, NULL, false, true, g_spot);
    npc->lookatPosition(target, true);
    return true;
}

bool ResumeGuarded(Character* npc)
{
    OrdersReceiver* orders = npc->getOrdersReciever();
    if (orders != NULL)
        orders->clearOrders();
    npc->reThinkCurrentAIAction();
    return true;
}

bool SafeReadSpeedOrders(Character* npc, MoveSpeed* out)
{
    TR_TRY { *out = npc->getMovementSpeedOrders(); return true; }
    TR_CATCH { return false; }
}

// Proving Grounds TownAnnouncer.cpp:251-253 (challenge_run.cpp SafeSetRun).
bool SafeSetRun(Character* npc)
{
    TR_TRY
    {
        CharMovement* movement = npc->getMovement();
        if (movement == NULL)
            return false;
        movement->setDesiredSpeedOrders(RUN);
        movement->setDesiredSpeed(RUN);
        return true;
    }
    TR_CATCH { return false; }
}

// Proving Grounds TownAnnouncer.cpp:271-272 (challenge_run.cpp SafeRestore).
bool SafeRestoreSpeed(Character* npc, MoveSpeed speed)
{
    TR_TRY
    {
        CharMovement* movement = npc->getMovement();
        if (movement == NULL)
            return false;
        movement->setDesiredSpeedOrders(speed);
        movement->restoreDesiredSpeed();
        return true;
    }
    TR_CATCH { return false; }
}
} // namespace

namespace NpcMotion
{
bool Position(Character* character, float* x, float* y, float* z)
{
    if (character == NULL)
        return false;
    TR_TRY { return ReadPositionGuarded(character, x, y, z); }
    TR_CATCH { return false; }
}

bool DistanceMeters(Character* a, Character* b, float* outMeters)
{
    float ax = 0.0f, ay = 0.0f, az = 0.0f;
    float bx = 0.0f, by = 0.0f, bz = 0.0f;
    if (!Position(a, &ax, &ay, &az) || !Position(b, &bx, &by, &bz))
        return false;
    const float dx = ax - bx;
    const float dy = ay - by;
    const float dz = az - bz;
    *outMeters = WorldUnitsToMeters(std::sqrt(dx * dx + dy * dy + dz * dz));
    return true;
}

bool IsUp(Character* character)
{
    if (character == NULL)
        return false;
    TR_TRY
    {
        return !character->isDead() && !character->isUnconcious() && !character->isDown() &&
               !character->isBeingCarried();
    }
    TR_CATCH { return false; }
}

bool MoveTo(Character* npc, Character* target)
{
    if (npc == NULL || target == NULL)
        return false;
    float x = 0.0f, y = 0.0f, z = 0.0f;
    Building* building = NULL;
    if (!Position(target, &x, &y, &z) || !SafeBuildingOf(target, &building))
        return false;
    return SafeIssueMove(npc, building, x, y, z);
}

bool MoveToPoint(Character* npc, float x, float y, float z)
{
    if (npc == NULL)
        return false;
    Building* building = NULL;
    if (!SafeBuildingOf(npc, &building))
        return false;
    return SafeIssueMove(npc, building, x, y, z);
}

bool StopWalking(Character* npc, bool arrived)
{
    if (npc == NULL)
        return false;
    TR_TRY { StopWalkingGuarded(npc, arrived); return true; }
    TR_CATCH { return false; }
}

bool BeginStand(Character* npc, Character* who)
{
    if (npc == NULL || who == NULL)
        return false;
    bool recorded = false;
    TR_TRY { recorded = RecordSpotGuarded(npc); }
    TR_CATCH { recorded = false; }
    if (!recorded)
        return false;
    return Stand(npc, who);
}

bool Stand(Character* npc, Character* who)
{
    if (npc == NULL || who == NULL)
        return false;
    TR_TRY { return StandGuarded(npc, who); }
    TR_CATCH { return false; }
}

bool ResumeRoutine(Character* npc)
{
    if (npc == NULL)
        return false;
    TR_TRY { return ResumeGuarded(npc); }
    TR_CATCH { return false; }
}

bool BeginRun(Character* npc)
{
    g_running = false;
    if (npc == NULL)
        return false;
    MoveSpeed saved = WALK;
    if (!SafeReadSpeedOrders(npc, &saved) || !SafeSetRun(npc))
        return false;
    g_savedSpeed = saved;
    g_running = true;
    return true;
}

void ReapplyRun(Character* npc)
{
    if (!g_running || npc == NULL)
        return;
    SafeSetRun(npc);
}

void EndRun(Character* npc)
{
    if (!g_running)
        return;
    g_running = false;
    if (npc == NULL)
        return;
    SafeRestoreSpeed(npc, g_savedSpeed);
}
} // namespace NpcMotion
} // namespace totalrecruit
