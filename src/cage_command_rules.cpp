#include "cage_command_rules.h"

#include <cstdio>

namespace totalrecruit
{
const unsigned int kOrderWatchTimeoutMs = 1500;
// Two real minutes: the actor walks to the cage, the door, then the blows.
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
} // namespace totalrecruit
