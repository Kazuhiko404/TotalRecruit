#pragma once

// The engine side of "is one of Kenshi's own windows open?", for the overlays that
// must step aside for it (overlay_visibility_rules.h decides; this only reads).
//
// The reads, all on ForgottenGUI (the global `gui`), all non-virtual:
//   - inDialogue()        the player is in a conversation. TotalLoot's pickpocket
//                         button already hides on it (pickpocket_reads.cpp).
//   - dialogue (0x20)     the DialogueWindow, whose getVisible() is the window itself.
//   - hasModalMessage()   a modal message box is up (also TotalLoot's check).
// Two dialogue signals on purpose: either one alone hides the overlays, so a frame
// where one lags the other does not flash the card over the conversation.
//
// Cost: three calls into the engine per call, no allocation. readout.cpp and
// head_notice.cpp each ask once per frame.

#include "overlay_visibility_rules.h"

namespace totalrecruit
{
// Fills the reading; `read` is false when `gui` is not there or a read faulted.
NativeWindowReading ReadNativeWindows();

// ReadNativeWindows + OverlaysYieldToNativeWindow: true while the readout card and
// the head notices must stay hidden. With probeOnClick, logs each change once:
//   [native-window] open: dialogue=<0|1> window=<0|1> modal=<0|1>
//   [native-window] closed
bool NativeWindowCoversOverlays();
} // namespace totalrecruit
