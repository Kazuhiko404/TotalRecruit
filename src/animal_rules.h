#pragma once

// How full an animal has to be before it follows the player, without the engine.
//
// A loose animal is tamed, not negotiated with: the whole cost is food, and how
// full the animal already is decides whether that cost has been paid. Maturity
// and level set the bar; the player's own level discounts it (in Kenshi terms the
// animal judging whether you can keep feeding it); temperament moves it a little.
// Every number is a 0..1 fraction of a full stomach.

#include "config.h"

namespace totalrecruit
{
// The bar for this animal, as a 0..1 fraction of its stomach. `maturity` is how
// grown it is, 0..1 of its own lifespan (the caller passes 1.0 when the engine
// cannot say). A cub or a weakling needs a mouthful; a grown beast wants far
// more; looking stronger than it discounts the bill. Never below cubFullness,
// never above a full stomach.
float TamingFullnessNeeded(float maturity, int npcLevel, int playerLevel,
                           const Animals& animals);

// R5.4 -- temperament as how much convincing the creature takes. `resolve` is
// ResolveMultiplier() of its temperament (1.0 for an ordinary one): around 1.0
// nothing moves, a timid animal settles for less, a stubborn one asks for more,
// and the bar stays capped at a full stomach so no temperament makes taming
// impossible.
float ApplyAnimalTemperament(float needed, float resolve, const Animals& animals);

// How much of the stomach is still missing, in whole percent, rounded to nearest.
int MissingStomachPercent(float needed, float fullness);
} // namespace totalrecruit
