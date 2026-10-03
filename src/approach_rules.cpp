#include "approach_rules.h"

#include <cstdio>

// Port of TotalSparring src/duel/challenge/approach_rules.cpp (ApproachStep),
// with the busy window turned from an abort into a wait (approach_rules.h).

namespace totalrecruit
{
namespace
{
ApproachAction Make(ApproachActionKind kind, ApproachAbortReason reason, const std::string& text)
{
    ApproachAction a;
    a.kind = kind;
    a.reason = reason;
    a.text = text;
    return a;
}

ApproachAction Abort(ApproachAbortReason reason, const std::string& text)
{
    return Make(Approach_Abort, reason, text);
}
} // namespace

ApproachInput::ApproachInput()
    : elapsedSec(0.0f), distanceMeters(-1.0f), sinceLastRepathSec(0.0f), npcValid(false),
      targetValid(false), combatStarted(false), targetBusy(false)
{
}

const char* ApproachAbortText(ApproachAbortReason reason)
{
    switch (reason)
    {
    case ApproachAbort_None:    return "none";
    case ApproachAbort_Invalid: return "invalid character";
    case ApproachAbort_Combat:  return "combat";
    case ApproachAbort_TooFar:  return "too far";
    case ApproachAbort_Timeout: return "timeout";
    }
    return "unknown";
}

ApproachAction ApproachStep(const ApproachInput& in, const ApproachLimits& limits)
{
    char buffer[96];

    if (!in.npcValid)
        return Abort(ApproachAbort_Invalid, "invalid character: the NPC can no longer be read or is down");
    if (!in.targetValid)
        return Abort(ApproachAbort_Invalid, "invalid character: the player leader can no longer be read or is down");

    if (in.combatStarted)
        return Abort(ApproachAbort_Combat, "combat: the calm did not hold, the NPC keeps fighting");

    const bool arrived = in.distanceMeters >= 0.0f && in.distanceMeters <= limits.arriveMeters;
    if (arrived && !in.targetBusy)
    {
        _snprintf_s(buffer, sizeof(buffer), _TRUNCATE, "arrived: %.1f m <= %.1f m",
                    in.distanceMeters, limits.arriveMeters);
        return Make(Approach_Arrive, ApproachAbort_None, buffer);
    }

    if (in.distanceMeters < 0.0f)
        return Abort(ApproachAbort_TooFar, "too far: distance unknown");
    if (in.distanceMeters > limits.abortDistanceMeters)
    {
        _snprintf_s(buffer, sizeof(buffer), _TRUNCATE, "too far: %.1f m > %.1f m",
                    in.distanceMeters, limits.abortDistanceMeters);
        return Abort(ApproachAbort_TooFar, buffer);
    }

    if (in.elapsedSec >= limits.timeoutSec)
    {
        _snprintf_s(buffer, sizeof(buffer), _TRUNCATE, "timeout: %.1f s >= %.1f s",
                    in.elapsedSec, limits.timeoutSec);
        return Abort(ApproachAbort_Timeout, buffer);
    }

    if (in.sinceLastRepathSec >= kApproachRepathIntervalSec)
    {
        if (arrived)
            return Make(Approach_Stand, ApproachAbort_None, "waiting: arrived, the window is busy");
        return Make(Approach_Repath, ApproachAbort_None, "repath");
    }

    return Make(Approach_KeepWalking, ApproachAbort_None, std::string());
}
} // namespace totalrecruit
