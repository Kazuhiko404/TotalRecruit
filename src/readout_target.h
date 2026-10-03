#pragma once

// Who the readout card is about when nobody pressed Hablar, and where on the screen
// a character is.
//
// The card follows the player's selection (readout.h): the NPC they left-clicked,
// never one of their own. Reading it is a single field of the player interface, cheap
// enough to compare every frame; the expensive evaluation only runs when it changes.
// The hover path (the NPC under the cursor, found by projecting every character near
// the camera) is gone: it was the slow, screen-covering behaviour the player turned
// down. Read-only; every pointer returned is raw and only valid for the tick that
// asked.

class Character;

namespace totalrecruit
{
// The character the player has selected, when it is not one of their own. NULL when
// nothing is selected, the selection is not a character, or it is the player's.
Character* SelectedNpc();

// The screen point the card hangs above for this character (just over their head).
// False when they are behind the camera or off the screen.
bool ReadoutAnchorFor(Character* character, int* x, int* y);
} // namespace totalrecruit
