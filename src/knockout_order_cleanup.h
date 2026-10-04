#pragma once

// Engine side of "the knockout order acts once": undoes what Noquear's order left
// on the actor, and heals actors stuck with an older knockout goal. The decisions
// are knockout_watch_rules.h; this module only reads and writes the engine.
//
// The cleanup is TotalPredator's pairing of every add with a stop
// (predator_logic.cpp RawStopOrders / RawStopAction), cut down to what the
// knockout order added, and never more:
//   OrdersReceiver::removeGoal(STEALTH_KNOCKOUT)   the goal the AI kept pursuing
//   OrdersReceiver::clearOrders()                  only when it removes exactly our
//                                                  order: the queue holds one order,
//                                                  the knockout on our prisoner
//                                                  (ShouldDropKnockoutOrder). KenshiLib
//                                                  has no call to remove one order, so
//                                                  a shift-queued or newer order keeps
//                                                  the whole queue untouched
//   Character::setStealthMode(false)               only when the order turned it on
//                                                  (ShouldEndStealthAfterKnockout)
//   Character::reThinkCurrentAIAction()            the AI picks what to do next
//
// The stray-goal sweep only ever removes the goal (never an order) and only for a
// goal whose target is a prisoner in the player's own cage
// (ShouldClearStrayKnockoutGoal), so a knockout from Kenshi's own menu is the game's.
//
// Its own translation unit: it needs AI/AITaskSystem.h (OrdersReceiver), which
// cannot share one with Tasker.h (both define taskPriority).
//
// Log (probeOnClick): "[cage] <actor> knockout cleanup (goal removed[, order
// cleared], stealth off|kept)"; a stray goal adds " stray goal for <target>".
// Failures always.

class Character;

namespace totalrecruit
{
// Whether the actor is sneaking now (Character::isStealthMode). Read when the
// order goes in, so the cleanup knows whether the order turned it on.
bool ReadStealthMode(Character* actor, bool* out);

// Whether the actor is still on the knockout in a way the task alone does not
// show: a STEALTH_KNOCKOUT player order still queued (walking to it), and the AI's
// current goal being the knockout. False when the orders cannot be read.
bool ReadKnockoutPursuit(Character* actor, bool* orderQueued, bool* goalCurrent);

// Undoes the knockout order `actor` was given on `target` (see above).
// `stealthBefore` is what ReadStealthMode said when the order went in; `why` is
// for the log only.
void CleanUpKnockoutOrder(Character* actor, Character* target, bool stealthBefore,
                          const char* why);

// The periodic heal: every player character other than `watchedActor` that
// carries a STEALTH_KNOCKOUT goal is checked against ShouldClearStrayKnockoutGoal
// and, when it may go, loses the goal (its orders and its stealth are left as they
// are: nothing says who gave them). Rate-limited inside (kStrayGoalSweepMs), so it
// is safe to call every frame.
void SweepStrayKnockoutGoals(Character* watchedActor);
} // namespace totalrecruit
