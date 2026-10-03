#pragma once

// The short line the readout shows for each thing that can stand in an NPC's way,
// without the engine.
//
// reason_catalog.cpp holds the full sentence the NPC's window reads; the card has
// room for about forty characters, so it has its own line per reason. A line is
// identified by an id: a ReasonKey (verdict.h) for what the rules said, plus the
// lines that are not a verdict reason (the time left in the cage, the odds of a roll
// the card could not make, the field-yield "beat them again"). Add a ReasonKey and
// the test fails until its row is here too. The route labels live here too, so the
// card and the recruit popup read one table.

#include "verdict.h"
#include "readout_model.h"

namespace totalrecruit
{
// The id of "faltan {hours} en la jaula".
const int kLineWaitCage = Reason_Count;
// The id of "{chance}%: puede unirse si le hablas": a roll the card could not make
// (the peek carries no roll key), so it states the odds instead of a "not yet".
const int kLineRollOpen = Reason_Count + 1;
// The id of "Aun no cede: vencelo otra vez": the field-yield "not yet" for someone
// who is not in the player's cage, where starving them is not on offer.
const int kLineFieldYieldBeatAgain = Reason_Count + 2;
// The ids of a Balanced ally's discounted price (ally_tier_rules.h): the yes and the
// "not yet" the purse cannot meet. The verdict's reason is CatsJoin / CatsAsk like
// any other price; the card names the friendship on top.
const int kLineAllyPaidJoin = Reason_Count + 3;
const int kLineAllyPaidAsk = Reason_Count + 4;
// The id of "Temporal: le quedan {need} dias": a temporary squad member's card
// (temp_member.h). The days travel in the line's `need`.
const int kLineTempMemberDaysLeft = Reason_Count + 5;

int ReadoutLineCount();
// messages.json path ("readout.line.cats.ask") and the built-in text used when the
// file has no entry. Placeholders: {cats} {purse} {hours} {food} {need} {chance}. Empty text
// means the card is not shown for that reason at all.
const char* ReadoutLineMessageKey(int id);
const char* ReadoutLineDefaultText(int id);

// The route tag both the card and the recruit popup show: its messages.json path
// ("readout.route.fieldYield") and the built-in label used when the file has none.
const char* ReadoutRouteMessageKey(ReadoutRoute route);
const char* ReadoutRouteDefaultText(ReadoutRoute route);
} // namespace totalrecruit
