#pragma once

#include <string>

class Character;

namespace totalrecruit
{
// Feeding a prisoner from your own pack.
//
// Without this the "care" price is a dead end: the NPC says they are starving
// and the player has no way to answer except to wait for hunger to do the work
// on its own, which is precisely the tedium this mod set out to avoid. Giving
// them food has to be an action the player can take.
//
// Kenshi has no "give item to prisoner" interaction, so the plugin performs it
// directly: take a food item from the player's squad, hand it to the captive,
// and let the engine apply it.

// True when anyone in the player's squad is carrying something edible.
bool PlayerHasFood();

// Feeds the captive one food item from the nearest squad member who has some.
// Returns false with a reason when there is nothing to give or the engine
// refuses.
bool FeedCaptive(Character* captive, std::string* outMessage);
} // namespace totalrecruit
