// Tests for src/beat_strike_rules: Golpear's strike once the character is at the
// cage (priority-one C). The punch clip (vanilla "badpunch") plays on the actor,
// the blow lands after the wind-up, the clip ends after its duration; a clip that
// is not found playing, or the animation switched off, lands at once as before.

#include "test_support.h"
#include "beat_strike_rules.h"

#include <cmath>
#include <string>

using namespace totalrecruit;

namespace
{
bool Near(float a, float b)
{
    return std::fabs(a - b) < 0.001f;
}

BeatStrikeTiming Timing()
{
    return BeatStrikeTimingFrom(true, "badpunch", 0.9f, 0.35f);
}

// Both characters fine, the prisoner still a valid target, nothing landed yet.
BeatStrikeFacts Fresh(float elapsed)
{
    BeatStrikeFacts f;
    f.actorResolves = true;
    f.actorUp = true;
    f.targetResolves = true;
    f.click = CageClick_Go;
    f.landed = false;
    f.elapsedSec = elapsed;
    f.clipChecked = false;
    f.clipPlaying = false;
    return f;
}

void TheTimingComesFromTheConfigClamped()
{
    const char* name = "strike timing: enabled with a clip animates; seconds clamped, the wind-up never past the clip";
    BeatStrikeTiming t = Timing();
    CHECK(t.animate);
    CHECK(Near(t.animSec, 0.9f) && Near(t.windupSec, 0.35f));
    t = BeatStrikeTimingFrom(true, "badpunch", 99.0f, 99.0f);
    CHECK(Near(t.animSec, kBeatAnimMaxSeconds));
    CHECK(Near(t.windupSec, kBeatAnimMaxSeconds));
    t = BeatStrikeTimingFrom(true, "badpunch", 0.0f, -1.0f);
    CHECK(Near(t.animSec, kBeatAnimMinSeconds));
    CHECK(Near(t.windupSec, 0.0f));
    t = BeatStrikeTimingFrom(true, "badpunch", 0.5f, 0.8f);
    CHECK(Near(t.windupSec, 0.5f));
    CHECK(!BeatStrikeTimingFrom(false, "badpunch", 0.9f, 0.35f).animate);
    CHECK(!BeatStrikeTimingFrom(true, "", 0.9f, 0.35f).animate);
    CHECK(!BeatStrikeTimingFrom(true, "   ", 0.9f, 0.35f).animate);
    CHECK(std::string(kBeatAnimDefaultClip) == "badpunch");
}

void TheBlowWaitsForTheWindup()
{
    const char* name = "strike: sustain the clip until the wind-up, then land, then sustain until it ends";
    const BeatStrikeTiming t = Timing();
    CHECK(NextBeatStrikeStep(Fresh(0.0f), t).kind == StrikeStep_Sustain);
    BeatStrikeFacts f = Fresh(0.2f);
    f.clipChecked = true;
    f.clipPlaying = true;
    CHECK(NextBeatStrikeStep(f, t).kind == StrikeStep_Sustain);
    f.elapsedSec = 0.35f;
    CHECK(NextBeatStrikeStep(f, t).kind == StrikeStep_Land);
    f.landed = true;
    f.elapsedSec = 0.5f;
    CHECK(NextBeatStrikeStep(f, t).kind == StrikeStep_Sustain);
    f.elapsedSec = 0.9f;
    CHECK(NextBeatStrikeStep(f, t).kind == StrikeStep_End);
}

void NoAnimationLandsAtOnce()
{
    const char* name = "strike: switched off, the blow lands on the first step and the strike ends";
    const BeatStrikeTiming off = BeatStrikeTimingFrom(false, "badpunch", 0.9f, 0.35f);
    CHECK(NextBeatStrikeStep(Fresh(0.0f), off).kind == StrikeStep_Land);
    BeatStrikeFacts f = Fresh(0.0f);
    f.landed = true;
    CHECK(NextBeatStrikeStep(f, off).kind == StrikeStep_End);
}

void AClipNotFoundDegradesToAnInstantBlow()
{
    const char* name = "strike: a clip the engine does not report playing lands at once (today's behaviour)";
    BeatStrikeFacts f = Fresh(0.05f);
    f.clipChecked = true;
    f.clipPlaying = false;
    CHECK(NextBeatStrikeStep(f, Timing()).kind == StrikeStep_Land);
}

void AnActorWhoGoesDownLandsNothing()
{
    const char* name = "strike: before the blow, actor gone / down / prisoner not allowed / resting abort; after it, the clip just ends";
    const BeatStrikeTiming t = Timing();
    BeatStrikeFacts f = Fresh(0.1f);
    f.actorResolves = false;
    BeatStrikeStep s = NextBeatStrikeStep(f, t);
    CHECK(s.kind == StrikeStep_Abort && s.reason == BeatAbort_ActorGone);
    f = Fresh(0.1f);
    f.actorUp = false;
    s = NextBeatStrikeStep(f, t);
    CHECK(s.kind == StrikeStep_Abort && s.reason == BeatAbort_ActorDown);
    f = Fresh(0.1f);
    f.targetResolves = false;
    s = NextBeatStrikeStep(f, t);
    CHECK(s.kind == StrikeStep_Abort && s.reason == BeatAbort_NotAllowed);
    f = Fresh(0.1f);
    f.click = CageClick_NotAllowed;
    s = NextBeatStrikeStep(f, t);
    CHECK(s.kind == StrikeStep_Abort && s.reason == BeatAbort_NotAllowed);
    f = Fresh(0.1f);
    f.click = CageClick_Resting;
    s = NextBeatStrikeStep(f, t);
    CHECK(s.kind == StrikeStep_Abort && s.reason == BeatAbort_Resting);
    // Once landed, a resting prisoner (the blow itself started the rest) or an
    // actor who went down only ends the clip.
    f = Fresh(0.5f);
    f.landed = true;
    f.click = CageClick_Resting;
    CHECK(NextBeatStrikeStep(f, t).kind == StrikeStep_Sustain);
    f.actorUp = false;
    CHECK(NextBeatStrikeStep(f, t).kind == StrikeStep_End);
    f.actorUp = true;
    f.actorResolves = false;
    CHECK(NextBeatStrikeStep(f, t).kind == StrikeStep_End);
}
} // namespace

void RunBeatStrikeRulesTests()
{
    TheTimingComesFromTheConfigClamped();
    TheBlowWaitsForTheWindup();
    NoAnimationLandsAtOnce();
    AClipNotFoundDegradesToAnInstantBlow();
    AnActorWhoGoesDownLandsNothing();
}
