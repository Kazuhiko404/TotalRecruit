#pragma once

// The price curve, without the engine. Talent, ally friendship and uniqueness are read
// from the character elsewhere and arrive here as PriceInputs.

#include "config.h"
#include "temperament.h"

namespace totalrecruit
{
// Cats for a character of this rough level, before any multiplier.
//
// Two curves added together, because no single one covers the range: a gentle
// square term carries the early levels (a level 3 bandit stays a few hundred
// cats) and a logistic S-curve, shifted to pass through zero, carries the rest.
// The raw curve: no floor, no ceiling and no multiplier. FinalPriceInCats applies
// those, in that order, so the minimum is the last word rather than something a
// later multiplier can undercut.
float BasePriceInCats(int level, const Prices& prices);

// The personality multiplier on the cats price: cowards sell cheap, the brave
// and the cruel dear.
float PersonalityPriceMultiplier(Temperament personality, const Prices& prices);

// How much an unusual talent adds. `edge` is the character's best stat over their
// own rough level (1.0 = nothing unusual) and `familyWeight` how rare that kind of
// skill is. Never below 1.0, never above prices.talentMaxMultiplier.
float TalentMultiplier(float edge, float familyWeight, const Prices& prices);

// The families of skill a talent can belong to (rules.cpp reads the stats and
// says which one the character's best stat falls in).
enum TalentFamily
{
    TalentFamily_Combat,
    TalentFamily_Shadow,
    TalentFamily_Craft
};

// How rare that kind of skill is: the weight TalentMultiplier scales the edge by.
// Combat is the baseline; the other two come from prices.
float TalentFamilyWeight(TalentFamily family, const Prices& prices);

// Everything that decides one quote, already read from the character.
struct PriceInputs
{
    int level;
    float talentMultiplier;    // TalentMultiplier(), 1.0 when none applies
    bool unique;
    Temperament personality;
    float factionMultiplier;   // a Balanced ally's friendship discount (AllyPriceMultiplier), else 1.0

    PriceInputs()
        : level(1), talentMultiplier(1.0f), unique(false),
          personality(Temperament_Normal), factionMultiplier(1.0f)
    {
    }
};

// talent x unique x personality, together, capped from above at
// prices.premiumMaxMultiplier (a value below 1.0 in the config reads as 1.0).
// Only the top is capped: a coward's 0.6 still discounts. The ally
// friendship multiplier is not part of it; FinalPriceInCats applies it after.
float PremiumMultiplier(const PriceInputs& inputs, const Prices& prices);

// The price the player is asked for, in whole cats.
//
// base curve x PremiumMultiplier x factionMultiplier (ally friendship), rounded to the nearest
// 10 cats (so a quote reads as a price, not as a float's tail), then held between
// prices.catsMinimum and prices.catsHardCap. The floor and the ceiling come last
// on purpose: applied before the multipliers, a coward's 0.6 undercut the minimum
// and a stack of them could overflow an int.
int FinalPriceInCats(const PriceInputs& inputs, const Prices& prices);
} // namespace totalrecruit
