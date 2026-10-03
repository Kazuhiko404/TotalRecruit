#pragma once

// The mouse, read cheaply and behind guards, for the readout card's dismissal
// (card_dismiss_rules.h). Nothing here keeps state or decides anything.

namespace totalrecruit
{
// Whether each button is held right now (GetAsyncKeyState, the same read
// TotalSparring's overlay dismissal uses). Physical buttons: swapped buttons are
// not mapped.
void ReadMouseButtons(bool* leftDown, bool* rightDown);

// True when MyGUI has a widget under the cursor (a window, a menu, a button). False
// when nothing is there or the read failed.
bool IsMouseOverMyGui();

// The cursor in MyGUI view pixels (the space ReadoutAnchorFor projects into).
// False when it could not be read.
bool ReadMousePosition(int* x, int* y);
} // namespace totalrecruit
