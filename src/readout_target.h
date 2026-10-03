#pragma once

// Where on the screen a character is, and (for the opt-in hover mode) who is under
// the cursor.
//
// By default the card is pinned by the Hablar click (readout.h) and this module only
// projects that character to the screen every frame. With readoutOnHover on, the NPC
// under the cursor is found the way any overlay would: there is no engine call that
// says what is under the cursor -- the world cursor hook never fires
// (KENSHI-MODDING-NOTES section 11) -- so the characters near the camera centre are
// projected to the screen and the one closest to the mouse, within a small radius,
// wins. That sweep is the costly part, which is why hover is opt-in. Read-only;
// every pointer returned is raw and only valid for the tick that asked.

class Character;

namespace totalrecruit
{
struct ReadoutTarget
{
    Character* character;
    // Where the cursor was when the target was found (the fallback anchor).
    int mouseX;
    int mouseY;

    ReadoutTarget() : character(0), mouseX(0), mouseY(0) {}
};

// The NPC under the cursor, never one of the player's own. False when there is
// none, or the cursor is over a window.
bool FindHoverTarget(ReadoutTarget* out);

// The screen point the card hangs above for this character (just over their head).
// False when they are behind the camera or off the screen.
bool ReadoutAnchorFor(Character* character, int* x, int* y);
} // namespace totalrecruit
