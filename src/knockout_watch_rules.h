#pragma once

// PURE: the knockout order acts once. The decisions behind Noquear's watch and the
// cleanup that lets the actor go, without the engine; knockout_order_cleanup.cpp
// and cage_commands.cpp are the engine side.
//
// Noquear gives the actor addOrder(STEALTH_KNOCKOUT) + addGoal(STEALTH_KNOCKOUT,
// prisoner). The goal is persistent: left in place, the actor's AI keeps pursuing
// it, sneaks again and knocks the prisoner out every time he stands up (seen in the
// log as one "[cage] ... knockout ordered" followed by a "[defeat-hook] knockout
// fired" a minute apart, forever). So the watch no longer closes when the order is
// taken: it stays on until the prisoner is down (or dead, or unreadable), the actor
// abandons the order, or the land window passes, and every way it ends cleans the
// actor up.
//
//   NextKnockoutWatchStep         what the watch does this frame.
//   WatchFrameMs                  how much one frame advances the watch's clock
//                                 (nothing while the game is paused).
//   ActorAbandonedKnockout        whether the actor is off the order this frame
//   NextAbandonedMs               (and for how long in a row).
//   ShouldDropKnockoutOrder       whether the cleanup may drop the queued order.
//   ShouldEndStealthAfterKnockout whether the cleanup turns sneaking off.
//   ShouldClearStrayKnockoutGoal  whether an already-stuck goal (an older save, a
//                                 watch lost to a load) may be removed.
//
// No Kenshi include: tested in tests/rules_tests/knockout_watch_rules_tests.cpp.

#include "cage_command_rules.h"

namespace totalrecruit
{
enum KnockoutWatchPhase
{
    KnockoutPhase_AwaitingTake,   // the order went in; has the actor taken it?
    KnockoutPhase_AwaitingDown    // taken; waiting for the prisoner to go down
};

enum KnockoutWatchStep
{
    KnockoutStep_Wait,            // nothing to do this frame
    KnockoutStep_Taken,           // the actor took the order: move to AwaitingDown
    KnockoutStep_Fallback,        // never taken: the engine's own knockout, then the cleanup
    KnockoutStep_CleanupDown,     // the prisoner is down, dead or gone: clean up, end
    KnockoutStep_CleanupTimeout,  // taken, but nothing landed inside the window: clean up, end
    KnockoutStep_Abandoned,       // taken, then the actor left the order: clean up, end (no fallback)
    KnockoutStep_ActorGone        // the actor no longer resolves (unloaded, freed): end, no cleanup
};

// How long after the order the watch waits for the knockout to land before it lets
// the actor go anyway. The same window the credit bridge gives the knockout.
extern const unsigned int kKnockoutLandWindowMs;

// How long in a row the actor must be off the order (ActorAbandonedKnockout) before
// the wait ends early.
extern const unsigned int kKnockoutAbandonMs;

// What the watch reads each frame. Task values are the engine's TaskType as ints.
struct KnockoutWatchFacts
{
    KnockoutWatchPhase phase;
    int taskBefore;          // the actor's task when the order went in
    int ordered;             // the task ordered (STEALTH_KNOCKOUT)
    bool readTask;           // taskNow could be read
    int taskNow;
    bool targetDown;         // unconscious or dead
    bool targetUnreadable;   // its state could not be read at all
    unsigned int elapsedMs;  // watch time since the order (WatchFrameMs: paused = none)
    bool orderQueued;        // a STEALTH_KNOCKOUT player order is still queued
    bool goalCurrent;        // the AI's current goal is the knockout
    unsigned int abandonedMs; // how long in a row the actor has been off the order
    bool actorGone;          // the actor's key no longer resolves to the watched pointer

    KnockoutWatchFacts()
        : phase(KnockoutPhase_AwaitingTake), taskBefore(0), ordered(0), readTask(false),
          taskNow(0), targetDown(false), targetUnreadable(false), elapsedMs(0),
          orderQueued(false), goalCurrent(false), abandonedMs(0), actorGone(false) {}
};

// An actor that no longer resolves ends the watch first, with no cleanup (there is
// nobody to clean up). A target down, dead, unreadable or no longer resolving ends
// the watch in either phase (there is nothing left to knock out). AwaitingTake: JudgeOrderWatch decides between
// waiting, taken and the fallback. AwaitingDown: abandoned once the actor has been
// off the order for abandonMs in a row, else wait until the land window.
KnockoutWatchStep NextKnockoutWatchStep(const KnockoutWatchFacts& facts,
                                        unsigned int takeTimeoutMs, unsigned int landWindowMs,
                                        unsigned int abandonMs);

// The watch's clock. Real milliseconds, but none while the game is paused -- a
// pause must neither force the fallback (the 1.5 s take timeout would read as an
// actor who never moved) nor run out the land window -- and at most
// kWatchMaxFrameMs per frame (a hitch or a load is not a minute of watching). Real
// time and not game time, like the submission event's clock: the take timeout and
// the abandon streak are about the engine reacting, not about in-game hours.
extern const unsigned int kWatchMaxFrameMs;
unsigned int WatchFrameMs(unsigned int realDeltaMs, bool paused);

// The actor is off the order this frame: the order was taken (AwaitingDown), and
// now the task is not the knockout, no knockout order is queued (walking to it
// keeps the order queued) and the AI's current goal is not the knockout. An
// unreadable task is not proof.
bool ActorAbandonedKnockout(const KnockoutWatchFacts& facts);

// The abandoned streak after a frame of deltaMs: grows while abandoned, back to
// zero on any sign of the order.
unsigned int NextAbandonedMs(unsigned int abandonedMs, bool abandonedNow, unsigned int deltaMs);

// The head of the actor's order queue, as read from the engine (POD: filled in a
// guarded frame). Task values are the engine's TaskType as ints.
struct KnockoutOrderQueue
{
    bool readable;           // the queue could be read
    bool queued;             // OrdersReceiver::hasPlayerOrder(STEALTH_KNOCKOUT)
    bool onlyOne;            // the queue holds exactly one order
    int firstKey;            // the first order's task
    int ordered;             // STEALTH_KNOCKOUT
    bool firstTargetIsOurs;  // the first order's subject is the watched prisoner

    KnockoutOrderQueue()
        : readable(false), queued(false), onlyOne(false), firstKey(0), ordered(0),
          firstTargetIsOurs(false) {}
};

// KenshiLib 0.5.0 has no call that removes one order, only clearOrders (all of
// them). So the cleanup drops the order only when clearOrders removes exactly
// ours: the queue holds one order, and it is the knockout on our prisoner.
// Anything else -- a shift-queued order behind it, a newer order first, a
// knockout on someone else -- keeps the whole queue; the goal removal and the
// re-think are what let the actor go then.
bool ShouldDropKnockoutOrder(const KnockoutOrderQueue& queue);

// True for the steps after which the watch is closed.
bool KnockoutStepEndsWatch(KnockoutWatchStep step);

// True for the closing steps that clean the actor up: all but ActorGone.
bool KnockoutStepCleansUp(KnockoutWatchStep step);

// For logs only.
const char* KnockoutStepName(KnockoutWatchStep step);

// The cleanup turns sneaking off only when the order turned it on: the actor was
// not sneaking when the order went in, and is now. A player who was already
// sneaking keeps sneaking.
bool ShouldEndStealthAfterKnockout(bool stealthBeforeOrder, bool stealthNow);

// --- already-stuck goals -----------------------------------------------------
//
// A save made while the old code was live carries the goal with it, and a load
// drops the watch. A periodic sweep over the player's characters removes such a
// goal, but only when it cannot be a knockout the player still wants:
//   - not the live watch's own actor (the watch cleans that one up itself);
//   - the goal's target is known (the actor's current goal is the knockout and
//     names who) -- an unknown target is left alone;
//   - and that target is a prisoner in the player's own cage (the only place the
//     mod gives this goal). Merely down is not enough: a vanilla knockout the
//     player started from Kenshi's own menu may leave the same goal on someone
//     outside a cage, and that one is the game's, not the mod's.
// The sweep only ever removes the goal: it never touches the actor's orders.
struct StrayKnockoutGoal
{
    bool hasGoal;              // OrdersReceiver::hasGoal(STEALTH_KNOCKOUT)
    bool ownedByWatch;         // the live watch's own actor
    bool targetKnown;          // the current goal is the knockout and its subject reads
    bool targetInPlayerCage;   // that target sits in one of the player's cages

    StrayKnockoutGoal()
        : hasGoal(false), ownedByWatch(false), targetKnown(false), targetInPlayerCage(false) {}
};

bool ShouldClearStrayKnockoutGoal(const StrayKnockoutGoal& goal);

// How often the sweep runs: cheap enough every few seconds, quick enough to stop a
// stuck actor before the prisoner is back on his feet.
extern const unsigned int kStrayGoalSweepMs;

// True when `periodMs` has passed since `lastMs` (unsigned: safe across a wrap).
bool StrayGoalSweepDue(unsigned int lastMs, unsigned int nowMs, unsigned int periodMs);
} // namespace totalrecruit
