#pragma once

// Golpear's strike at the cage, engine side (priority-one C). beat_walk hands
// the strike over here once the character arrived (or at the click, already in
// reach): the punch clip plays on the actor, the blow lands after the wind-up,
// and the clip ends after its duration (beat_strike_rules.h decides each frame).
//
// Landing the blow is the one place that does it for Golpear: the lander
// (BeatWalkHandlers::land, cage_commands LandBeatHit), the hit sound
// (beat_walk_sound) and the report (BeatWalkHandlers::report: the complaint and
// the roller line, beat_walk_feedback). A blow that could not land is reported as
// BeatAbort_Failed; an abort before the blow (actor down or gone, prisoner no
// longer valid or resting) is reported with its reason and lands nothing.
//
// One strike per actor (up to kMaxBeatWalks), keyed by CharKey and re-resolved
// every frame (StillSameCharacter). A new strike by the same actor replaces the
// old one silently (its clip is ended). Every way out ends the clip on both the
// Character and its AnimationClass: the end of the clip, an abort, a replacement,
// and a load (ForgetBeatStrikes).
//
// Engine calls (all guarded, POD-only frames): Character::runSlaveAnim(clip, 1,
// 0) every frame, Character::endSlaveAnim + AnimationClass::endSlaveAnim,
// Character::getAnimationClass and AnimationClassBase::getAnimationPlaying_animName
// (exports checked in the game's KenshiLib.dll with dumpbin).
//
// Log: once per session, always, "[cage] beat anim: clip '<clip>' found playing"
// or "... NOT found playing (the blow lands at once)"; with probeOnClick, per
// strike "[cage] <actor> -> <target> beat strike started / landed / ended /
// aborted (<why>)".

#include "beat_walk.h"

class Character;

namespace totalrecruit
{
// Starts the strike of `actor` on `target` (both resolved, the actor standing at
// the cage and facing the prisoner). `actorName` / `targetName` are the names read
// at the click. Without the animation it lands and reports at once.
void StartBeatStrike(Character* actor, Character* target, const std::string& actorName,
                     const std::string& targetName, const BeatWalkHandlers& handlers);

// The frame tick (from TickCageCommands): one step of every strike.
void TickBeatStrikes();

// Load boundary: ends every clip and drops every strike, with no message.
void ForgetBeatStrikes();

// Tells the reporter (handlers.report, NULL = nobody) how a strike or a walk ended.
void ReportBeat(const BeatWalkHandlers& handlers, BeatOutcomeKind kind, BeatWalkAbort reason,
                Character* actor, Character* target, const std::string& actorName,
                const std::string& targetName);
} // namespace totalrecruit
