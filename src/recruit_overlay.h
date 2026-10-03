#pragma once

class Character;

namespace totalrecruit
{
// Installs the context-menu hooks that show a "Recruit" button anchored to
// Kenshi's own right-click menu. Returns false if a hook could not be placed.
bool InstallRecruitOverlay();

// Destroys the button and drops any target reference.
void ShutdownRecruitOverlay();

// Drops the clicked target (raw engine pointers) and hides the button, so a
// load cannot leave it aimed at a character the engine just freed.
void ForgetOverlayTarget();
} // namespace totalrecruit
