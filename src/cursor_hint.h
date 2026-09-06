#pragma once

namespace totalrecruit
{
// Shows the talk cursor over anyone this mod can recruit.
//
// Kenshi already signals "there is a conversation here" by swapping the cursor,
// and players read that without being taught. Without it a recruitable prisoner
// looks exactly like scenery, and the only way to find one is to right-click
// everything -- so the mod's whole surface is invisible until you go looking.
//
// The engine decides the cursor per target in ForgottenGUI::changeMouseCursor;
// this hooks that call and substitutes TALK_CURSOR when the target is someone
// the rules would talk to.
bool InstallCursorHint();
void ShutdownCursorHint();
} // namespace totalrecruit
