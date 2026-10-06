// Tests for src/beat_walk_rules: Golpear walks the selected character to the cage
// first and the blow lands only on arrival (beat-feedback T1) -- in reach or not,
// keep walking, land, or abort and why -- plus the progress streak that tells a
// stuck or redirected actor, and what the player is told (T2, T3).

#include "test_support.h"
#include "beat_walk_rules.h"

#include <cstring>
#include <string>
#include <vector>

using namespace totalrecruit;

namespace
{
BeatWalkLimits Limits()
{
    BeatWalkLimits limits;
    limits.reachMeters = 4.0f;
    limits.timeoutSec = 30.0f;
    limits.stallSec = 5.0f;
    limits.progressMeters = 0.5f;
    return limits;
}

// An actor on its feet walking toward a valid prisoner, 10 m away, just started.
BeatWalkFacts Walking()
{
    BeatWalkFacts facts;
    facts.actorResolves = true;
    facts.actorUp = true;
    facts.targetResolves = true;
    facts.click = CageClick_Go;
    facts.distanceMeters = 10.0f;
    facts.elapsedSec = 0.0f;
    facts.sinceProgressSec = 0.0f;
    return facts;
}

void TheReachIsInclusiveAndAnUnknownDistanceIsNeverInReach()
{
    const char* name = "beat walk: in reach up to the reach, inclusive; unread distance is not in reach";
    CHECK(InBeatReach(0.0f, 4.0f));
    CHECK(InBeatReach(4.0f, 4.0f));
    CHECK(!InBeatReach(4.01f, 4.0f));
    CHECK(!InBeatReach(-1.0f, 4.0f));
}

void InReachLandsAtOnce()
{
    const char* name = "beat walk: already in reach lands on the first step";
    BeatWalkFacts facts = Walking();
    facts.distanceMeters = 2.5f;
    const BeatWalkStep step = NextBeatWalkStep(facts, Limits());
    CHECK(step.kind == BeatStep_Land);
    CHECK(step.reason == BeatAbort_None);
}

void FarAwayKeepsWalking()
{
    const char* name = "beat walk: out of reach, in time and moving keeps walking";
    BeatWalkFacts facts = Walking();
    CHECK(NextBeatWalkStep(facts, Limits()).kind == BeatStep_Walk);
    facts.elapsedSec = 29.0f;
    facts.sinceProgressSec = 4.9f;
    CHECK(NextBeatWalkStep(facts, Limits()).kind == BeatStep_Walk);
}

void ArrivingWinsOverTheTimeoutAndTheStall()
{
    const char* name = "beat walk: arriving on the frame the time runs out still lands";
    BeatWalkFacts facts = Walking();
    facts.distanceMeters = 3.0f;
    facts.elapsedSec = 31.0f;
    facts.sinceProgressSec = 9.0f;
    CHECK(NextBeatWalkStep(facts, Limits()).kind == BeatStep_Land);
}

void TheTimeoutAborts()
{
    const char* name = "beat walk: out of reach once the walk time is up aborts (timeout)";
    BeatWalkFacts facts = Walking();
    facts.elapsedSec = 30.0f;
    const BeatWalkStep step = NextBeatWalkStep(facts, Limits());
    CHECK(step.kind == BeatStep_Abort);
    CHECK(step.reason == BeatAbort_Timeout);
}

void NoProgressAborts()
{
    const char* name = "beat walk: no closer for the stall time aborts (stuck, or another order)";
    BeatWalkFacts facts = Walking();
    facts.elapsedSec = 6.0f;
    facts.sinceProgressSec = 5.0f;
    const BeatWalkStep step = NextBeatWalkStep(facts, Limits());
    CHECK(step.kind == BeatStep_Abort);
    CHECK(step.reason == BeatAbort_Stalled);
}

void TheActorAndTheTargetComeFirst()
{
    const char* name = "beat walk: actor gone, actor down, prisoner gone / not allowed / resting abort before landing";
    BeatWalkFacts facts = Walking();
    facts.distanceMeters = 1.0f;

    facts.actorResolves = false;
    CHECK(NextBeatWalkStep(facts, Limits()).reason == BeatAbort_ActorGone);
    facts.actorResolves = true;

    facts.actorUp = false;
    CHECK(NextBeatWalkStep(facts, Limits()).reason == BeatAbort_ActorDown);
    facts.actorUp = true;

    facts.targetResolves = false;
    CHECK(NextBeatWalkStep(facts, Limits()).reason == BeatAbort_NotAllowed);
    facts.targetResolves = true;

    facts.click = CageClick_NotAllowed;
    CHECK(NextBeatWalkStep(facts, Limits()).reason == BeatAbort_NotAllowed);

    facts.click = CageClick_Resting;
    const BeatWalkStep resting = NextBeatWalkStep(facts, Limits());
    CHECK(resting.kind == BeatStep_Abort);
    CHECK(resting.reason == BeatAbort_Resting);
}

void AnActorGoneWinsOverEverythingElse()
{
    const char* name = "beat walk: an actor who no longer resolves is the first answer";
    BeatWalkFacts facts = Walking();
    facts.actorResolves = false;
    facts.actorUp = false;
    facts.targetResolves = false;
    facts.elapsedSec = 99.0f;
    CHECK(NextBeatWalkStep(facts, Limits()).reason == BeatAbort_ActorGone);
}

void ProgressResetsTheStallStreak()
{
    const char* name = "beat walk progress: getting closer by the step resets the streak; else it grows";
    BeatWalkProgress progress;
    CHECK(progress.closestMeters < 0.0f);
    CHECK(progress.sinceProgressSec == 0.0f);

    // The first reading is the reference, not progress and not a stall.
    progress = AdvanceBeatProgress(progress, 10.0f, 0.5f, 0.5f);
    CHECK(progress.closestMeters == 10.0f);
    CHECK(progress.sinceProgressSec == 0.0f);

    // Closer, but less than the step: the streak grows, the reference stays.
    progress = AdvanceBeatProgress(progress, 9.7f, 1.0f, 0.5f);
    CHECK(progress.closestMeters == 10.0f);
    CHECK(progress.sinceProgressSec == 1.0f);

    // A full step closer: back to zero, new reference.
    progress = AdvanceBeatProgress(progress, 9.4f, 1.0f, 0.5f);
    CHECK(progress.closestMeters == 9.4f);
    CHECK(progress.sinceProgressSec == 0.0f);

    // Walking away (another order) never counts as progress.
    progress = AdvanceBeatProgress(progress, 15.0f, 2.0f, 0.5f);
    CHECK(progress.closestMeters == 9.4f);
    CHECK(progress.sinceProgressSec == 2.0f);
}

void AnUnreadDistanceIsNoProgress()
{
    const char* name = "beat walk progress: an unread distance only grows the streak";
    BeatWalkProgress progress;
    progress = AdvanceBeatProgress(progress, -1.0f, 1.5f, 0.5f);
    CHECK(progress.closestMeters < 0.0f);
    CHECK(progress.sinceProgressSec == 1.5f);
    progress = AdvanceBeatProgress(progress, 8.0f, 1.0f, 0.5f);
    CHECK(progress.closestMeters == 8.0f);
    CHECK(progress.sinceProgressSec == 0.0f);
}

void ANegativeFrameAddsNothing()
{
    const char* name = "beat walk progress: a negative frame (clock reset) adds no time";
    BeatWalkProgress progress;
    progress = AdvanceBeatProgress(progress, 10.0f, 0.0f, 0.5f);
    progress = AdvanceBeatProgress(progress, 10.0f, -3.0f, 0.5f);
    CHECK(progress.sinceProgressSec == 0.0f);
}

void TheLimitsComeFromTheConfigClamped()
{
    const char* name = "beat walk limits: reach and walk time from the config, clamped; stall and step fixed";
    const BeatWalkLimits limits = BeatWalkLimitsFrom(4.0f, 30.0f);
    CHECK(limits.reachMeters == 4.0f);
    CHECK(limits.timeoutSec == 30.0f);
    CHECK(limits.stallSec == kBeatWalkStallSeconds);
    CHECK(limits.progressMeters == kBeatWalkProgressMeters);

    const BeatWalkLimits low = BeatWalkLimitsFrom(-5.0f, 0.0f);
    CHECK(low.reachMeters == kBeatReachMinMeters);
    CHECK(low.timeoutSec == kBeatWalkMinSeconds);

    const BeatWalkLimits high = BeatWalkLimitsFrom(500.0f, 99999.0f);
    CHECK(high.reachMeters == kBeatReachMaxMeters);
    CHECK(high.timeoutSec == kBeatWalkMaxSeconds);

    // The stall must fit inside the shortest walk, or a short walk could never stall.
    CHECK(kBeatWalkStallSeconds < kBeatWalkMinSeconds);
    CHECK(kBeatReachMinMeters <= kBeatReachDefaultMeters &&
          kBeatReachDefaultMeters <= kBeatReachMaxMeters);
    CHECK(kBeatWalkMinSeconds <= kBeatWalkDefaultSeconds &&
          kBeatWalkDefaultSeconds <= kBeatWalkMaxSeconds);
}

void OnlyAPersonComplains()
{
    const char* name = "beat feedback: a person complains aloud, an animal never";
    CHECK(BeatComplaintSpoken(false));
    CHECK(!BeatComplaintSpoken(true));
}

void WhatThePlayerIsToldOnAnAbort()
{
    const char* name = "beat feedback: every abort but a replacement and a load tells the player";
    CHECK(BeatAbortTellsPlayer(BeatAbort_ActorGone));
    CHECK(BeatAbortTellsPlayer(BeatAbort_ActorDown));
    CHECK(BeatAbortTellsPlayer(BeatAbort_NotAllowed));
    CHECK(BeatAbortTellsPlayer(BeatAbort_Resting));
    CHECK(BeatAbortTellsPlayer(BeatAbort_Stalled));
    CHECK(BeatAbortTellsPlayer(BeatAbort_Timeout));
    CHECK(BeatAbortTellsPlayer(BeatAbort_Failed));
    CHECK(!BeatAbortTellsPlayer(BeatAbort_Replaced));
    CHECK(!BeatAbortTellsPlayer(BeatAbort_Load));
    CHECK(!BeatAbortTellsPlayer(BeatAbort_None));
}

void TheWalkAbortsHaveTheirOwnLines()
{
    const char* name = "beat feedback: the walk's own aborts have a messages.json key and a fallback";
    CHECK(std::strcmp(BeatAbortMessageKey(BeatAbort_Timeout), "cage.beatAbort.timeout") == 0);
    CHECK(std::strcmp(BeatAbortMessageKey(BeatAbort_Stalled), "cage.beatAbort.stalled") == 0);
    CHECK(std::strcmp(BeatAbortMessageKey(BeatAbort_ActorDown), "cage.beatAbort.actor") == 0);
    CHECK(std::strcmp(BeatAbortMessageKey(BeatAbort_ActorGone), "cage.beatAbort.actor") == 0);
    CHECK(BeatAbortFallback(BeatAbort_Timeout)[0] != '\0');
    CHECK(BeatAbortFallback(BeatAbort_Stalled)[0] != '\0');
    CHECK(BeatAbortFallback(BeatAbort_ActorDown)[0] != '\0');
    // The cage's own lines (not allowed, resting, failed) are cage_commands'.
    CHECK(BeatAbortMessageKey(BeatAbort_NotAllowed)[0] == '\0');
    CHECK(BeatAbortMessageKey(BeatAbort_Resting)[0] == '\0');
    CHECK(BeatAbortMessageKey(BeatAbort_Failed)[0] == '\0');
    CHECK(BeatAbortText(BeatAbort_Timeout)[0] != '\0');
    CHECK(BeatAbortText(BeatAbort_Load)[0] != '\0');
}
void TheHitSoundTriesTheConfiguredNameFirst()
{
    const char* name = "beat hit sound: the configured event first, then Loot-Scoot-Execute's list, no repeats";
    const std::vector<std::string> plain = BeatHitSoundCandidates("Heavy_Hit");
    CHECK(!plain.empty());
    CHECK(plain[0] == "Heavy_Hit");
    int heavy = 0;
    for (size_t i = 0; i < plain.size(); ++i)
    {
        if (plain[i] == "Heavy_Hit") ++heavy;
    }
    CHECK(heavy == 1);
    CHECK(plain.size() == kBeatHitSoundFallbackCount);

    const std::vector<std::string> custom = BeatHitSoundCandidates("My_Punch");
    CHECK(custom.size() == kBeatHitSoundFallbackCount + 1);
    CHECK(custom[0] == "My_Punch");
    CHECK(custom[1] == "Heavy_Hit");

    // An empty name is no candidate: the list alone.
    const std::vector<std::string> none = BeatHitSoundCandidates("");
    CHECK(none.size() == kBeatHitSoundFallbackCount);
    CHECK(none[0] == "Heavy_Hit");
}
} // namespace

void RunBeatWalkRulesTests()
{
    TheReachIsInclusiveAndAnUnknownDistanceIsNeverInReach();
    InReachLandsAtOnce();
    FarAwayKeepsWalking();
    ArrivingWinsOverTheTimeoutAndTheStall();
    TheTimeoutAborts();
    NoProgressAborts();
    TheActorAndTheTargetComeFirst();
    AnActorGoneWinsOverEverythingElse();
    ProgressResetsTheStallStreak();
    AnUnreadDistanceIsNoProgress();
    ANegativeFrameAddsNothing();
    TheLimitsComeFromTheConfigClamped();
    OnlyAPersonComplains();
    WhatThePlayerIsToldOnAnAbort();
    TheWalkAbortsHaveTheirOwnLines();
    TheHitSoundTriesTheConfiguredNameFirst();
}
