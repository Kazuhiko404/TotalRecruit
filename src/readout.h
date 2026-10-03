#pragma once

// The readout, end to end: decide who the card is about and when, ask the rules about
// them as a bystander (EvaluatePeek: no hold clock, no id, no roll), turn the answer
// into what to show (readout_model.h) and the words for it (readout_text.h), and
// draw it (readout_ui.h).
//
// Two ways in:
//   - Selection (readoutOnSelect, the default): the NPC the player selected with a
//     left click, never one of their own. Each frame only compares the selection
//     with last frame's (one field read); the card is worked out once when it
//     changes and again every kSelectionRefreshMs, so live numbers (price, stress)
//     stay roughly current without an evaluation per frame. Deselecting hides it,
//     selecting someone else switches it.
//   - Pinned: pressing Hablar calls RequestReadout. The card is worked out at the
//     click and stays over the NPC's head while the offer window that the same click
//     opened is up; only when it would cover that window does it step sideways to
//     the window's nearer side at the head's height (hidden when no side has room,
//     never moved up to the top of the screen). When that window closes (or after
//     kPinTimeoutMs when none opened) the pin ends: the card stays only if that NPC
//     is still the selection.
// Either way the card follows the NPC every frame, is hidden while they are off the
// screen and while a right-click menu is open (other mods' buttons flank the menu on
// both sides, so there is no side to step to), and goes when they die. It never takes
// input (ToolTip layer, no mouse focus).
//
// With probeOnClick on, a line is logged whenever what is drawn changes:
//   [readout] <name> route=<r> chance=<n> stress=<s> ... [hablar|select]
// and it states the numbers the card shows, so a screenshot and the log can be
// compared. And once per outcome per target, where the card went:
//   [readout-anchor] <name> [select|hablar] <outcome> head=(x,y) view=WxH
//       world=(x,y,z) origin=(x,y,z) [card=(l,t wxh)] [offer=(l,t wxh)]

class Character;

namespace totalrecruit
{
// Pins the card to this NPC (the Hablar click). Does nothing when the readout is
// off or the NPC is not someone the mod talks to. Replaces any card showing.
void RequestReadout(Character* character);

// Called once per frame from the frame tick: follows the selection, moves the card,
// ends the pin, and refreshes the card on its slow interval.
void ReadoutTick();

// Hides the card and forgets the target (a raw pointer): a load frees characters.
void ForgetReadout();

void ShutdownReadout();
} // namespace totalrecruit
