#include "beat_strike_rules.h"

namespace totalrecruit
{
const char* const kBeatAnimDefaultClip = "badpunch";

namespace
{
float Clamped(float value, float low, float high)
{
    if (value < low)
    {
        return low;
    }
    return value > high ? high : value;
}

bool Blank(const std::string& text)
{
    return text.find_first_not_of(" \t\r\n") == std::string::npos;
}

BeatStrikeStep Step(BeatStrikeStepKind kind, BeatWalkAbort reason)
{
    BeatStrikeStep step;
    step.kind = kind;
    step.reason = reason;
    return step;
}

BeatStrikeStep BeforeTheBlow(const BeatStrikeFacts& facts, const BeatStrikeTiming& timing)
{
    if (!facts.actorResolves)
    {
        return Step(StrikeStep_Abort, BeatAbort_ActorGone);
    }
    if (!facts.actorUp)
    {
        return Step(StrikeStep_Abort, BeatAbort_ActorDown);
    }
    if (!facts.targetResolves || facts.click == CageClick_NotAllowed)
    {
        return Step(StrikeStep_Abort, BeatAbort_NotAllowed);
    }
    if (facts.click == CageClick_Resting)
    {
        return Step(StrikeStep_Abort, BeatAbort_Resting);
    }
    if (!timing.animate || (facts.clipChecked && !facts.clipPlaying))
    {
        return Step(StrikeStep_Land, BeatAbort_None);
    }
    return facts.elapsedSec >= timing.windupSec ? Step(StrikeStep_Land, BeatAbort_None)
                                                : Step(StrikeStep_Sustain, BeatAbort_None);
}

BeatStrikeStep AfterTheBlow(const BeatStrikeFacts& facts, const BeatStrikeTiming& timing)
{
    if (!timing.animate || !facts.actorResolves || !facts.actorUp ||
        facts.elapsedSec >= timing.animSec)
    {
        return Step(StrikeStep_End, BeatAbort_None);
    }
    return Step(StrikeStep_Sustain, BeatAbort_None);
}
} // namespace

BeatStrikeTiming BeatStrikeTimingFrom(bool enabled, const std::string& clip, float animSec,
                                      float windupSec)
{
    BeatStrikeTiming timing;
    timing.animate = enabled && !Blank(clip);
    timing.animSec = Clamped(animSec, kBeatAnimMinSeconds, kBeatAnimMaxSeconds);
    timing.windupSec = Clamped(windupSec, kBeatWindupMinSeconds, timing.animSec);
    return timing;
}

BeatStrikeStep NextBeatStrikeStep(const BeatStrikeFacts& facts, const BeatStrikeTiming& timing)
{
    return facts.landed ? AfterTheBlow(facts, timing) : BeforeTheBlow(facts, timing);
}
} // namespace totalrecruit
