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

namespace totalrecruit
{
// A screen rectangle the card must not cover (the offer window).
struct ReadoutRect
{
    int left;
    int top;
    int width;
    int height;
};

// Lays out and shows the card with its bottom edge just above (anchorX, anchorY),
// clamped inside the screen. Builds the widgets on first use. False when they could
// not be built (the card stays hidden; the rest of the mod is untouched). The
// widgets are only laid out again when the view or its texts differ from the last
// call; an identical card is just moved.
bool ReadoutUiShow(const ReadoutView& view, const ReadoutTexts& texts, bool compact,
                   int anchorX, int anchorY);

// Moves a card that is already showing, keeping its size. Cheap enough for every
// frame. With `avoid`, a card that would cover that rectangle steps beside it.
void ReadoutUiMove(int anchorX, int anchorY, const ReadoutRect* avoid = 0);

void ReadoutUiHide();
bool ReadoutUiVisible();

// Destroys every widget (shutdown). A load only hides: the widgets belong to the
// GUI, not to a world.
void ReadoutUiDestroy();
} // namespace totalrecruit
