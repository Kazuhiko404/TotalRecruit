#pragma once

class Character;

namespace totalrecruit
{
// What the engine's own per-character memory is made to say about a rescue. It is
// saved with the game, which is why the mod uses it rather than a file.

// Records that the player saved this character's life, so the debt survives even if
// the player walks away before they wake.
void MarkRescuedBy(Character* character, Character* saviour);

// True when the character remembers the player as their lifesaver.
bool RemembersPlayerAsSaviour(Character* character);

// Hands over what a rescued NPC is carrying when they cannot follow the player, and
// returns how many cats changed hands (0 if they had nothing, or if they have
// already paid). The debt is settled once and the fact is stored in the engine's
// own per-character memory, so it survives a reload.
int PayPartingGift(Character* character);
} // namespace totalrecruit
