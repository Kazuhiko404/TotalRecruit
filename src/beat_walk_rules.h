#pragma once

// PURE: Golpear walks first (beat-feedback T1). The selected character walks to
// the prisoner and the blow lands only once it is within reach; beat_walk.cpp is
// the engine side: it reads the facts every frame, asks these rules what they
// mean and acts.
//
//   InBeatReach           within reach of the prisoner (metres, inclusive).
//   NextBeatWalkStep      keep walking, land the blow, or abort and why.
//   AdvanceBeatProgress   the "no closer" streak that tells a stuck actor, or one
//                         the player sent somewhere else, from one still walking.
//   BeatWalkLimitsFrom    the config's reach and walk time, clamped.
//   BeatComplaintSpoken   who complains aloud when the blow lands (T2).
//   BeatAbort*            what the player is told when the walk ends with no blow.
//
// Why progress and not the actor's task: whether a plugin's MOVE_CUS_ORDERED reads
// back as the actor's current task on a player character is not proven in game
// (the knockout watch had to treat "any task change" as taken). Distance is: an
// actor who gets no closer for kBeatWalkStallSeconds -- blocked, ordered elsewhere
// by the player, or told to stand still -- is not walking to the cage any more.
//
// No Kenshi / MyGUI include: tested in tests/rules_tests/beat_walk_rules_tests.cpp.

#include "cage_command_rules.h"

#include <cstddef>
#include <string>
#include <vector>

namespace totalrecruit
{
// The config's reach (submissionBeatReachMeters) and walk time
// (submissionBeatWalkSeconds): defaults and the clamps config.cpp and the Mod Hub
// use. Real seconds that stop while the game is paused, like the submission
// event's walk-up.
const float kBeatReachDefaultMeters = 4.0f;
const float kBeatReachMinMeters = 1.0f;
const float kBeatReachMaxMeters = 15.0f;
const float kBeatWalkDefaultSeconds = 30.0f;
const float kBeatWalkMinSeconds = 5.0f;
const float kBeatWalkMaxSeconds = 300.0f;

// No closer by kBeatWalkProgressMeters for kBeatWalkStallSeconds in a row: the
// walk is over.
const float kBeatWalkStallSeconds = 4.0f;
const float kBeatWalkProgressMeters = 0.5f;

enum BeatWalkStepKind
{
    BeatStep_Walk,    // nothing to do this frame
    BeatStep_Land,    // within reach: stop, face the prisoner, land the blow
    BeatStep_Abort    // no blow (see BeatWalkAbort)
};

// Why a walk ended with no blow. NextBeatWalkStep checks them in this order.
enum BeatWalkAbort
{
    BeatAbort_None = 0,
    BeatAbort_ActorGone,    // the actor no longer resolves (zone unloaded, freed)
    BeatAbort_ActorDown,    // the actor is down, unconscious, dead or carried
    BeatAbort_NotAllowed,   // the prisoner no longer resolves or is no longer a
                            // valid target (freed, out of the cage, down, beating off)
    BeatAbort_Resting,      // Golpear started resting on them meanwhile (another
                            // character's blow landed first)
    BeatAbort_Timeout,      // the walk time ran out out of reach
    BeatAbort_Stalled,      // no closer for kBeatWalkStallSeconds
    // Set by the engine side, never by NextBeatWalkStep:
    BeatAbort_Failed,       // in reach, but the blow itself could not be landed
    BeatAbort_Replaced,     // the same actor was told to beat again
    BeatAbort_Load          // a game is loading
};

struct BeatWalkLimits
{
    float reachMeters;
    float timeoutSec;
    float stallSec;
    float progressMeters;

    BeatWalkLimits()
        : reachMeters(kBeatReachDefaultMeters), timeoutSec(kBeatWalkDefaultSeconds),
          stallSec(kBeatWalkStallSeconds), progressMeters(kBeatWalkProgressMeters) {}
};

// The config's values, clamped to the ranges above; the stall and the step fixed.
BeatWalkLimits BeatWalkLimitsFrom(float reachMeters, float walkSeconds);

// One frame's facts, read by beat_walk.cpp.
struct BeatWalkFacts
{
    bool actorResolves;      // the actor's pointer still carries its key
    bool actorUp;            // alive, conscious, on their feet, not carried
    bool targetResolves;     // the prisoner's pointer still carries its key
    CageClickVerdict click;  // JudgeCageClick on the prisoner now (Golpear)
    float distanceMeters;    // actor to prisoner; < 0 = could not be read
    float elapsedSec;        // walk seconds since the click (paused = no time)
    float sinceProgressSec;  // AdvanceBeatProgress's streak

    BeatWalkFacts()
        : actorResolves(false), actorUp(false), targetResolves(false), click(CageClick_NotAllowed),
          distanceMeters(-1.0f), elapsedSec(0.0f), sinceProgressSec(0.0f) {}
};

struct BeatWalkStep
{
    BeatWalkStepKind kind;
    BeatWalkAbort reason;    // BeatAbort_None unless kind == BeatStep_Abort
};

// Within reach: a distance that was read, at most `reachMeters`.
bool InBeatReach(float distanceMeters, float reachMeters);

// PURE: one step of the walk. Order: ActorGone, ActorDown, NotAllowed (the
// prisoner unresolved or not allowed), Resting, Land (in reach), Timeout
// (elapsed >= timeoutSec), Stalled (sinceProgress >= stallSec), else Walk.
// Arriving wins over a timeout or a stall on the same frame: the actor is there.
// The first step, at the click, is the same question: in reach lands at once.
BeatWalkStep NextBeatWalkStep(const BeatWalkFacts& facts, const BeatWalkLimits& limits);

// The streak of walk time the actor got no closer. The first distance read is the
// reference; a reading at least `progressMeters` closer than the closest so far
// resets the streak and becomes the new reference; anything else (no closer,
// farther, unread) adds `frameSec` (a negative frame adds nothing).
struct BeatWalkProgress
{
    float closestMeters;     // < 0 = nothing read yet
    float sinceProgressSec;

    BeatWalkProgress() : closestMeters(-1.0f), sinceProgressSec(0.0f) {}
};

BeatWalkProgress AdvanceBeatProgress(BeatWalkProgress progress, float distanceMeters,
                                     float frameSec, float progressMeters);

// T2: who says it aloud when the blow lands. A person complains (SpeakAloud stays
// silent on its own when they cannot speak, unconscious included); an animal
// never gets a speech bubble, like the duel narration (DuelRaisesHeadNotice).
bool BeatComplaintSpoken(bool animal);

// Whether an abort is told to the player. A replacement is not (the new Golpear
// is the answer) and neither is a load (the world the walk was in is gone).
bool BeatAbortTellsPlayer(BeatWalkAbort reason);

// The walk's own lines in messages.json ("cage.beatAbort.*", with {actor} and
// {name}) and their built-in fallbacks; "" for the aborts that reuse the cage's
// lines (NotAllowed, Resting, Failed: cage_commands.h CageCommandProblem) and for
// those never told.
const char* BeatAbortMessageKey(BeatWalkAbort reason);
const char* BeatAbortFallback(BeatWalkAbort reason);

// The abort's short English name, for the "[cage]" log.
const char* BeatAbortText(BeatWalkAbort reason);

// T5: the hit sound on the prisoner when the blow lands
// (Character::audioEvent(name, SOUNDRANGE_ALWAYS), as Loot-Scoot-Execute plays its
// execute sound, Loot-Scoot-Execute.cpp kExecuteKillSoundEventCandidates). The
// event names to try, in order: the configured one (submissionBeatHitSound; ""
// is none), then that list, without repeats. audioEvent returns whether the
// event played, so the first one accepted is the sound and the rest are skipped.
const size_t kBeatHitSoundFallbackCount = 10;
std::vector<std::string> BeatHitSoundCandidates(const std::string& configured);
} // namespace totalrecruit
