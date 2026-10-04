#pragma once

// The walk-up of one NPC to the player, engine side, shared by every event that
// brings someone over to talk: the submission event (submission_director.cpp) and
// the free helper's offer (helper.cpp).
//
// Extracted from submission_director.cpp so the second event reuses the proven
// walk-up instead of a copy of it (TotalSparring's challenge walk-up, ported):
//
//   - The slot. One walk-up event at a time across every director: a submission and
//     a helper offer never walk two NPCs up at once, and npc_motion.h keeps a single
//     pinned spot and a single saved pace. A director claims the slot when its
//     event starts and releases it when the event ends.
//   - The clock. Real seconds that stop while the game is paused
//     (submission_rules.h AdvanceEventClock); one per director.
//   - The walker. The NPC, its key (an address is not an identity: the pointer is
//     only used while the key read back from it still matches), the calm toward
//     the squad re-asserted once a second (pacify.h SafeCalmToward, with the strike
//     count that says it broke, submission_rules.h NextCombatStrikes), and one step
//     of the walk (approach_rules.h ApproachStep): re-path to the leader, stand and
//     wait while the offer window or a fight keeps the player busy, stop and face
//     the leader on arrival. Arrive and Abort are handed back: what they mean is the
//     director's.

#include "approach_rules.h"
#include "char_key.h"

#include <string>

class Character;

namespace totalrecruit
{
// ------------------------------------------------------------------ The slot
enum ApproachOwner
{
    ApproachOwner_None,
    ApproachOwner_Submission,
    ApproachOwner_Helper
};

// Takes the slot for `owner`. True when it was free or already `owner`'s.
bool ClaimApproachSlot(ApproachOwner owner);
// Gives the slot back; nothing when `owner` does not hold it.
void ReleaseApproachSlot(ApproachOwner owner);
// Whether `owner` could claim the slot now.
bool ApproachSlotFreeFor(ApproachOwner owner);
// Who holds it, in English for the log.
const char* ApproachOwnerName(ApproachOwner owner);

// ----------------------------------------------------------------- The clock
class EventClock
{
public:
    EventClock();
    // Adds the real time since the last call, unless the game is paused.
    void Advance();
    float Now() const;
    // Back to zero, as at load.
    void Reset();

private:
    float seconds_;
    unsigned long lastTick_;
    bool haveLastTick_;
};

// ---------------------------------------------------------------- The walker
struct WalkUp
{
    Character* npc;
    CharKey key;
    std::string name;
    // Event seconds the current phase began and the calm / move order was last given.
    float phaseStart;
    float lastReassert;
    // Arrived while the player was busy: standing and waiting.
    bool standing;
    // How many calm checks in a row found the NPC fighting (submission_rules.h).
    int strikes;

    WalkUp();
};

// The walker's NPC while its key still matches, else NULL.
Character* WalkUpNpc(const WalkUp& walk);

// Starts a walk-up of `npc` to `leader` at event time `now`: mints the key, calms
// it toward the squad and gives the move order. False when the key cannot be read
// (nothing was done). `moving` says whether the move order went through.
bool BeginWalkUp(WalkUp* walk, Character* npc, Character* leader, float now, bool* moving);

// The calm, once a second: notes whether the last one held, then clears hostility
// toward every squad member again.
void ReassertCalm(WalkUp* walk, Character* npc);

// One frame of the walk at event time `now`. Re-paths and stands itself; on Arrive
// it has already stopped the NPC, calmed it and pinned it facing the leader. `tag`
// prefixes its log lines ("[submission]", "[helper]").
ApproachAction StepWalkUp(WalkUp* walk, float now, const ApproachLimits& limits, const char* tag);
} // namespace totalrecruit
