// Characterization tests for src/price_rules.cpp: the base curve as shipped.
// T5 re-derives the curve against the reference mods; these pin today's numbers
// so that change is visible.

#include "test_support.h"
#include "price_rules.h"

using namespace totalrecruit;

namespace
{
const Prices kPrices = Prices();

bool Near(float actual, float expected, float tolerance)
{
    const float d = actual - expected;
    return d < tolerance && d > -tolerance;
}

void LowLevelsHitTheMinimum()
{
    const char* name = "price: the bottom of the curve is catsMinimum";
    CHECK(BasePriceInCats(1, kPrices) == 200.0f);
    CHECK(BasePriceInCats(-5, kPrices) == 200.0f);   // clamped to level 1
}

void ShippedCurveAtFixedLevels()
{
    const char* name = "price: shipped curve at fixed levels";
    CHECK(Near(BasePriceInCats(5, kPrices), 467.0f, 2.0f));
    CHECK(Near(BasePriceInCats(25, kPrices), 9176.0f, 5.0f));
    CHECK(Near(BasePriceInCats(50, kPrices), 55151.0f, 20.0f));
    CHECK(Near(BasePriceInCats(75, kPrices), 91433.0f, 30.0f));
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
    dear.catsCeiling = 144000;
    CHECK(BasePriceInCats(75, dear) > BasePriceInCats(75, kPrices) * 1.5f);
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
} // namespace

void RunPriceRulesTests()
{
    LowLevelsHitTheMinimum();
    ShippedCurveAtFixedLevels();
    CurveNeverDecreases();
    CeilingIsConfigurable();
    PersonalityMultipliers();
}
