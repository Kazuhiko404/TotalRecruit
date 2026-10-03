#pragma once

#include <string>

#include "offer_view.h"

class Character;

namespace totalrecruit
{
// The recruit popup: every Hablar outcome opens it.
//
// Kenshi's own dialogue window can only show a conversation that exists as FCS
// data, which is exactly the dependency this mod is built to avoid -- an NPC
// from a mod nobody patched has no such data. So the window is built here, at
// runtime: Kenshi's Kenshi_WindowCX frame with the mockup's flat art inside
// (ui-mockup/readout.html, section 5).
//
// It draws an OfferView (offer_view.h) and decides nothing: which buttons exist,
// the price, the purse, the red line all come from the view. An empty accept or
// extra label hides that button; Cancel is always there, and so is the title-bar
// X, which means the same.
//
// The NPC still speaks in a real speech bubble over their head; this window
// carries the decision.
//
// Returns false when the window could not be created (logged).
bool ShowOffer(Character* npc, const OfferView& view);

// True while the offer is on screen. The context-menu overlay hides itself
// during that time so the two do not fight for the same corner.
bool IsOfferOpen();

// Where the open offer window is on screen, so the readout card pinned by the
// same click can stand beside it instead of on top of it. False when closed.
bool OfferWindowRect(int* left, int* top, int* width, int* height);

void HideOffer();
void ShutdownOfferPopup();
} // namespace totalrecruit
