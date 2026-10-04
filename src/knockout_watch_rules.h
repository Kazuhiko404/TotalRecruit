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
// taken: it stays on until the prisoner is down (or dead, or unreadable) or the land
// window passes, and every way it ends cleans the actor up.
//
//   NextKnockoutWatchStep         what the watch does this frame.
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
    KnockoutStep_CleanupTimeout   // taken, but nothing landed inside the window: clean up, end
};

// How long after the order the watch waits for the knockout to land before it lets
// the actor go anyway. The same window the credit bridge gives the knockout.
extern const unsigned int kKnockoutLandWindowMs;

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
    unsigned int elapsedMs;  // since the order (GetTickCount differences wrap safely)

    KnockoutWatchFacts()
        : phase(KnockoutPhase_AwaitingTake), taskBefore(0), ordered(0), readTask(false),
          taskNow(0), targetDown(false), targetUnreadable(false), elapsedMs(0) {}
};

// A target down, dead or unreadable ends the watch in either phase (there is
// nothing left to knock out). AwaitingTake: JudgeOrderWatch decides between
// waiting, taken and the fallback. AwaitingDown: wait until the land window.
KnockoutWatchStep NextKnockoutWatchStep(const KnockoutWatchFacts& facts,
                                        unsigned int takeTimeoutMs, unsigned int landWindowMs);

// True for the steps after which the watch is closed (and the actor cleaned up).
bool KnockoutStepEndsWatch(KnockoutWatchStep step);

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
//   - and that target is already down (nothing left to knock out) or a prisoner
//     in the player's own cage (the only place the mod gives this goal).
// A knockout the player orders through Kenshi's own menu is an order, not this
// goal, and a free standing stranger is never touched.
struct StrayKnockoutGoal
{
    bool hasGoal;              // OrdersReceiver::hasGoal(STEALTH_KNOCKOUT)
    bool ownedByWatch;         // the live watch's own actor
    bool targetKnown;          // the current goal is the knockout and its subject reads
    bool targetDown;           // that target is unconscious or dead
    bool targetInPlayerCage;   // that target sits in one of the player's cages

    StrayKnockoutGoal()
        : hasGoal(false), ownedByWatch(false), targetKnown(false), targetDown(false),
          targetInPlayerCage(false) {}
};

bool ShouldClearStrayKnockoutGoal(const StrayKnockoutGoal& goal);

// How often the sweep runs: cheap enough every few seconds, quick enough to stop a
// stuck actor before the prisoner is back on his feet.
extern const unsigned int kStrayGoalSweepMs;

// True when `periodMs` has passed since `lastMs` (unsigned: safe across a wrap).
bool StrayGoalSweepDue(unsigned int lastMs, unsigned int nowMs, unsigned int periodMs);
} // namespace totalrecruit
