#pragma once

// The readout, end to end: every few hundred milliseconds find who the player is
// pointing at (readout_target.h), ask the rules about them as a bystander
// (EvaluatePeek: no hold clock, no id, no roll), turn the answer into what to show
// (readout_model.h) and the words for it (readout_text.h), and draw it
// (readout_ui.h). The card follows the NPC every frame between those updates.
//
// With probeOnClick on, a line is logged whenever what is drawn changes:
//   [readout] <name> route=<r> chance=<n> stress=<s> ...
// and it states the numbers the card shows, so a screenshot and the log can be
// compared.

namespace totalrecruit
{
// Called once per frame from the frame tick; throttles itself.
void ReadoutTick();

// Hides the card and forgets the target (a raw pointer): a load frees characters.
void ForgetReadout();

void ShutdownReadout();
} // namespace totalrecruit
