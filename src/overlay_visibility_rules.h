#pragma once

// When TotalRecruit's world overlays -- the readout card over the selected NPC
// (readout.h) and the "wants to talk" head notices (head_notice.h) -- step aside for
// one of Kenshi's own windows, without the engine.
//
// Both overlays live on the ToolTip layer, above Kenshi's dialogue window, and
// follow a head that stands in the middle of the screen while the player talks to
// that NPC: the card covered the conversation (user report 2026-10-03). So while a
// conversation or a modal message box is open, both are hidden (kept, not
// dismissed), and they come back on the first frame after it closes.
//
// native_windows.h does the engine read; this is the one place that decides what it
// means, so the card and the notices can never disagree about it.
//
// Not covered on purpose: trade and inventory windows. ForgottenGUI only says "any
// inventory window is open", which is also true for the player's own inventory, and
// hiding the card whenever the player looks at their bag would be a new bug.

namespace totalrecruit
{
struct NativeWindowReading
{
    // The engine read worked. When it did not, the other fields mean nothing.
    bool read;
    // ForgottenGUI::inDialogue: the player is in a conversation.
    bool inDialogue;
    // ForgottenGUI::dialogue (the DialogueWindow) reports itself visible.
    bool dialogueWindowVisible;
    // ForgottenGUI::hasModalMessage: a modal message box is up.
    bool modalMessage;

    NativeWindowReading()
        : read(false), inDialogue(false), dialogueWindowVisible(false), modalMessage(false)
    {
    }
};

// True when the overlays must hide this frame: a conversation (either engine signal)
// or a modal message box is open. A failed read never hides them: they behave as
// they did before rather than vanishing for good on a bad pointer.
bool OverlaysYieldToNativeWindow(const NativeWindowReading& reading);
} // namespace totalrecruit
