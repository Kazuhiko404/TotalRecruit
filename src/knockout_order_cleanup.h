#pragma once

// Engine side of "the knockout order acts once": undoes what Noquear's order left
// on the actor, and heals actors stuck with an older knockout goal. The decisions
// are knockout_watch_rules.h; this module only reads and writes the engine.
//
// The cleanup is TotalPredator's pairing of every add with a stop
// (predator_logic.cpp RawStopOrders / RawStopAction), cut down to what the
// knockout order added:
//   OrdersReceiver::removeGoal(STEALTH_KNOCKOUT)   the goal the AI kept pursuing
//   OrdersReceiver::clearOrders()                  only while the knockout order is
//                                                  still queued (hasPlayerOrder), so
//                                                  a newer order the player gave is kept
//   Character::setStealthMode(false)               only when the order turned it on
//                                                  (ShouldEndStealthAfterKnockout)
//   Character::reThinkCurrentAIAction()            the AI picks what to do next
//
// Its own translation unit: it needs AI/AITaskSystem.h (OrdersReceiver), which
// cannot share one with Tasker.h (both define taskPriority).
//
// Log (probeOnClick): "[cage] <actor> knockout cleanup (goal removed, stealth
// off|kept)"; a stray goal adds " stray goal for <target>". Failures always.

class Character;

namespace totalrecruit
{
// Whether the actor is sneaking now (Character::isStealthMode). Read when the
// order goes in, so the cleanup knows whether the order turned it on.
bool ReadStealthMode(Character* actor, bool* out);

// Undoes the knockout order on `actor` (see above). `stealthBefore` is what
// ReadStealthMode said when the order went in; `why` is for the log only.
void CleanUpKnockoutOrder(Character* actor, bool stealthBefore, const char* why);

// The periodic heal: every player character other than `watchedActor` that
// carries a STEALTH_KNOCKOUT goal is checked against ShouldClearStrayKnockoutGoal
// and, when it may go, loses the goal (stealth is left as it is: nothing says
// who turned it on). Rate-limited inside (kStrayGoalSweepMs), so it is safe to
// call every frame.
void SweepStrayKnockoutGoals(Character* watchedActor);
} // namespace totalrecruit
