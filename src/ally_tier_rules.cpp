#include "ally_tier_rules.h"

namespace totalrecruit
{
AllyTier AllyTierOf(int npcLevel, const AllyTiers& config)
{
    if (npcLevel >= config.legendFromLevel) return AllyTier_Legend;
    if (npcLevel >= config.eliteFromLevel)  return AllyTier_Elite;
    if (npcLevel >= config.highFromLevel)   return AllyTier_High;
    if (npcLevel >= config.midFromLevel)    return AllyTier_Mid;
    return AllyTier_Low;
}

float AllyTierMaxDiscount(AllyTier tier, const AllyTiers& config)
{
    switch (tier)
    {
    case AllyTier_Low:   return config.lowMaxDiscount;
    case AllyTier_Mid:   return config.midMaxDiscount;
    case AllyTier_High:  return config.highMaxDiscount;
    case AllyTier_Elite: return config.eliteMaxDiscount;
    case AllyTier_Legend:
    default:             return 0.0f;
    }
}

float FriendshipDiscount(AllyTier tier, float relation, const AllyTiers& config)
{
    const float earned = (relation > 0.0f ? relation : 0.0f) / 100.0f * config.discountScale;
    const float cap = AllyTierMaxDiscount(tier, config);
    float discount = earned < cap ? earned : cap;
    if (discount < 0.0f) discount = 0.0f;
    return discount;
}

namespace
{
AllyTierDecision Paid(AllyTier tier, float relation, const AllyTiers& config)
{
    AllyTierDecision d;
    d.mode = AllyTierMode_Paid;
    d.tier = tier;
    d.reason = AllyTierReason_FriendlyPrice;
    d.discountFraction = FriendshipDiscount(tier, relation, config);
    return d;
}

AllyTierDecision Other(AllyTierMode mode, AllyTier tier, AllyTierReason reason)
{
    AllyTierDecision d;
    d.mode = mode;
    d.tier = tier;
    d.reason = reason;
    return d;
}

// Good or Neutral at 80+: they come along for a while, but only behind a leader
// who is not too far below them.
AllyTierDecision TemporaryIfClose(int npcLevel, int leaderLevel, AllyTier tier,
                                  const AllyTiers& config)
{
    if (npcLevel - leaderLevel <= config.closeLevelGap)
    {
        return Other(AllyTierMode_Temporary, tier, AllyTierReason_Temporary);
    }
    return Other(AllyTierMode_Refuse, tier, AllyTierReason_LeaderTooWeak);
}
} // namespace

AllyTierDecision DecideAllyTier(int npcLevel, int leaderLevel, PersonalityGroup group,
                                float relation, const AllyTiers& config)
{
    const AllyTier tier = AllyTierOf(npcLevel, config);
    const bool goodOrNeutral = group == PersonalityGroup_Good || group == PersonalityGroup_Neutral;

    switch (tier)
    {
    case AllyTier_Low:
        if (group == PersonalityGroup_Good)
        {
            return Other(AllyTierMode_Free, tier, AllyTierReason_GoodFriend);
        }
        return Paid(tier, relation, config);

    case AllyTier_Mid:
    case AllyTier_High:
        return Paid(tier, relation, config);

    case AllyTier_Elite:
        if (group == PersonalityGroup_Greedy)
        {
            return Paid(tier, relation, config);
        }
        if (goodOrNeutral)
        {
            return TemporaryIfClose(npcLevel, leaderLevel, tier, config);
        }
        return Other(AllyTierMode_Refuse, tier, AllyTierReason_Cruel);

    case AllyTier_Legend:
    default:
        if (goodOrNeutral)
        {
            return TemporaryIfClose(npcLevel, leaderLevel, tier, config);
        }
        return Other(AllyTierMode_Refuse, tier,
                     group == PersonalityGroup_Greedy ? AllyTierReason_Greedy : AllyTierReason_Cruel);
    }
}

float AllyPriceMultiplier(const AllyTierDecision& decision)
{
    if (decision.mode != AllyTierMode_Paid)
    {
        return 1.0f;
    }
    return 1.0f - decision.discountFraction;
}

int DiscountPercent(float fraction)
{
    if (fraction <= 0.0f)
    {
        return 0;
    }
    return (int)(fraction * 100.0f + 0.5f);
}

int TemporaryDaysFromSeed(unsigned int seed, const AllyTiers& config)
{
    const int low = config.tempMemberMinDays;
    const int high = config.tempMemberMaxDays;
    if (high <= low)
    {
        return low;
    }
    const unsigned int span = (unsigned int)(high - low + 1);
    return low + (int)(seed % span);
}
} // namespace totalrecruit
