#pragma once

// Who the readout is about right now, and where on the screen they are.
//
// Three sources, in this order: the character an open right-click menu is about,
// the object the player has selected, and (when allowed) the NPC under the cursor.
// There is no engine call that says what is under the cursor -- the world cursor hook
// never fires (KENSHI-MODDING-NOTES section 11) -- so the hover is found the way any
// overlay would: the characters near the camera centre are projected to the screen
// and the one closest to the mouse, within a small radius, wins. Read-only; every
// pointer returned is raw and only valid for the tick that asked.

class Character;

namespace totalrecruit
{
enum ReadoutSource
{
    ReadoutSource_Menu,
    ReadoutSource_Selection,
    ReadoutSource_Hover
};

struct ReadoutTarget
{
    Character* character;
    ReadoutSource source;
    // Where the cursor was when the target was found (the fallback anchor).
    int mouseX;
    int mouseY;

    ReadoutTarget() : character(0), source(ReadoutSource_Hover), mouseX(0), mouseY(0) {}
};

// False when nothing qualifies. `hoverAllowed` is the "show on hover" setting.
bool FindReadoutTarget(bool hoverAllowed, ReadoutTarget* out);

// The screen point the card hangs above for this character (just over their head).
// False when they are behind the camera or off the screen.
bool ReadoutAnchorFor(Character* character, int* x, int* y);

const char* ReadoutSourceName(ReadoutSource source);
} // namespace totalrecruit
