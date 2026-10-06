#include "common.h"
#include "beat_walk.h"
#include "approach_driver.h"
#include "beat_walk_sound.h"
#include "char_key_reader.h"
#include "config.h"
#include "engine_reads.h"
#include "log.h"
#include "npc_motion.h"

namespace totalrecruit
{
namespace
{
// No engine call is guarded here: every engine touch goes through a Safe* read
// or an NpcMotion call, which hold their own guards (C2712 never applies).

struct PendingBeat
{
    bool active;
    Character* actor;
    Character* target;
    CharKey actorKey;
    CharKey targetKey;
    std::string actorName;
    std::string targetName;
    // Walk seconds (g_clock) at the click and at the last step.
    float startedSec;
    float lastSec;
    BeatWalkProgress progress;
    BeatWalkHandlers handlers;

    PendingBeat() : active(false), actor(NULL), target(NULL), startedSec(0.0f), lastSec(0.0f) {}
};

PendingBeat g_pending[kMaxBeatWalks];
EventClock g_clock;

BeatWalkLimits Limits()
{
    const Config& config = GetConfig();
    return BeatWalkLimitsFrom(config.submission.beatReachMeters, config.submission.beatWalkSeconds);
}

void LogWalk(const PendingBeat& beat, const std::string& what)
{
    if (GetConfig().probeOnClick)
    {
        LogInfo("[cage] " + beat.actorName + " -> " + beat.targetName + " beat walk " + what);
    }
}

std::string Metres(float distance)
{
    char text[32];
    _snprintf_s(text, sizeof(text), _TRUNCATE, "%.1f m", distance);
    return text;
}

// The frame's facts for one strike, with the characters as they resolve now.
BeatWalkFacts ReadFacts(const PendingBeat& beat, Character* actor, Character* target, float now)
{
    BeatWalkFacts facts;
    facts.actorResolves = actor != NULL;
    facts.actorUp = actor != NULL && NpcMotion::IsUp(actor);
    facts.targetResolves = target != NULL;
    facts.click = target != NULL ? beat.handlers.judge(target) : CageClick_NotAllowed;
    float distance = -1.0f;
    if (actor != NULL && target != NULL && !NpcMotion::DistanceMeters(actor, target, &distance))
    {
        distance = -1.0f;
    }
    facts.distanceMeters = distance;
    facts.elapsedSec = now - beat.startedSec;
    facts.sinceProgressSec = beat.progress.sinceProgressSec;
    return facts;
}

void Report(const PendingBeat& beat, BeatOutcomeKind kind, BeatWalkAbort reason,
            Character* actor, Character* target)
{
    if (beat.handlers.report == NULL)
    {
        return;
    }
    BeatOutcome outcome;
    outcome.kind = kind;
    outcome.reason = reason;
    outcome.actor = actor;
    outcome.target = target;
    outcome.actorName = beat.actorName;
    outcome.targetName = beat.targetName;
    beat.handlers.report(outcome);
}

// In reach: stop, face the prisoner, land. False when the blow could not land.
bool Land(const PendingBeat& beat, Character* actor, Character* target, float distance)
{
    NpcMotion::StopWalking(actor, true);
    NpcMotion::Face(actor, target);
    LogWalk(beat, "arrived (" + Metres(distance) + ")");
    if (!beat.handlers.land(actor, target))
    {
        return false;
    }
    if (GetConfig().submission.beatHitSoundEnabled)
    {
        PlayBeatHitSound(target, actor, GetConfig().submission.beatHitSound);
    }
    return true;
}

int FindSlot(const CharKey& actorKey)
{
    for (int i = 0; i < kMaxBeatWalks; ++i)
    {
        if (g_pending[i].active && g_pending[i].actorKey == actorKey)
        {
            return i;
        }
    }
    for (int i = 0; i < kMaxBeatWalks; ++i)
    {
        if (!g_pending[i].active)
        {
            return i;
        }
    }
    return -1;
}

bool AnyPending()
{
    for (int i = 0; i < kMaxBeatWalks; ++i)
    {
        if (g_pending[i].active)
        {
            return true;
        }
    }
    return false;
}

// One step of one strike. Copied out and closed before acting on an end, so
// nothing the lander or the reporter calls can run it twice.
void StepOne(PendingBeat* slot, float now, const BeatWalkLimits& limits)
{
    Character* actor = StillSameCharacter(slot->actor, slot->actorKey);
    Character* target = StillSameCharacter(slot->target, slot->targetKey);

    BeatWalkFacts facts = ReadFacts(*slot, actor, target, now);
    slot->progress = AdvanceBeatProgress(slot->progress, facts.distanceMeters, now - slot->lastSec,
                                         limits.progressMeters);
    slot->lastSec = now;
    facts.sinceProgressSec = slot->progress.sinceProgressSec;

    const BeatWalkStep step = NextBeatWalkStep(facts, limits);
    if (step.kind == BeatStep_Walk)
    {
        return;
    }
    const PendingBeat beat = *slot;
    *slot = PendingBeat();
    if (step.kind == BeatStep_Land)
    {
        if (Land(beat, actor, target, facts.distanceMeters))
        {
            Report(beat, BeatOutcome_Landed, BeatAbort_None, actor, target);
        }
        else
        {
            LogWalk(beat, std::string("aborted (") + BeatAbortText(BeatAbort_Failed) + ")");
            Report(beat, BeatOutcome_Aborted, BeatAbort_Failed, actor, target);
        }
        return;
    }
    LogWalk(beat, std::string("aborted (") + BeatAbortText(step.reason) + ")");
    Report(beat, BeatOutcome_Aborted, step.reason, actor, target);
}
} // namespace

BeatWalkStart StartBeatWalk(Character* actor, Character* target, const BeatWalkHandlers& handlers)
{
    if (actor == NULL || target == NULL || handlers.judge == NULL || handlers.land == NULL)
    {
        return BeatWalkStart_Failed;
    }
    PendingBeat beat;
    if (!SafeCharKey(actor, &beat.actorKey) || !SafeCharKey(target, &beat.targetKey))
    {
        LogError("[cage] " + CharacterName(actor) + " -> " + CharacterName(target) +
                 " beat walk failed: character key unreadable");
        return BeatWalkStart_Failed;
    }
    const int index = FindSlot(beat.actorKey);
    if (index < 0)
    {
        LogError("[cage] " + CharacterName(actor) + " -> " + CharacterName(target) +
                 " beat walk failed: too many pending strikes");
        return BeatWalkStart_Failed;
    }
    if (g_pending[index].active)
    {
        // The same actor, told again: the old strike ends silently.
        LogWalk(g_pending[index], std::string("aborted (") + BeatAbortText(BeatAbort_Replaced) + ")");
        g_pending[index] = PendingBeat();
    }

    // An idle clock is advanced at the click so the gap since the last strike
    // counts as one frame at most (AdvanceEventClock caps it).
    g_clock.Advance();
    const float now = g_clock.Now();
    beat.active = true;
    beat.actor = actor;
    beat.target = target;
    beat.actorName = CharacterName(actor);
    beat.targetName = CharacterName(target);
    beat.startedSec = now;
    beat.lastSec = now;
    beat.handlers = handlers;

    // The click is the first step: in reach lands at once, the same question as
    // every later frame.
    const BeatWalkLimits limits = Limits();
    BeatWalkFacts facts = ReadFacts(beat, actor, target, now);
    beat.progress = AdvanceBeatProgress(beat.progress, facts.distanceMeters, 0.0f,
                                        limits.progressMeters);
    const BeatWalkStep step = NextBeatWalkStep(facts, limits);
    if (step.kind == BeatStep_Land)
    {
        if (!Land(beat, actor, target, facts.distanceMeters))
        {
            return BeatWalkStart_Failed;
        }
        Report(beat, BeatOutcome_Landed, BeatAbort_None, actor, target);
        return BeatWalkStart_Landed;
    }
    if (step.kind == BeatStep_Abort)
    {
        LogWalk(beat, std::string("aborted (") + BeatAbortText(step.reason) + ")");
        Report(beat, BeatOutcome_Aborted, step.reason, actor, target);
        return BeatWalkStart_Ended;
    }

    if (!NpcMotion::MoveTo(actor, target))
    {
        LogError("[cage] " + beat.actorName + " -> " + beat.targetName +
                 " beat walk failed: move order raised");
        return BeatWalkStart_Failed;
    }
    g_pending[index] = beat;
    LogWalk(beat, "started (" + Metres(facts.distanceMeters) + ")");
    return BeatWalkStart_Walking;
}

void TickBeatWalks()
{
    if (!AnyPending())
    {
        return;
    }
    g_clock.Advance();
    const float now = g_clock.Now();
    const BeatWalkLimits limits = Limits();
    for (int i = 0; i < kMaxBeatWalks; ++i)
    {
        if (g_pending[i].active)
        {
            StepOne(&g_pending[i], now, limits);
        }
    }
}

void ForgetBeatWalks()
{
    for (int i = 0; i < kMaxBeatWalks; ++i)
    {
        if (g_pending[i].active)
        {
            LogWalk(g_pending[i], std::string("aborted (") + BeatAbortText(BeatAbort_Load) + ")");
        }
        g_pending[i] = PendingBeat();
    }
    g_clock.Reset();
}
} // namespace totalrecruit
