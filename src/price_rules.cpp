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

// The raw curve, calibrated against the published Recruit-Prisoners-for-Cats
// tiers by base stats (level here): weak NPCs 1-6k, modest fighters 6-25k,
// 40-50 stats 30-50k, 55-65 stats 50-60k, 70-80+ stats ~80k. Shipped values give
// ~1.1k at 5, ~3k at 10, ~9.6k at 20, ~14k at 25, ~32k at 40, ~45k at 50,
// ~57k at 60 and ~76k at 75 (see the feature document for the fit).
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

int FinalPriceInCats(const PriceInputs& inputs, const Prices& prices)
{
    float cats = BasePriceInCats(inputs.level, prices);
    cats *= inputs.talentMultiplier;
    if (inputs.unique)
    {
        cats *= prices.uniqueMultiplier;
    }
    cats *= PersonalityPriceMultiplier(inputs.personality, prices);
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
