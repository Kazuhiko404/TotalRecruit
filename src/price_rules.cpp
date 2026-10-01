#include "price_rules.h"

#include <cmath>

namespace totalrecruit
{
// Calibrated against the published Recruit-Prisoners-for-Cats tiers:
// ~1k at 5 stats, ~7k at 12, ~20k at 25, ~35k at 40, ~55k at 50, ~80k at 75.
float BasePriceInCats(int level, const Prices& prices)
{
    if (level < 1)
    {
        level = 1;
    }

    const float lv = (float)level;
    const float shape = prices.priceCurveSteepness;
    const float mid   = prices.priceCurveMidpoint;
    const float sigNow  = 1.0f / (1.0f + expf(-shape * (lv - mid)));
    const float sigZero = 1.0f / (1.0f + expf(shape * mid));

    float cats = ((float)prices.catsPerLevel * 0.1f) * lv * lv
               + (float)prices.catsCeiling * (sigNow - sigZero);
    if (cats < (float)prices.catsMinimum)
    {
        cats = (float)prices.catsMinimum;
    }
    return cats;
}

float PersonalityPriceMultiplier(Temperament personality, const Prices& prices)
{
    switch (personality)
    {
    case Temperament_Fearful:
        return prices.fearfulMultiplier;
    case Temperament_Brave:
    case Temperament_ColdCruel:
    case Temperament_Maniacal:
        return prices.braveMultiplier;
    default:
        return 1.0f;
    }
}
} // namespace totalrecruit
