#pragma once

// A small card over the head of an NPC who wants to talk to the player.
//
// The roller says "X quiere hablar contigo", and a player who did not catch the name
// does not know which of the people on screen it was. So whoever calls out also gets
// a compact card over their head -- their name and the line ("Quiere hablar
// contigo", or the event's own words) -- in the readout card's look, on the same
// ToolTip layer, never taking the mouse.
//
// It stays for headNoticeSeconds (config, default 300, Mod Hub), counted in real
// seconds frozen while the game is paused, and then goes by itself. Selecting that
// NPC (left click) dismisses it for good: the normal readout card takes the head, so
// the two never overlap. Selecting someone else does not touch it, and neither does a
// click elsewhere. If the normal card (about another NPC) would cover it, the normal
// card wins and the notice hides for that frame. It hides while the head is off the
// screen and while a right-click menu is open, steps sideways beside the offer window
// (screen_projection.h PlaceCard), and is dropped when the NPC dies or joins the
// squad. At most kHeadNoticeCap at once, one per NPC (head_notice_rules.h).
//
// Callers today: hail.cpp (a ready captive calls out) and submission_director.cpp (a
// beaten NPC walks up to ask to join). Any future "X wants to talk to you" event
// raises one with RaiseHeadNotice.

#include <string>

class Character;

namespace totalrecruit
{
// Puts a notice over this NPC's head with their name and `text` (one short line,
// player-facing, from messages.json). Raising again for the same NPC replaces the
// line and restarts the time. Does nothing when the mod is off or the NPC cannot be
// identified.
void RaiseHeadNotice(Character* npc, const std::string& text);

// Takes this NPC's notice down now: the conversation it announced has happened or
// was called off. Safe for an NPC without one.
void DismissHeadNotice(Character* npc);

// Called once per frame from the frame tick, after ReadoutTick (it reads where the
// normal card is this frame).
void HeadNoticeTick();

// Drops every notice (raw Character* inside): the load boundary calls it.
void ForgetHeadNotices();

// Destroys the widgets (plugin shutdown).
void ShutdownHeadNotices();
} // namespace totalrecruit
