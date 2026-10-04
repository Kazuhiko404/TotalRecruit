#pragma once

// A temporary member who left the squad without a wander package, walked back to
// where they joined (the fallback of squad_leave.h; the decisions are
// leave_homing_rules.h).
//
// StartLeaveHoming issues the walk-up's own move order (npc_motion.h MoveToPoint,
// MOVE_CUS_ORDERED) to the join spot; LeaveHomingTick (frame tick) re-issues it every
// kHomingReissueSeconds until they are within kHomingArriveMeters (arrived) or
// kHomingTimeoutSeconds have passed (gave up), then clears the order and asks their
// AI to re-think (npc_motion.h ResumeRoutine). Logged as "[temp] <name> heading back
// to (x,z)", then "arrived" or "gave up". Session only: a load ends every walk.

class Character;

namespace totalrecruit
{
// Starts walking `npc` back to (x, z), world units.
void StartLeaveHoming(Character* npc, int x, int z);

// Called every frame from frame_tick.
void LeaveHomingTick();

// Load begin: every walk belongs to the world that was just abandoned.
void ForgetLeaveHoming();
} // namespace totalrecruit
