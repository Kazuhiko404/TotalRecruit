#include "beat_walk_rules.h"

namespace totalrecruit
{
namespace
{
float Clamped(float value, float low, float high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

BeatWalkStep Step(BeatWalkStepKind kind, BeatWalkAbort reason)
{
    BeatWalkStep step;
    step.kind = kind;
    step.reason = reason;
    return step;
}

BeatWalkStep Abort(BeatWalkAbort reason)
{
    return Step(BeatStep_Abort, reason);
}
} // namespace

BeatWalkLimits BeatWalkLimitsFrom(float reachMeters, float walkSeconds)
{
    BeatWalkLimits limits;
    limits.reachMeters = Clamped(reachMeters, kBeatReachMinMeters, kBeatReachMaxMeters);
    limits.timeoutSec = Clamped(walkSeconds, kBeatWalkMinSeconds, kBeatWalkMaxSeconds);
    return limits;
}

bool InBeatReach(float distanceMeters, float reachMeters)
{
    return distanceMeters >= 0.0f && distanceMeters <= reachMeters;
}

BeatWalkStep NextBeatWalkStep(const BeatWalkFacts& facts, const BeatWalkLimits& limits)
{
    if (!facts.actorResolves)
    {
        return Abort(BeatAbort_ActorGone);
    }
    if (!facts.actorUp)
    {
        return Abort(BeatAbort_ActorDown);
    }
    if (!facts.targetResolves || facts.click == CageClick_NotAllowed)
    {
        return Abort(BeatAbort_NotAllowed);
    }
    if (facts.click == CageClick_Resting)
    {
        return Abort(BeatAbort_Resting);
    }
    if (InBeatReach(facts.distanceMeters, limits.reachMeters))
    {
        return Step(BeatStep_Land, BeatAbort_None);
    }
    if (facts.elapsedSec >= limits.timeoutSec)
    {
        return Abort(BeatAbort_Timeout);
    }
    if (facts.sinceProgressSec >= limits.stallSec)
    {
        return Abort(BeatAbort_Stalled);
    }
    return Step(BeatStep_Walk, BeatAbort_None);
}

BeatWalkProgress AdvanceBeatProgress(BeatWalkProgress progress, float distanceMeters,
                                     float frameSec, float progressMeters)
{
    if (distanceMeters >= 0.0f && progress.closestMeters < 0.0f)
    {
        // The reference: neither progress nor a stall.
        progress.closestMeters = distanceMeters;
        progress.sinceProgressSec = 0.0f;
        return progress;
    }
    if (distanceMeters >= 0.0f && distanceMeters <= progress.closestMeters - progressMeters)
    {
        progress.closestMeters = distanceMeters;
        progress.sinceProgressSec = 0.0f;
        return progress;
    }
    if (frameSec > 0.0f)
    {
        progress.sinceProgressSec += frameSec;
    }
    return progress;
}

bool BeatComplaintSpoken(bool animal)
{
    return !animal;
}

bool BeatAbortTellsPlayer(BeatWalkAbort reason)
{
    switch (reason)
    {
    case BeatAbort_ActorGone:
    case BeatAbort_ActorDown:
    case BeatAbort_NotAllowed:
    case BeatAbort_Resting:
    case BeatAbort_Timeout:
    case BeatAbort_Stalled:
    case BeatAbort_Failed:
        return true;
    case BeatAbort_None:
    case BeatAbort_Replaced:
    case BeatAbort_Load:
    default:
        return false;
    }
}

const char* BeatAbortMessageKey(BeatWalkAbort reason)
{
    switch (reason)
    {
    case BeatAbort_ActorGone:
    case BeatAbort_ActorDown: return "cage.beatAbort.actor";
    case BeatAbort_Timeout:   return "cage.beatAbort.timeout";
    case BeatAbort_Stalled:   return "cage.beatAbort.stalled";
    default:                  return "";
    }
}

const char* BeatAbortFallback(BeatWalkAbort reason)
{
    switch (reason)
    {
    case BeatAbort_ActorGone:
    case BeatAbort_ActorDown: return "{actor} ya no puede golpearlo: no hubo golpe";
    case BeatAbort_Timeout:   return "{actor} no lleg\xc3\xb3 a la jaula a tiempo: no hubo golpe";
    case BeatAbort_Stalled:   return "{actor} dej\xc3\xb3 de acercarse a la jaula: no hubo golpe";
    default:                  return "";
    }
}

const char* BeatAbortText(BeatWalkAbort reason)
{
    switch (reason)
    {
    case BeatAbort_ActorGone:  return "actor gone";
    case BeatAbort_ActorDown:  return "actor down";
    case BeatAbort_NotAllowed: return "not allowed";
    case BeatAbort_Resting:    return "resting";
    case BeatAbort_Timeout:    return "timeout";
    case BeatAbort_Stalled:    return "stalled";
    case BeatAbort_Failed:     return "blow failed";
    case BeatAbort_Replaced:   return "replaced by a new Golpear";
    case BeatAbort_Load:       return "at load";
    case BeatAbort_None:
    default:                   return "none";
    }
}
} // namespace totalrecruit
