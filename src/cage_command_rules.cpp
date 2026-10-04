#include "cage_command_rules.h"

#include <cstdio>

namespace totalrecruit
{
const unsigned int kOrderWatchTimeoutMs = 1500;
// Two real minutes: the knockout order walks the actor to the cage and the door.
const unsigned int kOrderedKnockoutWindowMs = 120000;

bool CageCommandsAllowed(const CageCommandFacts& facts)
{
    // Hitting someone already unconscious teaches them nothing and only risks
    // killing them; outside the player's cage the engine has its own attack order.
    return facts.allowBeating && facts.inPlayerCage && !facts.down;
}

std::vector<ContextButtonKind> ComposeContextBlock(bool cageCommands)
{
    std::vector<ContextButtonKind> block;
    block.push_back(ContextButton_Talk);
    if (cageCommands)
    {
        block.push_back(ContextButton_Beat);
        block.push_back(ContextButton_Knockout);
    }
    return block;
}

OrderWatchVerdict JudgeOrderWatch(int taskBefore, int ordered, bool readNow, int taskNow,
                                  unsigned int elapsedMs, unsigned int timeoutMs)
{
    // Any change counts as taken: the first step of an order is often walking to
    // the target, which is a different task from the one ordered.
    if (readNow && (taskNow == ordered || taskNow != taskBefore))
    {
        return OrderWatch_Taken;
    }
    return elapsedMs >= timeoutMs ? OrderWatch_TimedOut : OrderWatch_Pending;
}

void OrderedKnockoutCredit::Remember(const CharKey& target, unsigned int nowMs)
{
    pending_[target] = nowMs;
}

bool OrderedKnockoutCredit::Claim(const CharKey& target, unsigned int nowMs)
{
    Pending::iterator it = pending_.find(target);
    if (it == pending_.end())
    {
        return false;
    }
    // Unsigned subtraction: correct across a GetTickCount wrap.
    const bool inWindow = (nowMs - it->second) < kOrderedKnockoutWindowMs;
    pending_.erase(it);
    return inWindow;
}

void OrderedKnockoutCredit::Clear()
{
    pending_.clear();
}

int OrderedKnockoutCredit::Size() const
{
    return (int)pending_.size();
}

int KnockoutPercent(float probability)
{
    float percent = (probability > 1.0f) ? probability : probability * 100.0f;
    if (percent < 0.0f) percent = 0.0f;
    if (percent > 100.0f) percent = 100.0f;
    return (int)(percent + 0.5f);
}

std::string KnockoutLabel(const std::string& base, bool haveOdds, float probability)
{
    if (!haveOdds)
    {
        return base;
    }
    char buffer[16];
    _snprintf_s(buffer, sizeof(buffer), _TRUNCATE, " %d%%", KnockoutPercent(probability));
    return base + buffer;
}

float SkillFraction(float statLevel)
{
    const float fraction = statLevel / 100.0f;
    if (fraction < 0.0f) return 0.0f;
    if (fraction > 1.0f) return 1.0f;
    return fraction;
}

// A bare-fist punch is in the low teens of blunt; this one only stands in when
// the engine's own number cannot be read.
const float kBeatDefaultBlunt = 15.0f;
// Half the death threshold: deep enough for the blow to knock a prisoner out
// (a torso collapses below zero), far enough from -max to never kill.
const float kBeatHealthFloorFraction = 0.5f;
const int kBeatTorsoPartType = 0;

namespace
{
float NotNegative(float value)
{
    return value > 0.0f ? value : 0.0f;
}
} // namespace

float BeatBlunt(bool readable, float cut, float blunt, float pierce)
{
    if (!readable)
    {
        return kBeatDefaultBlunt;
    }
    const float total = NotNegative(cut) + NotNegative(blunt) + NotNegative(pierce);
    return total > 0.0f ? total : kBeatDefaultBlunt;
}

float BeatHealthFloor(float maxHealth)
{
    return maxHealth > 0.0f ? -kBeatHealthFloorFraction * maxHealth : 0.0f;
}

float CapBeatBlunt(float blunt, float health, float maxHealth)
{
    const float room = NotNegative(health - BeatHealthFloor(maxHealth));
    const float wanted = NotNegative(blunt);
    return wanted < room ? wanted : room;
}

int PickBeatPart(const BeatPartFacts* parts, int count)
{
    if (parts == NULL)
    {
        return -1;
    }
    int firstFatal = -1;
    for (int i = 0; i < count; ++i)
    {
        if (!parts[i].present)
        {
            continue;
        }
        if (parts[i].type == kBeatTorsoPartType)
        {
            return i;
        }
        if (firstFatal < 0 && parts[i].fatal)
        {
            firstFatal = i;
        }
    }
    return firstFatal;
}
} // namespace totalrecruit
