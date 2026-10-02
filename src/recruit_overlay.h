#pragma once

class Character;

namespace totalrecruit
{
// Installs the context-menu hooks that show a "Recruit" button anchored to
// Kenshi's own right-click menu. Returns false if a hook could not be placed.
bool InstallRecruitOverlay();

// Destroys the button and drops any target reference.
void ShutdownRecruitOverlay();

// The character the open right-click menu is about, when the menu is open and the
// mod offers to talk to them; NULL otherwise. A raw pointer for the caller to
// read behind a guard and not keep (the readout asks every few hundred ms).
Character* OverlayMenuTarget();

// Drops the clicked target (raw engine pointers) and hides the button, so a
// load cannot leave it aimed at a character the engine just freed.
void ForgetOverlayTarget();
} // namespace totalrecruit
