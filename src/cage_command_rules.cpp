#include "cage_command_rules.h"

#include <cstdio>

namespace totalrecruit
{
const unsigned int kOrderWatchTimeoutMs = 1500;
// Two real minutes: the knockout order walks the actor to the cage and the door.
const unsigned int kOrderedKnockoutWindowMs = 120000;
// Five real seconds: the blow's knockout, if it comes, comes at once.
const unsigned int kBeatCreditWindowMs = 5000;

bool CageCommandsAllowed(const CageCommandFacts& facts)
{
    // Hitting someone already unconscious teaches them nothing and only risks
    // killing them; outside the player's cage the engine has its own attack order.
    return facts.allowBeating && facts.inPlayerCage && !facts.down;
}

bool BeatReady(const CageCommandFacts& facts)
{
    return CageCommandsAllowed(facts) && facts.beatHoursLeft <= 0.0f;
}

bool KnockoutReady(const CageCommandFacts& facts)
{
    return CageCommandsAllowed(facts) && facts.knockoutHoursLeft <= 0.0f;
}

CageClickVerdict JudgeCageClick(const CageCommandFacts& facts, bool beat)
{
    if (!CageCommandsAllowed(facts))
    {
        return CageClick_NotAllowed;
    }
    const float hoursLeft = beat ? facts.beatHoursLeft : facts.knockoutHoursLeft;
    return hoursLeft > 0.0f ? CageClick_Resting : CageClick_Go;
}

void CageCooldownLedger::Remember(const CharKey& target, double nowHours)
{
    last_[target] = nowHours;
}

float CageCooldownLedger::HoursLeft(const CharKey& target, double nowHours,
                                    float cooldownHours) const
{
    Last::const_iterator it = last_.find(target);
    if (it == last_.end() || cooldownHours <= 0.0f)
    {
        return 0.0f;
    }
    const double since = nowHours - it->second;
    if (since < 0.0)
    {
        return 0.0f;
    }
    const double left = (double)cooldownHours - since;
    return left > 0.0 ? (float)left : 0.0f;
}

void CageCooldownLedger::Clear()
{
    last_.clear();
}

int RestHoursShown(float hoursLeft)
{
    if (hoursLeft <= 0.0f)
    {
        return 0;
    }
    const int whole = (int)hoursLeft;
    return ((float)whole < hoursLeft) ? whole + 1 : whole;
}

std::string RestLabel(const std::string& base, float hoursLeft)
{
    const int hours = RestHoursShown(hoursLeft);
    if (hours <= 0)
    {
        return base;
    }
    char buffer[24];
    _snprintf_s(buffer, sizeof(buffer), _TRUNCATE, " (%d h)", hours);
    return base + buffer;
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

unsigned int CreditWindowMs(OrderedCreditKind kind)
{
    switch (kind)
    {
    case OrderedCredit_Beat:     return kBeatCreditWindowMs;
    case OrderedCredit_Knockout: return kOrderedKnockoutWindowMs;
    case OrderedCredit_None:
    default:                     return 0u;
    }
}

void OrderedKnockoutCredit::Remember(const CharKey& target, unsigned int nowMs,
                                     OrderedCreditKind kind)
{
    Entry entry;
    entry.atMs = nowMs;
    entry.kind = kind;
    pending_[target] = entry;
}

OrderedCreditKind OrderedKnockoutCredit::Claim(const CharKey& target, unsigned int nowMs)
{
    Pending::iterator it = pending_.find(target);
    if (it == pending_.end())
    {
        return OrderedCredit_None;
    }
    // Unsigned subtraction: correct across a GetTickCount wrap.
    const Entry entry = it->second;
    pending_.erase(it);
    const bool inWindow = (nowMs - entry.atMs) < CreditWindowMs(entry.kind);
    return inWindow ? entry.kind : OrderedCredit_None;
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

std::string KnockoutCaption(const std::string& base, float hoursLeft, bool haveOdds,
                            float probability)
{
    return hoursLeft > 0.0f ? RestLabel(base, hoursLeft)
                            : KnockoutLabel(base, haveOdds, probability);
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

float BeatHeldHealth(float before, float after, float floor)
{
    if (after >= floor || before <= floor)
    {
        return after;
    }
    return floor;
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
