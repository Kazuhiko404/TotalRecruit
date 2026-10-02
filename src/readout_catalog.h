#pragma once

// The short line the readout shows for each thing that can stand in an NPC's way,
// without the engine.
//
// reason_catalog.cpp holds the full sentence the NPC's window reads; the card has
// room for about forty characters, so it has its own line per reason. A line is
// identified by an id: a ReasonKey (verdict.h) for what the rules said, plus the
// one line that is not a verdict reason, the time left in the cage. Add a ReasonKey
// and the test fails until its row is here too.

#include "verdict.h"

namespace totalrecruit
{
// The id of "faltan {hours} en la jaula".
const int kLineWaitCage = Reason_Count;

int ReadoutLineCount();
// messages.json path ("readout.line.cats.ask") and the built-in text used when the
// file has no entry. Placeholders: {cats} {purse} {hours} {food} {need}. Empty text
// means the card is not shown for that reason at all.
const char* ReadoutLineMessageKey(int id);
const char* ReadoutLineDefaultText(int id);
} // namespace totalrecruit
