#include "price_rules.h"

#include <cmath>

namespace totalrecruit
{
namespace
{
const float kFactionMultiplierMin = 0.25f;
const float kFactionMultiplierMax = 3.0f;
// Quotes are rounded to this many cats so they read as prices.
const float kPriceStep = 10.0f;
// Even with the ceiling lifted a quote must stay a valid int, with room to spare.
const float kAbsoluteMaxCats = 1.0e9f;
} // namespace

// The raw curve, calibrated to vanilla-like prices: a level-13 named, talented
// NPC quotes ~2500 cats (never above 3000) once the capped premium applies, and
// the strongest ordinary characters approach ~27k. Shipped values give ~380 at
// 5, ~1.05k at 10, ~1.6k at 13, ~3.4k at 20, ~11.5k at 40, ~20.3k at 60 and
// ~27.2k at 75.
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

    return ((float)prices.catsPerLevel * 0.1f) * lv * lv
         + (float)prices.catsCeiling * (sigNow - sigZero);
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

// A craftsman is worth more than a swordsman of the same level: Kenshi hands you
// fighters constantly and engineers almost never. Combat is the baseline.
float TalentFamilyWeight(TalentFamily family, const Prices& prices)
{
    if (family == TalentFamily_Craft)  return prices.craftFamilyWeight;
    if (family == TalentFamily_Shadow) return prices.shadowFamilyWeight;
    return 1.0f;
}

float TalentMultiplier(float edge, float familyWeight, const Prices& prices)
{
    if (prices.talentWeight <= 0.0f || edge <= 1.0f)
    {
        return 1.0f;
    }

    float multiplier = 1.0f + (edge - 1.0f) * familyWeight * prices.talentWeight;
    const float most = (prices.talentMaxMultiplier > 1.0f) ? prices.talentMaxMultiplier : 1.0f;
    if (multiplier > most)
    {
        multiplier = most;
    }
    return multiplier;
}

float FactionPriceMultiplier(float relation, const Prices& prices)
{
    float multiplier = 1.0f - (relation / 100.0f) * prices.factionWeight;
    if (multiplier < kFactionMultiplierMin)
    {
        multiplier = kFactionMultiplierMin;
    }
    if (multiplier > kFactionMultiplierMax)
    {
        multiplier = kFactionMultiplierMax;
    }
    return multiplier;
}

float PremiumMultiplier(const PriceInputs& inputs, const Prices& prices)
{
    float premium = inputs.talentMultiplier;
    if (inputs.unique)
    {
        premium *= prices.uniqueMultiplier;
    }
    premium *= PersonalityPriceMultiplier(inputs.personality, prices);

    const float most = (prices.premiumMaxMultiplier > 1.0f) ? prices.premiumMaxMultiplier : 1.0f;
    if (premium > most)
    {
        premium = most;
    }
    return premium;
}

int FinalPriceInCats(const PriceInputs& inputs, const Prices& prices)
{
    float cats = BasePriceInCats(inputs.level, prices);
    cats *= PremiumMultiplier(inputs, prices);
    cats *= inputs.factionMultiplier;

    // Rounded to a price, then held between the floor and the ceiling. The
    // comparison is written so a NaN falls through to the ceiling.
    cats = floorf(cats / kPriceStep + 0.5f) * kPriceStep;

    float most = (prices.catsHardCap > 0) ? (float)prices.catsHardCap : kAbsoluteMaxCats;
    const float least = (float)prices.catsMinimum;
    if (most < least)
    {
        most = least;
    }
    if (!(cats < most))
    {
        cats = most;
    }
    if (cats < least)
    {
        cats = least;
    }
    return (int)cats;
}
} // namespace totalrecruit
