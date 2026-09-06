#pragma once

namespace totalrecruit
{
// A once-per-frame callback, hooked onto ForgottenGUI::update.
//
// It exists for one reason: the engine keeps both parties state-locked for the
// duration of a conversation, so a recruit the player accepted in dialogue has
// to be completed a frame later, after the window has closed. Mutating them
// inside the dialogue hook risks leaving the character in a broken state.
bool InstallFrameTick();
void ShutdownFrameTick();
} // namespace totalrecruit
