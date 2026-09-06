#pragma once

namespace totalrecruit
{
// Installs the context-menu hooks that show a "Recruit" button anchored to
// Kenshi's own right-click menu. Returns false if a hook could not be placed.
bool InstallRecruitOverlay();

// Destroys the button and drops any target reference.
void ShutdownRecruitOverlay();
} // namespace totalrecruit
