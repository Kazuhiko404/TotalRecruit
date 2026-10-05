#include "common.h"
#include "approach_driver.h"
#include "submission_rules.h"
#include "npc_motion.h"
#include "pacify.h"
#include "engine_reads.h"
#include "char_key_reader.h"
#include "offer_popup.h"
#include "log.h"

#include <cstdio>

namespace totalrecruit
{
namespace
{
const int kMaxSquad = 256;

ApproachOwner g_slot = ApproachOwner_None;

Character* Leader()
{
    Character* leader = NULL;
    return SafePlayerLeader(&leader) ? leader : NULL;
}

void Log(const char* tag, const std::string& line)
{
    LogInfo(std::string(tag) + " " + line);
}
} // namespace

// ------------------------------------------------------------------ The slot
bool ClaimApproachSlot(ApproachOwner owner)
{
    if (g_slot != ApproachOwner_None && g_slot != owner)
        return false;
    g_slot = owner;
    return true;
}

void ReleaseApproachSlot(ApproachOwner owner)
{
    if (g_slot == owner)
        g_slot = ApproachOwner_None;
}

bool ApproachSlotFreeFor(ApproachOwner owner)
{
    return g_slot == ApproachOwner_None || g_slot == owner;
}

const char* ApproachOwnerName(ApproachOwner owner)
{
    switch (owner)
    {
    case ApproachOwner_Submission: return "submission";
    case ApproachOwner_Helper:     return "helper";
    default:                       return "none";
    }
}

// ----------------------------------------------------------------- The clock
EventClock::EventClock() : seconds_(0.0f), lastTick_(0), haveLastTick_(false) {}

void EventClock::Advance()
{
    const DWORD now = GetTickCount();
    if (!haveLastTick_)
    {
        haveLastTick_ = true;
        lastTick_ = now;
        return;
    }
    const DWORD elapsed = now - (DWORD)lastTick_;
    lastTick_ = now;
    bool paused = false;
    SafeIsPaused(&paused);
    seconds_ = AdvanceEventClock(seconds_, (unsigned int)elapsed, paused);
}

float EventClock::Now() const
{
    return seconds_;
}

void EventClock::Reset()
{
    seconds_ = 0.0f;
    lastTick_ = 0;
    haveLastTick_ = false;
}

// ---------------------------------------------------------------- The walker
WalkUp::WalkUp()
    : npc(NULL), key(), name(), phaseStart(0.0f), lastReassert(0.0f), standing(false), strikes(0)
{
}

Character* WalkUpNpc(const WalkUp& walk)
{
    return StillSameCharacter(walk.npc, walk.key);
}

void ReassertCalm(WalkUp* walk, Character* npc)
{
    if (npc == NULL)
        return;
    walk->strikes = NextCombatStrikes(walk->strikes, SafeInCombat(npc));
    Character* squad[kMaxSquad];
    const int count = SafePlayerCharacters(squad, kMaxSquad);
    SafeCalmToward(npc, squad, count);
}

bool BeginWalkerInPlace(WalkUp* walk, Character* npc, float now)
{
    CharKey key;
    if (npc == NULL || !SafeCharKey(npc, &key))
        return false;
    *walk = WalkUp();
    walk->npc = npc;
    walk->key = key;
    walk->name = CharacterName(npc);
    walk->phaseStart = now;
    walk->lastReassert = now;

    ReassertCalm(walk, npc);
    walk->strikes = 0;
    return true;
}

bool BeginWalkUp(WalkUp* walk, Character* npc, Character* leader, float now, bool* moving)
{
    if (!BeginWalkerInPlace(walk, npc, now))
        return false;
    *moving = NpcMotion::MoveTo(npc, leader);
    return true;
}

ApproachAction StepWalkUp(WalkUp* walk, float now, const ApproachLimits& limits, const char* tag)
{
    Character* npc = WalkUpNpc(*walk);
    Character* leader = Leader();

    ApproachInput in;
    in.npcValid = NpcMotion::IsUp(npc);
    in.targetValid = NpcMotion::IsUp(leader);
    in.elapsedSec = now - walk->phaseStart;
    in.sinceLastRepathSec = now - walk->lastReassert;
    in.combatStarted = CalmBroken(walk->strikes);
    in.targetBusy = IsOfferOpen() || SafeInCombat(leader);
    float meters = -1.0f;
    if (in.npcValid && in.targetValid && NpcMotion::DistanceMeters(npc, leader, &meters))
        in.distanceMeters = meters;

    const ApproachAction action = ApproachStep(in, limits);
    switch (action.kind)
    {
    case Approach_Repath:
        ReassertCalm(walk, npc);
        walk->standing = false;
        if (!NpcMotion::MoveTo(npc, leader))
            Log(tag, walk->name + ": move order FAILED");
        walk->lastReassert = now;
        break;

    case Approach_Stand:
        ReassertCalm(walk, npc);
        if (walk->standing)
        {
            NpcMotion::Stand(npc, leader);
        }
        else
        {
            NpcMotion::StopWalking(npc, true);
            walk->standing = NpcMotion::BeginStand(npc, leader);
            Log(tag, walk->name + ": arrived, waits for the window to be free");
        }
        walk->lastReassert = now;
        break;

    case Approach_Arrive:
    {
        NpcMotion::StopWalking(npc, true);
        ReassertCalm(walk, npc);
        NpcMotion::BeginStand(npc, leader);
        char note[48] = { 0 };
        _snprintf_s(note, sizeof(note), _TRUNCATE, " (%.1f s)", in.elapsedSec);
        Log(tag, walk->name + ": " + action.text + note);
        break;
    }

    default:
        break;
    }
    return action;
}
} // namespace totalrecruit
