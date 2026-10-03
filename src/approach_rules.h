#pragma once

// The walk-up of an NPC to the player, without the engine.
//
// Ported from TotalSparring's challenge walk-up (src/duel/challenge/
// approach_rules.cpp, ApproachStep, and world_scale.h): the director reads the
// engine once per frame into an ApproachInput, this step decides what the walk
// does next, and the director acts. Every rule is a plain value, tested in
// tests/rules_tests/submission_rules_tests.cpp.
//
// Two changes from the reference, both because the walker here is a beaten NPC
// asking to join rather than a challenger:
//  - An open popup (or the player fighting) does not abort the walk. The NPC
//    still walks up; once there it stands and waits (Approach_Stand) until the
//    window is free or the time runs out, instead of opening a second window
//    over the first.
//  - The limits are a struct, not the challenge config, so the approach window
//    (submissionApproachSeconds) is the timeout.

#include <string>

namespace totalrecruit
{
// Kenshi world units are about 10 cm, not metres (TotalSparring world_scale.h:
// an NPC that walked up shoulder to shoulder stopped 7.5 units away). Every
// distance in the rules is in metres; engine distances are converted first.
const float kWorldUnitsPerMeter = 10.0f;

inline float WorldUnitsToMeters(float units)
{
    return units / kWorldUnitsPerMeter;
}

inline float MetersToWorldUnits(float meters)
{
    return meters * kWorldUnitsPerMeter;
}

enum ApproachActionKind
{
    Approach_KeepWalking = 0,   // nothing to do this frame
    Approach_Repath,            // re-issue the move order to the player's current spot
    Approach_Arrive,            // close enough and the window is free: stop and ask
    Approach_Stand,             // close enough but the window is busy: halt, face, wait
    Approach_Abort              // give up (see ApproachAbortReason)
};

// Why a walk-up was abandoned. Checked in this order: a character gone or a
// fight started makes every other answer meaningless.
enum ApproachAbortReason
{
    ApproachAbort_None = 0,
    ApproachAbort_Invalid,   // the NPC or the player leader can no longer be read / is down
    ApproachAbort_Combat,    // the calm did not hold: the NPC kept fighting
    ApproachAbort_TooFar,    // farther than abortDistanceMeters (or unknown)
    ApproachAbort_Timeout    // timeoutSec passed without asking
};

// Re-issue the move order (or re-pin the waiting NPC) this often, in event
// seconds, so a player who walks away is followed.
const float kApproachRepathIntervalSec = 1.0f;

struct ApproachLimits
{
    float arriveMeters;
    float abortDistanceMeters;
    float timeoutSec;

    ApproachLimits() : arriveMeters(3.0f), abortDistanceMeters(120.0f), timeoutSec(60.0f) {}
};

struct ApproachInput
{
    float elapsedSec;           // event seconds since the walk began (paused = no time)
    float distanceMeters;       // NPC to player leader; < 0 = could not be read
    float sinceLastRepathSec;   // event seconds since the move order was last issued
    bool npcValid;
    bool targetValid;
    bool combatStarted;         // the calm broke (submission_rules CalmBroken)
    bool targetBusy;            // a popup is open or the player is fighting

    ApproachInput();
};

struct ApproachAction
{
    ApproachActionKind kind;
    ApproachAbortReason reason;   // ApproachAbort_None unless kind == Approach_Abort
    std::string text;             // English, for the "[submission]" log line
};

// The abort reason's short English name ("timeout", "too far", ...).
const char* ApproachAbortText(ApproachAbortReason reason);

// PURE: one step of the walk-up. Order: Invalid, Combat, Arrive (distance <=
// arriveMeters and not busy), TooFar (> abortDistanceMeters or unknown),
// Timeout (elapsed >= timeoutSec), Stand (arrived but busy, every
// kApproachRepathIntervalSec), Repath (every kApproachRepathIntervalSec), else
// KeepWalking. Arriving wins over a timeout on the same frame: the NPC is
// already there.
ApproachAction ApproachStep(const ApproachInput& in, const ApproachLimits& limits);
} // namespace totalrecruit
