#pragma once

// The price curve, without the engine. Talent, faction and uniqueness multiply
// on top of this in rules.cpp because they read the character.

#include "config.h"
#include "temperament.h"

namespace totalrecruit
{
// Cats for a character of this rough level, before any multiplier.
//
// Two curves added together, because no single one covers the range: a gentle
// square term carries the early levels (a level 3 bandit stays a few hundred
// cats) and a logistic S-curve, shifted to pass through zero, carries the rest.
// Never below prices.catsMinimum.
float BasePriceInCats(int level, const Prices& prices);

// The personality multiplier on the cats price: cowards sell cheap, the brave
// and the cruel dear.
float PersonalityPriceMultiplier(Temperament personality, const Prices& prices);
} // namespace totalrecruit
