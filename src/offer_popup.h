#pragma once

#include <string>

class Character;

namespace totalrecruit
{
// The decision window.
//
// Kenshi's own dialogue window can only show a conversation that exists as FCS
// data, which is exactly the dependency this mod is built to avoid -- an NPC
// from a mod nobody patched has no such data. So the window is built here, at
// runtime, out of MyGUI widgets wearing Kenshi's own skins. It reads as native
// because the skins are native.
//
// The NPC still speaks in a real speech bubble over their head; this window
// only carries the choice.
//
// Returns false when the window could not be created, so the caller can fall
// back to acting directly rather than leaving the player with nothing.
// extraLabel adds a third button when there is something the player can DO
// about the refusal -- feeding a starving captive, say. Pass an empty string
// for the usual two-button form.
bool ShowOffer(Character* npc, const std::string& npcLine,
               const std::string& acceptLabel, const std::string& declineLabel,
               const std::string& extraLabel);

// True while the offer is on screen. The context-menu overlay hides itself
// during that time so the two do not fight for the same corner.
bool IsOfferOpen();

// Where the open offer window is on screen, so the readout card pinned by the
// same click can stand beside it instead of on top of it. False when closed.
bool OfferWindowRect(int* left, int* top, int* width, int* height);

void HideOffer();
void ShutdownOfferPopup();
} // namespace totalrecruit
