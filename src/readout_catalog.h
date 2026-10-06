#pragma once

// The short line the readout shows for each thing that can stand in an NPC's way,
// without the engine.
//
// reason_catalog.cpp holds the full sentence the NPC's window reads; the card has
// room for about forty characters, so it has its own line per reason. A line is
// identified by an id: a ReasonKey (verdict.h) for what the rules said, plus the
// lines that are not a verdict reason (the odds of a roll
// the card could not make, the field-yield "beat them again"). Add a ReasonKey and
// the test fails until its row is here too. The route labels live here too, so the
// card and the recruit popup read one table.

#include "verdict.h"
#include "readout_model.h"

namespace totalrecruit
{
// The id of "{chance}%: puede unirse si le hablas": a roll the card could not make
// (the peek carries no roll key outside the rescue), so it states the odds
// instead of a "not yet".
const int kLineRollOpen = Reason_Count;
// The id of "Aun no cede: vencelo otra vez": the field-yield "not yet" for someone
// who is not in the player's cage, where starving them is not on offer.
const int kLineFieldYieldBeatAgain = Reason_Count + 1;
// The ids of a Balanced ally's discounted price (ally_tier_rules.h): the yes and the
// "not yet" the purse cannot meet. The verdict's reason is CatsJoin / CatsAsk like
// any other price; the card names the friendship on top.
const int kLineAllyPaidJoin = Reason_Count + 2;
const int kLineAllyPaidAsk = Reason_Count + 3;
// The id of "Temporal: le quedan {need} dias": a temporary squad member's card
// (temp_member.h). The days travel in the line's `need`.
const int kLineTempMemberDaysLeft = Reason_Count + 4;
// The id of "Ayudante: le quedan {need} dias": a free helper's card (helper.h). The
// days travel in the line's `need`.
const int kLineHelperDaysLeft = Reason_Count + 5;
// The ids of the head-to-head duel record of a challengeable ally
// (duel_challenge_rules.h): "Duelos: {food}-{need}" (wins in the line's `food`,
// losses in its `need`), and the same with the draws in `chance` when there are some.
const int kLineDuelRecord = Reason_Count + 6;
const int kLineDuelRecordWithDraws = Reason_Count + 7;
// The same record carrying what the next win does (duel_join_rules.h): Balanced
// "Duelos: {food}-{need}, si ganas {chance}% se une" (the chance in `chance`; with
// draws they move to `purse`), Cheat "..., si ganas se une" (draws in `chance`).
const int kLineDuelRecordChance = Reason_Count + 8;
const int kLineDuelRecordDrawsChance = Reason_Count + 9;
const int kLineDuelRecordSure = Reason_Count + 10;
const int kLineDuelRecordDrawsSure = Reason_Count + 11;
// The id of "Encerrado: {time}": how long a prisoner has been held in a cage the hold
// clock counts (prisoner-hold-time). The hours travel in the line's `hours`, written
// by hold_duration_rules.h.
const int kLineHoldElapsed = Reason_Count + 12;
// The record while the next win is still under the minimum wins (duel_join_rules.h,
// duel-any-source U4): "Duelos: {food}-{need}, faltan {chance} victorias" (the count in
// `chance`, always 2 or more: one missing win means the next one decides), and with
// the draws in `purse` ("..., faltan {chance}": "victorias" does not fit 40 glyphs).
const int kLineDuelRecordNeedWins = Reason_Count + 13;
const int kLineDuelRecordDrawsNeedWins = Reason_Count + 14;

int ReadoutLineCount();
// messages.json path ("readout.line.cats.ask") and the built-in text used when the
// file has no entry. Placeholders: {cats} {purse} {hours} {time} {food} {need} {chance}. Empty text
// means the card is not shown for that reason at all.
const char* ReadoutLineMessageKey(int id);
const char* ReadoutLineDefaultText(int id);

// The route tag both the card and the recruit popup show: its messages.json path
// ("readout.route.fieldYield") and the built-in label used when the file has none.
const char* ReadoutRouteMessageKey(ReadoutRoute route);
const char* ReadoutRouteDefaultText(ReadoutRoute route);
} // namespace totalrecruit
