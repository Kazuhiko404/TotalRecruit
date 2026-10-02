// Tests for src/price_rules.cpp: the base curve, the multipliers and the final
// quote. T5 re-derived the curve against the published Recruit-Prisoners-for-Cats
// tiers (weak NPCs 1-6k, modest fighters 6-25k, 40-50 stats 30-50k, 55-65 stats
// 50-60k, 70-80+ stats ~80k, strongest uniques 100-150k); the bands below are
// those tiers and the pinned numbers are the shipped curve.

#include "test_support.h"
#include "price_rules.h"

#include <climits>

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
    // Level 1 is ~150 cats on the curve, below catsMinimum: the floor belongs to
    // FinalPriceInCats, after the multipliers.
    CHECK(BasePriceInCats(1, kPrices) < 200.0f);
    CHECK(Near(BasePriceInCats(-5, kPrices), BasePriceInCats(1, kPrices), 0.001f));   // clamped to level 1
}

void ShippedCurveAtFixedLevels()
{
    const char* name = "price: shipped curve at fixed levels";
    CHECK(Near(BasePriceInCats(5, kPrices), 1065.0f, 5.0f));
    CHECK(Near(BasePriceInCats(10, kPrices), 2955.0f, 5.0f));
    CHECK(Near(BasePriceInCats(20, kPrices), 9565.0f, 5.0f));
    CHECK(Near(BasePriceInCats(40, kPrices), 32159.0f, 10.0f));
    CHECK(Near(BasePriceInCats(60, kPrices), 56862.0f, 15.0f));
    CHECK(Near(BasePriceInCats(75, kPrices), 76481.0f, 20.0f));
}

void CurveMatchesThePublishedTiers()
{
    const char* name = "price: the curve sits inside the reference mod's tiers";
    CHECK(Between(BasePriceInCats(5, kPrices), 1000.0f, 6000.0f));     // weak
    CHECK(Between(BasePriceInCats(15, kPrices), 1000.0f, 6000.0f));    // weak, top of the band
    CHECK(Between(BasePriceInCats(25, kPrices), 6000.0f, 25000.0f));   // modest fighters
    CHECK(Between(BasePriceInCats(40, kPrices), 30000.0f, 50000.0f));  // 40-50 stats
    CHECK(Between(BasePriceInCats(50, kPrices), 30000.0f, 50000.0f));
    CHECK(Between(BasePriceInCats(60, kPrices), 50000.0f, 60000.0f));  // 55-65 stats
    CHECK(Between(BasePriceInCats(75, kPrices), 70000.0f, 85000.0f));  // 70-80+ stats, ~80k
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
    CHECK(Near((float)Quote(5), 1060.0f, 10.0f));
    CHECK(Near((float)Quote(10), 2950.0f, 10.0f));
    CHECK(Near((float)Quote(20), 9570.0f, 10.0f));
    CHECK(Near((float)Quote(40), 32160.0f, 10.0f));
    CHECK(Near((float)Quote(60), 56860.0f, 15.0f));
}

void UniqueMultipliesTheQuote()
{
    const char* name = "price: a named character costs uniqueMultiplier times more";
    PriceInputs in;
    in.level = 60;
    in.unique = true;
    const int unique = FinalPriceInCats(in, kPrices);
    CHECK(Near((float)unique, 113720.0f, 30.0f));
    CHECK(Between((float)unique, 100000.0f, 150000.0f));   // the reference mod's unique band
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
    CHECK(FinalPriceInCats(in, kPrices) == kPrices.catsHardCap);

    // The cap is configurable and 0 lifts it; the quote stays a valid int either way.
    Prices uncapped;
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

void FactionMultiplierIsBounded()
{
    const char* name = "price: faction multiplier";
    CHECK(FactionPriceMultiplier(0.0f, kPrices) == 1.0f);
    CHECK(Near(FactionPriceMultiplier(-100.0f, kPrices), 1.75f, 0.001f));   // hated: +75%
    CHECK(Near(FactionPriceMultiplier(50.0f, kPrices), 0.625f, 0.001f));
    CHECK(FactionPriceMultiplier(100.0f, kPrices) == 0.25f);               // clamped, not 0.0
    CHECK(FactionPriceMultiplier(5000.0f, kPrices) == 0.25f);
    Prices fierce;
    fierce.factionWeight = 2.0f;
    CHECK(FactionPriceMultiplier(-100.0f, fierce) == 3.0f);                // clamped at 3x
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
    // 32158.79 * 1.25 * 2.0 * 1.6 * 0.8 = 102908
    CHECK(Near((float)FinalPriceInCats(in, kPrices), 102910.0f, 20.0f));
}
} // namespace

void RunPriceRulesTests()
{
    BaseCurveIsTheRawCurve();
    ShippedCurveAtFixedLevels();
    CurveMatchesThePublishedTiers();
    CurveNeverDecreases();
    CeilingIsConfigurable();
    PersonalityMultipliers();
    QuotesAtFixedLevels();
    UniqueMultipliesTheQuote();
    FloorComesAfterTheMultipliers();
    CeilingComesAfterTheMultipliers();
    QuotesAreRoundedToTens();
    TalentMultiplierIsBounded();
    FactionMultiplierIsBounded();
    MultipliersStackInOrderWithOneRounding();
}
