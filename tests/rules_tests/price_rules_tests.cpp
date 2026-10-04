// Tests for src/price_rules.cpp: the base curve, the multipliers and the final
// quote. The curve is calibrated to vanilla-like prices: a level-13 named,
// talented NPC (the in-game Soman case) quotes ~2500 cats, never above 3000, and
// the strongest ordinary characters approach ~27k. Talent x unique x personality
// is capped at premiumMaxMultiplier; the pinned numbers are the shipped curve.

#include "test_support.h"
#include "price_rules.h"

#include <climits>
#include <cmath>

using namespace totalrecruit;

namespace
{
const Prices kPrices = Prices();

bool Near(float actual, float expected, float tolerance)
{
    const float d = actual - expected;
    return d < tolerance && d > -tolerance;
}

bool Between(float value, float low, float high)
{
    return value >= low && value <= high;
}

// A quote for an ordinary character of this level.
int Quote(int level, const Prices& prices = kPrices)
{
    PriceInputs in;
    in.level = level;
    return FinalPriceInCats(in, prices);
}

void BaseCurveIsTheRawCurve()
{
    const char* name = "price: the base curve has no floor of its own";
    // Level 1 is ~55 cats on the curve, below catsMinimum: the floor belongs to
    // FinalPriceInCats, after the multipliers.
    CHECK(BasePriceInCats(1, kPrices) < 200.0f);
    CHECK(Near(BasePriceInCats(-5, kPrices), BasePriceInCats(1, kPrices), 0.001f));   // clamped to level 1
}

void ShippedCurveAtFixedLevels()
{
    const char* name = "price: shipped curve at fixed levels";
    CHECK(Near(BasePriceInCats(5, kPrices), 380.0f, 5.0f));
    CHECK(Near(BasePriceInCats(10, kPrices), 1054.0f, 5.0f));
    CHECK(Near(BasePriceInCats(13, kPrices), 1614.0f, 5.0f));
    CHECK(Near(BasePriceInCats(20, kPrices), 3410.0f, 5.0f));
    CHECK(Near(BasePriceInCats(40, kPrices), 11462.0f, 10.0f));
    CHECK(Near(BasePriceInCats(60, kPrices), 20256.0f, 15.0f));
    CHECK(Near(BasePriceInCats(75, kPrices), 27234.0f, 20.0f));
}

void CurveIsVanillaLike()
{
    const char* name = "price: the curve stays at vanilla-like prices";
    CHECK(Between(BasePriceInCats(5, kPrices), 200.0f, 600.0f));       // a weak bandit: a few hundred
    CHECK(Between(BasePriceInCats(13, kPrices), 1200.0f, 2000.0f));    // all stats ~13
    CHECK(Between(BasePriceInCats(40, kPrices), 9000.0f, 14000.0f));   // a serious fighter
    CHECK(Between(BasePriceInCats(75, kPrices), 25000.0f, 30000.0f));  // strongest ordinary, ~27k
}

void CurveNeverDecreases()
{
    const char* name = "price: never cheaper at a higher level";
    float previous = 0.0f;
    for (int level = 1; level <= 120; ++level)
    {
        const float price = BasePriceInCats(level, kPrices);
        CHECK(price >= previous);
        previous = price;
    }
}

void CeilingIsConfigurable()
{
    const char* name = "price: catsCeiling scales the top of the curve";
    Prices dear;
    dear.catsCeiling = kPrices.catsCeiling * 2;
    CHECK(BasePriceInCats(75, dear) > BasePriceInCats(75, kPrices) * 1.3f);
}

void PersonalityMultipliers()
{
    const char* name = "price: cowards cheap, the brave and the cruel dear";
    CHECK(PersonalityPriceMultiplier(Temperament_Fearful, kPrices) == 0.6f);
    CHECK(PersonalityPriceMultiplier(Temperament_Brave, kPrices) == 1.6f);
    CHECK(PersonalityPriceMultiplier(Temperament_ColdCruel, kPrices) == 1.6f);
    CHECK(PersonalityPriceMultiplier(Temperament_Maniacal, kPrices) == 1.6f);
    CHECK(PersonalityPriceMultiplier(Temperament_Normal, kPrices) == 1.0f);
    CHECK(PersonalityPriceMultiplier(Temperament_Honorable, kPrices) == 1.0f);
}

void QuotesAtFixedLevels()
{
    const char* name = "price: the quote at fixed levels (ordinary character)";
    CHECK(Near((float)Quote(5), 380.0f, 10.0f));
    CHECK(Near((float)Quote(10), 1050.0f, 10.0f));
    CHECK(Near((float)Quote(20), 3410.0f, 10.0f));
    CHECK(Near((float)Quote(40), 11460.0f, 10.0f));
    CHECK(Near((float)Quote(60), 20260.0f, 15.0f));
}

void UniqueMultipliesTheQuote()
{
    const char* name = "price: a named character costs uniqueMultiplier times more, up to the premium cap";
    PriceInputs in;
    in.level = 60;
    in.unique = true;

    // With the premium cap lifted the name alone doubles the quote.
    Prices loose;
    loose.premiumMaxMultiplier = 10.0f;
    CHECK(Near((float)FinalPriceInCats(in, loose), 40510.0f, 20.0f));   // 20256 x 2.0

    // Shipped: 2.0 is above the 1.6 premium cap, so the cap decides.
    CHECK(Near((float)FinalPriceInCats(in, kPrices), 32410.0f, 20.0f)); // 20256 x 1.6
}

void SomanCaseQuotesAVanillaPrice()
{
    const char* name = "price: a level-13 named talented NPC quotes 2000-3000 (Soman case)";
    PriceInputs in;
    in.level = 13;
    in.unique = true;
    in.talentMultiplier = 1.5f;
    in.personality = Temperament_Normal;
    const int cats = FinalPriceInCats(in, kPrices);
    CHECK(Between((float)cats, 2000.0f, 3000.0f));
    CHECK(cats == 2580);   // 1614.2 x 1.6 (1.5 x 2.0 capped)
}

void PremiumIsCappedFromAbove()
{
    const char* name = "price: talent x unique x personality is capped at premiumMaxMultiplier";
    CHECK(Near(kPrices.premiumMaxMultiplier, 1.6f, 0.0001f));

    PriceInputs in;
    in.level = 40;
    in.unique = true;
    in.personality = Temperament_Brave;
    in.talentMultiplier = 1.5f;
    CHECK(Near(PremiumMultiplier(in, kPrices), 1.6f, 0.0001f));   // 4.8 asked
    const float base = BasePriceInCats(40, kPrices);
    const int expected = (int)(floorf(base * kPrices.premiumMaxMultiplier / 10.0f + 0.5f) * 10.0f);
    CHECK(FinalPriceInCats(in, kPrices) == expected);                  // 18340

    PriceInputs plain;
    CHECK(PremiumMultiplier(plain, kPrices) == 1.0f);                  // nothing to add

    Prices loose;
    loose.premiumMaxMultiplier = 50.0f;
    CHECK(Near(PremiumMultiplier(in, loose), 4.8f, 0.001f));          // the raw stack
}

void FearfulStillDiscounts()
{
    const char* name = "price: the premium cap never lifts a coward's discount";
    PriceInputs in;
    in.level = 40;
    in.personality = Temperament_Fearful;
    CHECK(Near(PremiumMultiplier(in, kPrices), 0.6f, 0.0001f));
    CHECK(FinalPriceInCats(in, kPrices) == 6880);                      // 11462.4 x 0.6
    CHECK(FinalPriceInCats(in, kPrices) < Quote(40));
}

void FloorComesAfterTheMultipliers()
{
    const char* name = "price: catsMinimum is the last word, a coward cannot undercut it";
    PriceInputs in;
    in.level = 1;
    in.personality = Temperament_Fearful;
    in.factionMultiplier = 0.25f;
    CHECK(FinalPriceInCats(in, kPrices) == 200);

    Prices noFloor;
    noFloor.catsMinimum = 0;
    // With no floor configured the curve and the multipliers speak for themselves.
    CHECK(FinalPriceInCats(in, noFloor) < 200);
}

void CeilingComesAfterTheMultipliers()
{
    const char* name = "price: catsHardCap stops stacked multipliers";
    PriceInputs in;
    in.level = 75;
    in.unique = true;
    in.personality = Temperament_Brave;
    in.talentMultiplier = 2.0f;
    in.factionMultiplier = 3.0f;
    // The premium cap alone already keeps this at ~131k; lift it so the stack
    // (27234 x 2 x 2 x 1.6 x 3 = ~523k) has to meet the hard cap.
    Prices stacked;
    stacked.premiumMaxMultiplier = 50.0f;
    CHECK(FinalPriceInCats(in, stacked) == kPrices.catsHardCap);

    // The cap is configurable and 0 lifts it; the quote stays a valid int either way.
    Prices uncapped = stacked;
    uncapped.catsHardCap = 0;
    CHECK(FinalPriceInCats(in, uncapped) > kPrices.catsHardCap);

    PriceInputs absurd;
    absurd.level = INT_MAX;
    absurd.unique = true;
    absurd.talentMultiplier = 1000.0f;
    absurd.factionMultiplier = 1000.0f;
    CHECK(FinalPriceInCats(absurd, uncapped) > 0);     // no wrap-around to a negative price
    CHECK(FinalPriceInCats(absurd, kPrices) == kPrices.catsHardCap);
}

void QuotesAreRoundedToTens()
{
    const char* name = "price: quotes round to the nearest 10 cats";
    Prices flat;
    flat.catsCeiling = 0;      // only the square term: 1 cat per level squared
    flat.catsPerLevel = 10;
    flat.catsMinimum = 0;
    CHECK(Quote(3, flat) == 10);     //   9 ->  10
    CHECK(Quote(5, flat) == 30);     //  25 ->  30 (halves round up)
    CHECK(Quote(12, flat) == 140);   // 144 -> 140
    CHECK(Quote(13, flat) == 170);   // 169 -> 170
}

void TalentMultiplierIsBounded()
{
    const char* name = "price: talent multiplier";
    CHECK(TalentMultiplier(1.0f, 1.0f, kPrices) == 1.0f);        // nothing unusual
    CHECK(TalentMultiplier(0.5f, 1.0f, kPrices) == 1.0f);        // never a discount
    CHECK(Near(TalentMultiplier(1.5f, 1.0f, kPrices), 1.5f, 0.001f));
    CHECK(Near(TalentMultiplier(1.25f, 1.6f, kPrices), 1.4f, 0.001f));  // craftsmen weigh more
    CHECK(Near(TalentMultiplier(1.5f, 1.6f, kPrices), 1.5f, 0.001f));   // 1.8 asked, default cap 1.5
    CHECK(Near(kPrices.talentMaxMultiplier, 1.5f, 0.0001f));            // talent already raises the chance
    CHECK(TalentMultiplier(20.0f, 1.6f, kPrices) == kPrices.talentMaxMultiplier);   // capped

    Prices off;
    off.talentWeight = 0.0f;
    CHECK(TalentMultiplier(5.0f, 1.0f, off) == 1.0f);            // 0 disables it

    Prices loose;
    loose.talentMaxMultiplier = 0.5f;                            // nonsense is read as "no bonus"
    CHECK(TalentMultiplier(5.0f, 1.0f, loose) == 1.0f);
}

void MultipliersStackInOrderWithOneRounding()
{
    const char* name = "price: multipliers combine once, then round";
    PriceInputs in;
    in.level = 40;
    in.unique = true;
    in.personality = Temperament_Brave;
    in.talentMultiplier = 1.25f;
    in.factionMultiplier = 0.8f;
    // 11462.4 * min(1.25 * 2.0 * 1.6, 1.6) * 0.8 = 14671.9: faction applies after the cap
    CHECK(Near((float)FinalPriceInCats(in, kPrices), 14670.0f, 10.0f));
    Prices loose;
    loose.premiumMaxMultiplier = 50.0f;
    // 11462.4 * 1.25 * 2.0 * 1.6 * 0.8 = 36679.7
    CHECK(Near((float)FinalPriceInCats(in, loose), 36680.0f, 10.0f));
}

// --- T8: the family weights moved out of rules.cpp into the config.

void FamilyWeightsKeepTheirShippedValues()
{
    const char* name = "talent family: combat 1.0, shadow 1.3, craft 1.6 by default";
    CHECK(TalentFamilyWeight(TalentFamily_Combat, kPrices) == 1.0f);
    CHECK(Near(TalentFamilyWeight(TalentFamily_Shadow, kPrices), 1.3f, 0.0001f));
    CHECK(Near(TalentFamilyWeight(TalentFamily_Craft, kPrices), 1.6f, 0.0001f));
}

void FamilyWeightsAreConfigurable()
{
    const char* name = "talent family: shadow and craft follow the config, combat stays 1.0";
    Prices prices;
    prices.craftFamilyWeight = 2.5f;
    prices.shadowFamilyWeight = 0.5f;
    CHECK(Near(TalentFamilyWeight(TalentFamily_Craft, prices), 2.5f, 0.0001f));
    CHECK(Near(TalentFamilyWeight(TalentFamily_Shadow, prices), 0.5f, 0.0001f));
    CHECK(TalentFamilyWeight(TalentFamily_Combat, prices) == 1.0f);
}

void FamilyWeightChangesTheQuote()
{
    const char* name = "talent family: a heavier craft weight makes the same craftsman dearer";
    Prices heavy;
    heavy.craftFamilyWeight = 3.0f;
    heavy.talentMaxMultiplier = 5.0f;
    Prices light = heavy;
    light.craftFamilyWeight = 1.0f;
    const float edge = 1.5f;
    CHECK(TalentMultiplier(edge, TalentFamilyWeight(TalentFamily_Craft, heavy), heavy) >
          TalentMultiplier(edge, TalentFamilyWeight(TalentFamily_Craft, light), light));
}
} // namespace

void RunPriceRulesTests()
{
    BaseCurveIsTheRawCurve();
    ShippedCurveAtFixedLevels();
    CurveIsVanillaLike();
    CurveNeverDecreases();
    CeilingIsConfigurable();
    PersonalityMultipliers();
    QuotesAtFixedLevels();
    UniqueMultipliesTheQuote();
    SomanCaseQuotesAVanillaPrice();
    PremiumIsCappedFromAbove();
    FearfulStillDiscounts();
    FloorComesAfterTheMultipliers();
    CeilingComesAfterTheMultipliers();
    QuotesAreRoundedToTens();
    TalentMultiplierIsBounded();
    MultipliersStackInOrderWithOneRounding();
    FamilyWeightsKeepTheirShippedValues();
    FamilyWeightsAreConfigurable();
    FamilyWeightChangesTheQuote();
}
