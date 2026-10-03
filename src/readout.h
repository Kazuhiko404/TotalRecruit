#pragma once

// The readout, end to end: decide who the card is about and when, ask the rules about
// them as a bystander (EvaluatePeek: no hold clock, no id, no roll), turn the answer
// into what to show (readout_model.h) and the words for it (readout_text.h), and
// draw it (readout_ui.h).
//
// Two ways in:
//   - Pinned (the default): pressing Hablar calls RequestReadout. The card is worked
//     out ONCE, there, and then only follows the NPC every frame. It stands beside
//     the offer window that the same click opened and is dismissed when that window
//     closes, after kPinTimeoutMs when no window opened, when the NPC dies or leaves
//     the screen, or when another request replaces it.
//   - Hover (readoutOnHover, opt-in): the NPC under the cursor, re-evaluated four
//     times a second. That sweep and evaluation cost frame time with many NPCs on
//     screen, which is why it is off by default. Hidden while the offer is open.
//
// With probeOnClick on, a line is logged whenever what is drawn changes:
//   [readout] <name> route=<r> chance=<n> stress=<s> ... [hablar|hover]
// and it states the numbers the card shows, so a screenshot and the log can be
// compared.

class Character;

namespace totalrecruit
{
// Pins the card to this NPC (the Hablar click). Does nothing when the readout is
// off or the NPC is not someone the mod talks to. Replaces any card showing.
void RequestReadout(Character* character);

// Called once per frame from the frame tick: moves the pinned card, dismisses it,
// or (hover mode) throttles its own re-evaluation.
void ReadoutTick();

// Hides the card and forgets the target (a raw pointer): a load frees characters.
void ForgetReadout();

void ShutdownReadout();
} // namespace totalrecruit
