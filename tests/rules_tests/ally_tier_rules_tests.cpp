// Tests for src/ally_tier_rules.cpp: what an allied NPC asks of the player in
// Balanced, by their level and personality group (the table in ally_tier_rules.h).
// Every cut level belongs to the tier it starts, and every boundary is checked on
// both sides.

#include "test_support.h"
#include "ally_tier_rules.h"

#include <cmath>

using namespace totalrecruit;

namespace
{
const AllyTiers kTiers = AllyTiers();

bool Near(float a, float b)
{
    return std::fabs(a - b) < 0.0001f;
}

// Leader level = NPC level unless a test is about the gap.
AllyTierDecision Decide(int level, PersonalityGroup group, float relation = 50.0f)
{
    return DecideAllyTier(level, level, group, relation, kTiers);
}

void CutLevelsBelongToTheTierTheyStart()
{
    const char* name = "tier: 20, 50, 80 and 100 each start their tier; 19, 49, 79, 99 end the one below";
    CHECK(AllyTierOf(0, kTiers) == AllyTier_Low);
    CHECK(AllyTierOf(1, kTiers) == AllyTier_Low);
    CHECK(AllyTierOf(19, kTiers) == AllyTier_Low);
    CHECK(AllyTierOf(20, kTiers) == AllyTier_Mid);
    CHECK(AllyTierOf(49, kTiers) == AllyTier_Mid);
    CHECK(AllyTierOf(50, kTiers) == AllyTier_High);
    CHECK(AllyTierOf(79, kTiers) == AllyTier_High);
    CHECK(AllyTierOf(80, kTiers) == AllyTier_Elite);
    CHECK(AllyTierOf(99, kTiers) == AllyTier_Elite);
    CHECK(AllyTierOf(100, kTiers) == AllyTier_Legend);
    CHECK(AllyTierOf(250, kTiers) == AllyTier_Legend);
}

void TheCutsComeFromTheConfig()
{
    const char* name = "tier: the cut levels are config";
    AllyTiers t;
    t.midFromLevel = 30;
    t.highFromLevel = 60;
    t.eliteFromLevel = 90;
    t.legendFromLevel = 110;
    CHECK(AllyTierOf(29, t) == AllyTier_Low);
    CHECK(AllyTierOf(30, t) == AllyTier_Mid);
    CHECK(AllyTierOf(60, t) == AllyTier_High);
    CHECK(AllyTierOf(89, t) == AllyTier_High);
    CHECK(AllyTierOf(90, t) == AllyTier_Elite);
    CHECK(AllyTierOf(110, t) == AllyTier_Legend);
    // A Good level 25 ally is in the low tier now, so free.
    CHECK(DecideAllyTier(25, 25, PersonalityGroup_Good, 50.0f, t).mode == AllyTierMode_Free);
}

void EachTierHasItsCap()
{
    const char* name = "discount: 20% below 50, 10% from 50, 10% for the greedy at 80-99, none past that";
    CHECK(Near(AllyTierMaxDiscount(AllyTier_Low, kTiers), 0.20f));
    CHECK(Near(AllyTierMaxDiscount(AllyTier_Mid, kTiers), 0.20f));
    CHECK(Near(AllyTierMaxDiscount(AllyTier_High, kTiers), 0.10f));
    CHECK(Near(AllyTierMaxDiscount(AllyTier_Elite, kTiers), 0.10f));
    CHECK(Near(AllyTierMaxDiscount(AllyTier_Legend, kTiers), 0.0f));
}

void TheDiscountIsTheRelationUpToTheCap()
{
    const char* name = "discount: min(tierMax, max(0, relation) / 100 * scale)";
    CHECK(Near(FriendshipDiscount(AllyTier_Mid, 14.0f, kTiers), 0.14f));
    CHECK(Near(FriendshipDiscount(AllyTier_Mid, 20.0f, kTiers), 0.20f));
    CHECK(Near(FriendshipDiscount(AllyTier_Mid, 90.0f, kTiers), 0.20f));
    CHECK(Near(FriendshipDiscount(AllyTier_High, 90.0f, kTiers), 0.10f));
    CHECK(Near(FriendshipDiscount(AllyTier_High, 6.0f, kTiers), 0.06f));
    CHECK(Near(FriendshipDiscount(AllyTier_Low, -40.0f, kTiers), 0.0f));
    AllyTiers half;
    half.discountScale = 0.5f;
    CHECK(Near(FriendshipDiscount(AllyTier_Mid, 28.0f, half), 0.14f));
    CHECK(Near(FriendshipDiscount(AllyTier_Mid, 100.0f, half), 0.20f));
    AllyTiers none;
    none.discountScale = 0.0f;
    CHECK(Near(FriendshipDiscount(AllyTier_Mid, 100.0f, none), 0.0f));
}

void BelowTwentyTheGoodJoinFreeAndTheRestPay()
{
    const char* name = "1-19: Good joins free (the roll decides), everyone else pays up to 20%";
    AllyTierDecision d = Decide(10, PersonalityGroup_Good);
    CHECK(d.mode == AllyTierMode_Free);
    CHECK(d.reason == AllyTierReason_GoodFriend);
    CHECK(d.tier == AllyTier_Low);
    CHECK(Near(d.discountFraction, 0.0f));
    CHECK(Decide(19, PersonalityGroup_Good).mode == AllyTierMode_Free);

    const PersonalityGroup others[3] = { PersonalityGroup_Neutral, PersonalityGroup_Greedy,
                                         PersonalityGroup_Bad };
    for (int i = 0; i < 3; ++i)
    {
        d = Decide(19, others[i], 50.0f);
        CHECK(d.mode == AllyTierMode_Paid);
        CHECK(d.reason == AllyTierReason_FriendlyPrice);
        CHECK(Near(d.discountFraction, 0.20f));
    }
    CHECK(Near(Decide(5, PersonalityGroup_Neutral, 14.0f).discountFraction, 0.14f));
}

void FromTwentyToFortyNineEveryonePaysUpToTwenty()
{
    const char* name = "20-49: everyone pays, up to 20% (a Good level 20 already pays)";
    for (int g = PersonalityGroup_Good; g <= PersonalityGroup_Bad; ++g)
    {
        AllyTierDecision d = Decide(20, (PersonalityGroup)g, 50.0f);
        CHECK(d.mode == AllyTierMode_Paid);
        CHECK(d.tier == AllyTier_Mid);
        CHECK(Near(d.discountFraction, 0.20f));
        d = Decide(49, (PersonalityGroup)g, 17.0f);
        CHECK(d.mode == AllyTierMode_Paid);
        CHECK(Near(d.discountFraction, 0.17f));
    }
}

void FromFiftyToSeventyNineEveryonePaysUpToTen()
{
    const char* name = "50-79: everyone pays, up to 10%";
    for (int g = PersonalityGroup_Good; g <= PersonalityGroup_Bad; ++g)
    {
        AllyTierDecision d = Decide(50, (PersonalityGroup)g, 50.0f);
        CHECK(d.mode == AllyTierMode_Paid);
        CHECK(d.tier == AllyTier_High);
        CHECK(Near(d.discountFraction, 0.10f));
        CHECK(Decide(79, (PersonalityGroup)g).mode == AllyTierMode_Paid);
    }
}

void FromEightyTheGreedyPayTheGoodComeForAWhileTheBadRefuse()
{
    const char* name = "80-99: Greedy pays up to 10%, Good/Neutral are temporary, Bad refuses";
    AllyTierDecision d = Decide(80, PersonalityGroup_Greedy, 50.0f);
    CHECK(d.mode == AllyTierMode_Paid);
    CHECK(d.tier == AllyTier_Elite);
    CHECK(Near(d.discountFraction, 0.10f));
    CHECK(Decide(99, PersonalityGroup_Greedy).mode == AllyTierMode_Paid);

    d = Decide(80, PersonalityGroup_Good);
    CHECK(d.mode == AllyTierMode_Temporary);
    CHECK(d.reason == AllyTierReason_Temporary);
    CHECK(Near(d.discountFraction, 0.0f));
    CHECK(Decide(99, PersonalityGroup_Neutral).mode == AllyTierMode_Temporary);

    d = Decide(80, PersonalityGroup_Bad);
    CHECK(d.mode == AllyTierMode_Refuse);
    CHECK(d.reason == AllyTierReason_Cruel);
    CHECK(Decide(99, PersonalityGroup_Bad).reason == AllyTierReason_Cruel);
}

void FromAHundredOnlyTheGoodAndNeutralComeAndOnlyForAWhile()
{
    const char* name = "100+: Good/Neutral are temporary, Greedy and Bad refuse";
    CHECK(Decide(100, PersonalityGroup_Good).mode == AllyTierMode_Temporary);
    CHECK(Decide(150, PersonalityGroup_Neutral).mode == AllyTierMode_Temporary);
    AllyTierDecision d = Decide(100, PersonalityGroup_Greedy);
    CHECK(d.mode == AllyTierMode_Refuse);
    CHECK(d.reason == AllyTierReason_Greedy);
    CHECK(d.tier == AllyTier_Legend);
    CHECK(Decide(100, PersonalityGroup_Bad).reason == AllyTierReason_Cruel);
}

void ATemporaryMemberOnlyFollowsSomeoneCloseInLevel()
{
    const char* name = "temporary: npc - leader <= closeLevelGap (15), inclusive; further is too weak";
    AllyTierDecision d = DecideAllyTier(80, 65, PersonalityGroup_Good, 50.0f, kTiers);
    CHECK(d.mode == AllyTierMode_Temporary);
    d = DecideAllyTier(80, 64, PersonalityGroup_Good, 50.0f, kTiers);
    CHECK(d.mode == AllyTierMode_Refuse);
    CHECK(d.reason == AllyTierReason_LeaderTooWeak);
    CHECK(DecideAllyTier(100, 85, PersonalityGroup_Neutral, 50.0f, kTiers).mode == AllyTierMode_Temporary);
    CHECK(DecideAllyTier(100, 84, PersonalityGroup_Neutral, 50.0f, kTiers).reason == AllyTierReason_LeaderTooWeak);
    // A leader above them is no objection.
    CHECK(DecideAllyTier(85, 120, PersonalityGroup_Good, 50.0f, kTiers).mode == AllyTierMode_Temporary);
    // The gap is config, and does not touch the paying greedy.
    AllyTiers wide;
    wide.closeLevelGap = 30;
    CHECK(DecideAllyTier(80, 50, PersonalityGroup_Good, 50.0f, wide).mode == AllyTierMode_Temporary);
    CHECK(DecideAllyTier(80, 1, PersonalityGroup_Greedy, 50.0f, kTiers).mode == AllyTierMode_Paid);
}

void OnlyAPaidDealMovesThePrice()
{
    const char* name = "price: 1 - discount for Paid, 1.0 for every other deal";
    AllyTierDecision paid = Decide(34, PersonalityGroup_Neutral, 14.0f);
    CHECK(Near(AllyPriceMultiplier(paid), 0.86f));
    CHECK(Near(AllyPriceMultiplier(Decide(10, PersonalityGroup_Good)), 1.0f));
    CHECK(Near(AllyPriceMultiplier(Decide(85, PersonalityGroup_Good)), 1.0f));
    CHECK(Near(AllyPriceMultiplier(Decide(85, PersonalityGroup_Bad)), 1.0f));
}

void TheDiscountReadsAsAWholePercent()
{
    const char* name = "percent: rounded to the nearest whole percent";
    CHECK(DiscountPercent(0.14f) == 14);
    CHECK(DiscountPercent(0.136f) == 14);
    CHECK(DiscountPercent(0.20f) == 20);
    CHECK(DiscountPercent(0.0f) == 0);
    CHECK(DiscountPercent(-0.3f) == 0);
}

void TheDaysStayInsideTheConfiguredRange()
{
    const char* name = "days: every seed lands in [min, max] and every day in it is reachable";
    bool seen[11] = { false };
    for (unsigned int seed = 0; seed < 1000; ++seed)
    {
        const int days = TemporaryDaysFromSeed(seed, kTiers);
        CHECK(days >= 3 && days <= 10);
        if (days >= 0 && days <= 10) seen[days] = true;
    }
    for (int d = 3; d <= 10; ++d)
    {
        CHECK(seen[d]);
    }
    CHECK(TemporaryDaysFromSeed(0xFFFFFFFFu, kTiers) >= 3);
    AllyTiers inverted;
    inverted.tempMemberMinDays = 5;
    inverted.tempMemberMaxDays = 2;
    CHECK(TemporaryDaysFromSeed(12345u, inverted) == 5);
    AllyTiers fixed;
    fixed.tempMemberMinDays = 7;
    fixed.tempMemberMaxDays = 7;
    CHECK(TemporaryDaysFromSeed(999u, fixed) == 7);
}
} // namespace

void RunAllyTierRulesTests()
{
    CutLevelsBelongToTheTierTheyStart();
    TheCutsComeFromTheConfig();
    EachTierHasItsCap();
    TheDiscountIsTheRelationUpToTheCap();
    BelowTwentyTheGoodJoinFreeAndTheRestPay();
    FromTwentyToFortyNineEveryonePaysUpToTwenty();
    FromFiftyToSeventyNineEveryonePaysUpToTen();
    FromEightyTheGreedyPayTheGoodComeForAWhileTheBadRefuse();
    FromAHundredOnlyTheGoodAndNeutralComeAndOnlyForAWhile();
    ATemporaryMemberOnlyFollowsSomeoneCloseInLevel();
    OnlyAPaidDealMovesThePrice();
    TheDiscountReadsAsAWholePercent();
    TheDaysStayInsideTheConfiguredRange();
}
