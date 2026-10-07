#include "common.h"
#include "beat_strike.h"
#include "approach_driver.h"
#include "beat_strike_rules.h"
#include "beat_walk_sound.h"
#include "char_key_reader.h"
#include "config.h"
#include "log.h"
#include "npc_motion.h"

#include <kenshi/Animation/AnimationClass.h>

namespace totalrecruit
{
namespace
{
struct PendingStrike
{
    bool active;
    Character* actor;
    Character* target;
    CharKey actorKey;
    CharKey targetKey;
    std::string actorName;
    std::string targetName;
    std::string clip;
    BeatStrikeTiming timing;
    BeatWalkHandlers handlers;
    float startedSec;
    bool landed;
    bool clipChecked;
    bool clipPlaying;

    PendingStrike()
        : active(false), actor(NULL), target(NULL), startedSec(0.0f), landed(false),
          clipChecked(false), clipPlaying(false) {}
};

PendingStrike g_strikes[kMaxBeatWalks];
EventClock g_clock;
bool g_clipResultLogged = false;

// ---- engine calls, each in a POD-only guarded frame (C2712) ----------------
void SafeRunClip(Character* actor, const std::string& clip)
{
    if (actor == NULL)
    {
        return;
    }
    TR_TRY { actor->runSlaveAnim(clip, 1.0f, 0.0f); }
    TR_CATCH {}
}

void SafeEndClip(Character* actor, const std::string& clip)
{
    if (actor == NULL)
    {
        return;
    }
    TR_TRY
    {
        actor->endSlaveAnim(clip);
        AnimationClass* animation = actor->getAnimationClass();
        if (animation != NULL)
        {
            animation->endSlaveAnim(clip);
        }
    }
    TR_CATCH {}
}

bool SafeClipPlaying(Character* actor, const std::string& clip, bool* out)
{
    if (actor == NULL || out == NULL)
    {
        return false;
    }
    TR_TRY
    {
        AnimationClass* animation = actor->getAnimationClass();
        if (animation == NULL)
        {
            return false;
        }
        *out = animation->getAnimationPlaying_animName(clip) != NULL;
        return true;
    }
    TR_CATCH { return false; }
}
// ----------------------------------------------------------------------------

BeatStrikeTiming Timing()
{
    const Submission& sub = GetConfig().submission;
    return BeatStrikeTimingFrom(sub.beatAnimEnabled, sub.beatAnimClip, sub.beatAnimSeconds,
                                sub.beatWindupSeconds);
}

void LogStrike(const PendingStrike& strike, const std::string& what)
{
    if (GetConfig().probeOnClick)
    {
        LogInfo("[cage] " + strike.actorName + " -> " + strike.targetName + " beat strike " + what);
    }
}

void LogClipResult(const PendingStrike& strike)
{
    LogStrike(strike, strike.clipPlaying ? "clip playing" : "clip not found playing");
    if (g_clipResultLogged)
    {
        return;
    }
    g_clipResultLogged = true;
    LogInfo("[cage] beat anim: clip '" + strike.clip + "'" +
            (strike.clipPlaying ? " found playing" : " NOT found playing (the blow lands at once)"));
}

// The blow: lander, sound, report. Reported as Failed (false) when the lander
// says no.
bool Deliver(const PendingStrike& strike, Character* actor, Character* target)
{
    if (!strike.handlers.land(actor, target))
    {
        LogStrike(strike, std::string("aborted (") + BeatAbortText(BeatAbort_Failed) + ")");
        ReportBeat(strike.handlers, BeatOutcome_Aborted, BeatAbort_Failed, actor, target,
                   strike.actorName, strike.targetName);
        return false;
    }
    if (GetConfig().submission.beatHitSoundEnabled)
    {
        PlayBeatHitSound(target, GetConfig().submission.beatHitSound);
    }
    LogStrike(strike, "landed");
    ReportBeat(strike.handlers, BeatOutcome_Landed, BeatAbort_None, actor, target, strike.actorName,
               strike.targetName);
    return true;
}

BeatStrikeFacts ReadFacts(const PendingStrike& strike, Character* actor, Character* target, float now)
{
    BeatStrikeFacts facts;
    facts.actorResolves = actor != NULL;
    facts.actorUp = actor != NULL && NpcMotion::IsUp(actor);
    facts.targetResolves = target != NULL;
    facts.click = (target != NULL && strike.handlers.judge != NULL) ? strike.handlers.judge(target)
                                                                     : CageClick_NotAllowed;
    facts.landed = strike.landed;
    facts.elapsedSec = now - strike.startedSec;
    facts.clipChecked = strike.clipChecked;
    facts.clipPlaying = strike.clipPlaying;
    return facts;
}

// One step of one strike. Returns false when it is over (the slot is cleared by
// the caller). A Land is followed by one more question in the same frame, so a
// strike with no animation lands and ends at once.
bool StepOne(PendingStrike* strike, float now)
{
    Character* actor = StillSameCharacter(strike->actor, strike->actorKey);
    Character* target = StillSameCharacter(strike->target, strike->targetKey);

    if (strike->timing.animate && !strike->clipChecked && now > strike->startedSec && actor != NULL)
    {
        bool playing = false;
        strike->clipChecked = true;
        strike->clipPlaying = SafeClipPlaying(actor, strike->clip, &playing) && playing;
        LogClipResult(*strike);
    }

    for (int pass = 0; pass < 2; ++pass)
    {
        const BeatStrikeStep step = NextBeatStrikeStep(ReadFacts(*strike, actor, target, now),
                                                       strike->timing);
        switch (step.kind)
        {
        case StrikeStep_Sustain:
            SafeRunClip(actor, strike->clip);
            return true;
        case StrikeStep_Land:
            strike->landed = true;
            if (!Deliver(*strike, actor, target))
            {
                SafeEndClip(actor, strike->clip);
                return false;
            }
            continue;
        case StrikeStep_Abort:
            SafeEndClip(actor, strike->clip);
            LogStrike(*strike, std::string("aborted (") + BeatAbortText(step.reason) + ")");
            ReportBeat(strike->handlers, BeatOutcome_Aborted, step.reason, actor, target,
                       strike->actorName, strike->targetName);
            return false;
        case StrikeStep_End:
        default:
            if (strike->timing.animate)
            {
                SafeEndClip(actor, strike->clip);
                LogStrike(*strike, "ended");
            }
            return false;
        }
    }
    return true;
}

int FindSlot(const CharKey& actorKey)
{
    for (int i = 0; i < kMaxBeatWalks; ++i)
    {
        if (g_strikes[i].active && g_strikes[i].actorKey == actorKey)
        {
            return i;
        }
    }
    for (int i = 0; i < kMaxBeatWalks; ++i)
    {
        if (!g_strikes[i].active)
        {
            return i;
        }
    }
    return -1;
}

// Ends the clip of a strike that is dropped (replaced, or a load).
void Drop(PendingStrike* strike, BeatWalkAbort why)
{
    if (strike->active && strike->timing.animate)
    {
        SafeEndClip(StillSameCharacter(strike->actor, strike->actorKey), strike->clip);
    }
    if (strike->active)
    {
        LogStrike(*strike, std::string("aborted (") + BeatAbortText(why) + ")");
    }
    *strike = PendingStrike();
}

bool AnyActive()
{
    for (int i = 0; i < kMaxBeatWalks; ++i)
    {
        if (g_strikes[i].active)
        {
            return true;
        }
    }
    return false;
}
} // namespace

void ReportBeat(const BeatWalkHandlers& handlers, BeatOutcomeKind kind, BeatWalkAbort reason,
                Character* actor, Character* target, const std::string& actorName,
                const std::string& targetName)
{
    if (handlers.report == NULL)
    {
        return;
    }
    BeatOutcome outcome;
    outcome.kind = kind;
    outcome.reason = reason;
    outcome.actor = actor;
    outcome.target = target;
    outcome.actorName = actorName;
    outcome.targetName = targetName;
    handlers.report(outcome);
}

void StartBeatStrike(Character* actor, Character* target, const std::string& actorName,
                     const std::string& targetName, const BeatWalkHandlers& handlers)
{
    PendingStrike strike;
    strike.actor = actor;
    strike.target = target;
    strike.actorName = actorName;
    strike.targetName = targetName;
    strike.handlers = handlers;
    strike.timing = Timing();
    strike.clip = GetConfig().submission.beatAnimClip;

    const bool keyed = SafeCharKey(actor, &strike.actorKey) && SafeCharKey(target, &strike.targetKey);
    const int index = keyed ? FindSlot(strike.actorKey) : -1;
    if (index < 0)
    {
        // No key or no slot: the blow still lands, without the animation.
        strike.timing.animate = false;
        strike.landed = true;
        Deliver(strike, actor, target);
        return;
    }
    Drop(&g_strikes[index], BeatAbort_Replaced);

    g_clock.Advance();
    strike.startedSec = g_clock.Now();
    strike.active = true;
    if (strike.timing.animate)
    {
        SafeRunClip(actor, strike.clip);
        LogStrike(strike, "started (clip '" + strike.clip + "')");
    }
    g_strikes[index] = strike;
    // The first step now: without the animation it lands and ends in this call.
    if (!StepOne(&g_strikes[index], strike.startedSec))
    {
        g_strikes[index] = PendingStrike();
    }
}

void TickBeatStrikes()
{
    if (!AnyActive())
    {
        return;
    }
    g_clock.Advance();
    const float now = g_clock.Now();
    for (int i = 0; i < kMaxBeatWalks; ++i)
    {
        if (g_strikes[i].active && !StepOne(&g_strikes[i], now))
        {
            g_strikes[i] = PendingStrike();
        }
    }
}

void ForgetBeatStrikes()
{
    for (int i = 0; i < kMaxBeatWalks; ++i)
    {
        Drop(&g_strikes[i], BeatAbort_Load);
    }
    g_clock.Reset();
}
} // namespace totalrecruit
