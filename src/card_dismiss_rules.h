#pragma once

// When a click elsewhere puts the readout card away, without the engine.
//
// Kenshi never clears the selection on a right-click to walk (nor always on a
// left-click on the ground), so a card that only followed the selection stayed up
// for good. These rules dismiss it on a fresh mouse press that is not on a MyGUI
// widget and not on the card's NPC, and bring it back on a left press on that NPC,
// on a change of selection, or when Hablar pins it again. A right press on the NPC
// is the context menu: it changes nothing (the card already hides while the menu
// is open). A card pinned by Hablar beside an open offer window ignores outside
// presses; it ends with the offer as before.
//
// readout.cpp owns the state and the engine reads (mouse_input.h,
// readout_target.h); everything here is plain values.

namespace totalrecruit
{
enum MousePress
{
    Press_None,
    Press_Left,
    Press_Right
};

// The buttons as last read, for edge detection.
struct PressEdgeState
{
    // False until the first read after a reset: that read only records the buttons,
    // so a button already held when the card came up is not taken as a press.
    bool primed;
    bool leftWasDown;
    bool rightWasDown;

    PressEdgeState() : primed(false), leftWasDown(false), rightWasDown(false) {}
};

// The press that started this frame, if any (left wins when both start together).
MousePress NextPressEdge(PressEdgeState* state, bool leftDown, bool rightDown);

// Forget the buttons: the next read primes again.
void ResetPressEdges(PressEdgeState* state);

// Where a character stands on the screen: their projected feet and the projected
// point just over their head (pixels, origin top-left, y down).
struct BodySpan
{
    int footX;
    int footY;
    int headX;
    int headY;

    BodySpan() : footX(0), footY(0), headX(0), headY(0) {}
    BodySpan(int fx, int fy, int hx, int hy) : footX(fx), footY(fy), headX(hx), headY(hy) {}
};

// Half the width of the clickable body box, as a share of its on-screen height. A
// standing character is about 20 world units tall and 7.5 across the shoulders
// (TotalSparring challenge/world_scale.h), so a true half-width is ~0.19 of the
// height; 0.3 leaves room for arms, weapons and an imprecise click.
const float kBodyHalfWidthPerHeight = 0.3f;
// The box is never narrower than this on either side, so a far-zoomed character
// stays clickable.
const int kBodyMinHalfWidthPx = 10;

// True when the screen point is inside the body box: from the head point down to
// the feet, centred between their x, kBodyHalfWidthPerHeight of the height to
// either side (at least kBodyMinHalfWidthPx).
bool PointOnBody(int x, int y, const BodySpan& body);

struct CardDismissInput
{
    // A press that started this frame (NextPressEdge).
    MousePress press;
    // The cursor was over a MyGUI widget (the offer window, Mod Hub, a menu button).
    bool overMyGui;
    // The press was on the card's NPC (PointOnBody). Only read for a press.
    bool onNpc;
    // Hablar pinned the card and its offer window is open.
    bool pinnedWithOffer;
    // The selection is another character (or none) since the last frame.
    bool selectionChanged;
    // Hablar was pressed for this card this frame.
    bool pinRequested;

    CardDismissInput()
        : press(Press_None), overMyGui(false), onNpc(false), pinnedWithOffer(false),
          selectionChanged(false), pinRequested(false)
    {
    }
};

// Whether the card is dismissed after this frame, given whether it was before.
bool NextCardDismissed(bool dismissed, const CardDismissInput& input);
} // namespace totalrecruit
