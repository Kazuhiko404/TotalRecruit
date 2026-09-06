#pragma once

class Character;

namespace totalrecruit
{
// Route A -- rescue.
//
// You found them bleeding out and chose to save them. No cage, no price, no
// negotiation. It is the only route in Kenshi where the player is not
// exercising power, and no existing recruitment mod has it.
//
// Two hooks carry it:
//   applyFirstAid / applyDoctoring -- these take the healer as an argument, so
//   the plugin can tell "the player saved them" from "they recovered on their
//   own", which is what the user asked for.
//   sendDialogEvent -- EV_GET_UP_PEACE fires when they get back on their feet,
//   which is when the offer should come, not the moment healing ends.
bool InstallRescueHooks();
void ShutdownRescueHooks();

// Called once per frame from the GUI tick. Completes any recruit the player
// accepted in dialogue, which cannot be done inside the dialogue hook itself.
void RescueTick();
} // namespace totalrecruit
