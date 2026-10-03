#pragma once

// The readout card: MyGUI widgets only. It draws a ReadoutView and its texts where it
// is told and decides nothing -- what to show is readout_model.cpp, the words are
// readout_text.cpp, who and when is readout.cpp.
//
// Built from the mockup (ui-mockup/readout.html): every constant under the "Layout"
// banner in readout_ui.cpp is the px value of the CSS variable of the same name.
// Fills are a shipped white PNG tinted with bright foreground colours, because a
// MyGUI tint multiplies against the skin underneath and a skin mod (DarkUI,
// PowerModUI) turns a dark-on-dark palette black; without the PNG the stock
// WhiteSkin is used. The card lives on the ToolTip layer (Pick=false) and takes no
// mouse focus: it cannot take a click.

#include "readout_model.h"
#include "readout_text.h"
#include "screen_projection.h"

namespace totalrecruit
{
// Where the card went: over the head, beside the window, or hidden for want of a
// side (screen_projection.h PlaceCard), and its rectangle on the screen.
struct ReadoutPlacement
{
    CardPlacement placement;
    ScreenRect card;

    ReadoutPlacement() : placement(Placement_NoRoom), card() {}
};

// Lays out and places the card with its bottom edge just above (anchorX, anchorY),
// clamped inside the screen sideways. With `avoid` (the offer window), a card that
// would cover it steps sideways beside it at the same height, and is hidden when
// neither side has room. Builds the widgets on first use. False when they could not
// be built (the card stays hidden; the rest of the mod is untouched). The widgets
// are only laid out again when the view or its texts differ from the last call; an
// identical card is just placed.
bool ReadoutUiShow(const ReadoutView& view, const ReadoutTexts& texts, bool compact,
                   int anchorX, int anchorY, const ScreenRect* avoid, ReadoutPlacement* out);

// Places a card that is already built, keeping its size, by the same rule. Cheap
// enough for every frame.
void ReadoutUiMove(int anchorX, int anchorY, const ScreenRect* avoid, ReadoutPlacement* out);

void ReadoutUiHide();
bool ReadoutUiVisible();

// Destroys every widget (shutdown). A load only hides: the widgets belong to the
// GUI, not to a world.
void ReadoutUiDestroy();
} // namespace totalrecruit
