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
//
// The anchor is the point just over the head: the character's game-space position
// lifted by a head height in world units, taken to render space by the scene's
// relative origin and projected through the player camera (screen_projection.h has
// the math and the evidence for the origin step).

#include "card_dismiss_rules.h"
#include "screen_projection.h"

class Character;

namespace totalrecruit
{
// The character the player has selected, when it is not one of their own. NULL when
// nothing is selected, the selection is not a character, or it is the player's.
Character* SelectedNpc();

enum AnchorResult
{
    Anchor_Ok,
    // The camera, its scene or the view size could not be read.
    Anchor_NoCamera,
    // The character's position could not be read.
    Anchor_NoPosition,
    Anchor_Behind,
    Anchor_OffScreen
};

// Everything the anchor was worked out from, so a log line can state it.
struct AnchorReading
{
    AnchorResult result;
    // The screen point the card hangs above (valid for Anchor_Ok; the raw projected
    // pixel for Anchor_OffScreen).
    int x;
    int y;
    int viewW;
    int viewH;
    // The game-space point projected (over the head) and the scene's relative origin.
    Vec3 head;
    Vec3 origin;

    AnchorReading() : result(Anchor_NoCamera), x(0), y(0), viewW(0), viewH(0) {}
};

// The screen point the card hangs above for this character (just over their head).
AnchorReading ReadoutAnchorFor(Character* character);

// Where this character stands on the screen: their feet and the point over their
// head (the anchor's height, without its pixel lift), projected the same way. Off
// the screen still reads (the pixels are kept); false when behind the camera or
// unreadable. Only asked on a mouse press, never per frame.
bool ReadoutBodySpanFor(Character* character, BodySpan* out);

const char* AnchorResultWord(AnchorResult result);
} // namespace totalrecruit
