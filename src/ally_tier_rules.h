#pragma once

// What an allied NPC asks of the player in the Balanced preset, without the engine.
//
// An ally (relation >= friendlyRelation) used to be one daily roll whatever their
// level. In Balanced their level and their personality group (personality_groups.h)
// now decide the deal, and friendship takes money off the price:
//
//   NPC level   Group            Deal
//   below 20    Good             Free (today's ally roll still decides)
//   below 20    others           Paid, discount up to lowMaxDiscount (20%)
//   20 - 49     everyone         Paid, discount up to midMaxDiscount (20%)
//   50 - 79     everyone         Paid, discount up to highMaxDiscount (10%)
//   80 - 99     Greedy           Paid, discount up to eliteMaxDiscount (10%)
//   80 - 99     Good / Neutral   Temporary member, only if close in level
//   80 - 99     Bad              Refuse
//   100+        Good / Neutral   Temporary member, only if close in level
//   100+        Greedy / Bad     Refuse
//
// Every cut level belongs to the tier it starts (config.h AllyTiers). "Close in
// level" is npcLevel - leaderLevel <= closeLevelGap; further than that the NPC
// refuses because the leader is too weak to be followed.
//
// discount = min(tierMax, max(0, relation) / 100 * discountScale).

#include "config.h"
#include "personality_groups.h"

namespace totalrecruit
{
enum AllyTierMode
{
    AllyTierMode_Free,
    AllyTierMode_Paid,
    AllyTierMode_Temporary,
    AllyTierMode_Refuse
};

enum AllyTier
{
    AllyTier_Low,     // below midFromLevel
    AllyTier_Mid,     // midFromLevel .. highFromLevel - 1
    AllyTier_High,    // highFromLevel .. eliteFromLevel - 1
    AllyTier_Elite,   // eliteFromLevel .. legendFromLevel - 1
    AllyTier_Legend   // legendFromLevel and above
};

// Why the tier came out the way it did; the route turns it into a ReasonKey.
enum AllyTierReason
{
    AllyTierReason_GoodFriend,      // Free
    AllyTierReason_FriendlyPrice,   // Paid
    AllyTierReason_Temporary,       // Temporary
    AllyTierReason_LeaderTooWeak,   // Refuse: a temporary candidate, too far above the leader
    AllyTierReason_Cruel,           // Refuse: Bad at 80+
    AllyTierReason_Greedy           // Refuse: Greedy at 100+
};

struct AllyTierDecision
{
    AllyTierMode mode;
    // 0..tierMax. Only a Paid deal carries one; every other mode is 0.
    float discountFraction;
    AllyTier tier;
    AllyTierReason reason;

    AllyTierDecision()
        : mode(AllyTierMode_Free), discountFraction(0.0f), tier(AllyTier_Low),
          reason(AllyTierReason_GoodFriend)
    {
    }
};

// The tier a level falls in (cut levels belong to the tier they start).
AllyTier AllyTierOf(int npcLevel, const AllyTiers& config);

// The most friendship can take off in this tier.
float AllyTierMaxDiscount(AllyTier tier, const AllyTiers& config);

// min(tierMax, max(0, relation) / 100 * discountScale), never negative.
float FriendshipDiscount(AllyTier tier, float relation, const AllyTiers& config);

AllyTierDecision DecideAllyTier(int npcLevel, int leaderLevel, PersonalityGroup group,
                                float relation, const AllyTiers& config);

// The price multiplier a decision stands for: 1 - discount for Paid, 1.0 otherwise.
// It is what character_facts puts in PriceInputs.factionMultiplier for a Balanced
// ally, where that multiplier is 1.0 otherwise (Vanilla+ only), so nothing counts
// twice.
float AllyPriceMultiplier(const AllyTierDecision& decision);

// The discount as a whole percent, rounded (0.14 -> 14).
int DiscountPercent(float fraction);

// Keeps the draw of a temporary member's days apart from the join and haggle rolls
// of the same NPC on the same day ("TEMP").
const unsigned int kTempDaysSalt = 0x54454D50u;

// How many days a temporary member stays: a seed (DailySeed under kTempDaysSalt)
// mapped onto [config.tempMemberMinDays, config.tempMemberMaxDays], both inclusive.
// A max below the min reads as the min.
int TemporaryDaysFromSeed(unsigned int seed, const AllyTiers& config);
} // namespace totalrecruit
