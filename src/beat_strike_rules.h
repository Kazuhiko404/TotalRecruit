#pragma once

// PURE: Golpear's strike once the character is at the cage (priority-one C).
// beat_strike.cpp is the engine side.
//
// Vanilla Kenshi has no punch ANIMATION record; its unarmed strikes are
// COMBAT_TECHNIQUE records in rebirth.mod, and the "ma badpunch" technique plays
// the clip "badpunch". The proven way to play a clip on a character
// (TotalTraining anim_player.cpp, Loot-Scoot-Execute) is
// Character::runSlaveAnim(clip, 1, 0) re-requested every frame -- a run* request
// lasts one frame -- and then endSlaveAnim(clip) on both the Character and its
// AnimationClass (a missing end freezes the pose).
//
// So the strike is: start the clip, keep it alive every frame, land the blow
// (damage, stress, complaint, sound, message) after the wind-up, end the clip
// after its duration. The first frame after the start asks the engine whether the
// clip is playing (AnimationClassBase::getAnimationPlaying_animName): a clip that
// is not found lands at once, which is the behaviour before the animation, and the
// clip is still ended at its time (harmless if nothing played).
//
// NextBeatStrikeStep, in order. Before the blow: actor gone, actor down, prisoner
// gone or no longer a valid target, Golpear resting (another blow landed first)
// abort with no blow; then no animation, or a clip not found, lands at once; then
// the wind-up lands. After the blow: no animation, the actor gone or down, or the
// duration reached end the clip; else it is sustained.
//
// No Kenshi / MyGUI include: tested in tests/rules_tests/beat_strike_rules_tests.cpp.

#include "beat_walk_rules.h"

#include <string>

namespace totalrecruit
{
// The config's clip (submissionBeatAnimClip), duration (submissionBeatAnimSeconds)
// and wind-up (submissionBeatWindupSeconds): defaults and clamps for config.cpp.
// Real seconds that stop while the game is paused.
extern const char* const kBeatAnimDefaultClip;
const float kBeatAnimDefaultSeconds = 0.9f;
const float kBeatAnimMinSeconds = 0.2f;
const float kBeatAnimMaxSeconds = 3.0f;
const float kBeatWindupDefaultSeconds = 0.35f;
const float kBeatWindupMinSeconds = 0.0f;
const float kBeatWindupMaxSeconds = kBeatAnimMaxSeconds;

struct BeatStrikeTiming
{
    bool animate;      // the animation is on and has a clip
    float animSec;     // how long the clip is kept alive
    float windupSec;   // when the blow lands, at most animSec

    BeatStrikeTiming() : animate(false), animSec(kBeatAnimDefaultSeconds), windupSec(kBeatWindupDefaultSeconds) {}
};

// The config's values, clamped; the wind-up never past the clip's duration.
BeatStrikeTiming BeatStrikeTimingFrom(bool enabled, const std::string& clip, float animSec,
                                      float windupSec);

enum BeatStrikeStepKind
{
    StrikeStep_Sustain,  // keep the clip alive this frame
    StrikeStep_Land,     // land the blow now (the clip goes on)
    StrikeStep_End,      // end the clip: the strike is over
    StrikeStep_Abort     // no blow: end the clip, report why (BeatStrikeStep::reason)
};

struct BeatStrikeFacts
{
    bool actorResolves;
    bool actorUp;
    bool targetResolves;
    CageClickVerdict click;   // JudgeCageClick on the prisoner now (Golpear)
    bool landed;              // the blow already landed
    float elapsedSec;         // since the strike started
    bool clipChecked;         // the engine was asked whether the clip plays
    bool clipPlaying;         // and it said yes

    BeatStrikeFacts()
        : actorResolves(false), actorUp(false), targetResolves(false), click(CageClick_NotAllowed),
          landed(false), elapsedSec(0.0f), clipChecked(false), clipPlaying(false) {}
};

struct BeatStrikeStep
{
    BeatStrikeStepKind kind;
    BeatWalkAbort reason;   // BeatAbort_None unless kind == StrikeStep_Abort
};

BeatStrikeStep NextBeatStrikeStep(const BeatStrikeFacts& facts, const BeatStrikeTiming& timing);
} // namespace totalrecruit
