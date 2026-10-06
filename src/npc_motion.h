#pragma once

// Moving someone else's NPC: walk it to a character, run it to a point, pin it
// in place facing someone, and give it back its routine and its pace.
//
// Every call is a port of TotalSparring's NPC challenge (src/duel/challenge/),
// which proved them on foreign NPCs in game:
//   MoveTo / MoveToPoint  challenge_director.cpp IssueMoveGuarded
//                         (CharMovement::setDestination HIGH_PRIORITY +
//                         addOrder MOVE_CUS_ORDERED into the target's building)
//   StopWalking           challenge_director.cpp StopWalkingGuarded
//   BeginStand / Stand    challenge_stance.cpp (re-issue the move order to the
//                         spot it arrived at, then lookatPosition fullbody):
//                         halt + face alone did not hold, the NPC's AI walked off
//   ResumeRoutine         challenge_stance.cpp ResumeGuarded (clearOrders +
//                         reThinkCurrentAIAction)
//   BeginRun / ReapplyRun / EndRun
//                         challenge_run.cpp (save getMovementSpeedOrders, set
//                         RUN, restore with setDesiredSpeedOrders +
//                         restoreDesiredSpeed)
//
// The NPC's standing orders and AI goals are never rewritten: clearing the
// order this module gave is enough for it to go back to what it was doing.
//
// One NPC at a time (the submission director's single event), so the pinned
// spot and the saved speed are single slots. Every engine touch is SEH-guarded
// and reports failure as false; nothing is thrown.

class Character;

namespace totalrecruit
{
namespace NpcMotion
{
// Where a character stands, in world units. False when it cannot be read.
bool Position(Character* character, float* x, float* y, float* z);

// Distance between two characters in metres (world units / 10).
bool DistanceMeters(Character* a, Character* b, float* outMeters);

// Alive, conscious, on their feet and not carried: someone who can walk or be
// walked to (TotalSparring challenge_director.cpp SafeIsUp, plus isDown).
bool IsUp(Character* character);

// Walk `npc` to where `target` stands now.
bool MoveTo(Character* npc, Character* target);

// Walk `npc` to a spot offset from where `target` stands now (world units), into the
// target's building like MoveTo.
bool MoveBeside(Character* npc, Character* target, float dxUnits, float dzUnits);

// Walk (or run, after BeginRun) `npc` to a point, in world units.
bool MoveToPoint(Character* npc, float x, float y, float z);

// Clears the order this module gave. Arrived: also halt, so it stands in front
// of the player. Not arrived: ask its AI to re-think.
bool StopWalking(Character* npc, bool arrived);

// Records where `npc` stands as the spot to hold, then pins it there facing `who`.
bool BeginStand(Character* npc, Character* who);

// Re-pins `npc` to the recorded spot and turns its whole body to `who`.
bool Stand(Character* npc, Character* who);

// Turns `npc`'s whole body to where `who` stands now (lookatPosition fullbody, as
// Stand does) without pinning a spot: stateless, so it never touches the single
// pinned slot of the walk-up. Golpear faces the prisoner with it on landing.
bool Face(Character* npc, Character* who);

// Clears the NPC's orders and asks its AI to re-think: back to its routine.
bool ResumeRoutine(Character* npc);

// Saves the NPC's speed order and makes it run. True when it now runs.
bool BeginRun(Character* npc);

// Re-applies the run after a new move order (no save). Nothing unless BeginRun
// succeeded.
void ReapplyRun(Character* npc);

// Gives back the speed order BeginRun saved. Safe without a BeginRun; clears the
// slot either way. NULL (the NPC is gone) only clears it.
void EndRun(Character* npc);

// The free helper's fallback escort (helper.h), per NPC and stateless, so any number
// of helpers can use them:
//   SetPace    RUN (challenge_run.cpp SafeSetRun) or WALK speed orders, with no saved
//              slot: re-applied after each move order, WALK given back at the end.
//   EngageFor  TotalSparring fight_starter.cpp, the attacker's half of a bout: the
//              combat stance (PASSIVE off, HOLD off, DEF on, AGG on),
//              rememberCharacter(target, ST_TEMPORARY_ENEMY), addOrder(NULL,
//              FOCUSED_MELEE_ATTACK, target, no shift, clear, its own position),
//              attackTarget(target), target->attackingYou(npc, true, true), then
//              CombatClass::initCombatMode(hand(target), 0, true). True when the
//              order was given.
bool SetPace(Character* npc, bool run);
bool EngageFor(Character* npc, Character* target);
} // namespace NpcMotion
} // namespace totalrecruit
