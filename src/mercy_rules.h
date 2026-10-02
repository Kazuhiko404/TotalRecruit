#pragma once

// What each answer to a plea for mercy does, without the engine (REQUISITOS
// R3.6 and R3.7).
//
// Three answers, because two would be a lie: joining and killing are not the
// only things a player can do to someone bleeding in the dirt.
//   Accept -- they join. The plea itself said "I will do what you ask", so
//             nothing is priced and nothing is refused.
//   Ignore -- they lose all aggro and leave. Whoever can buy their life leaves
//             what they carry behind, which is what gives the button a cost.
//   Reject -- nothing changes and the fight goes on.

#include "temperament.h"

namespace totalrecruit
{
enum PleaAnswer
{
    PleaAnswer_Accept,
    PleaAnswer_Ignore,
    PleaAnswer_Reject
};

enum PleaOutcome
{
    // Recruited on the spot, free of charge.
    PleaOutcome_Join,
    // Let go, leaving their purse behind.
    PleaOutcome_LetGoWithTribute,
    // Let go with nothing gained.
    PleaOutcome_LetGo,
    // Aggro stays; the fight continues.
    PleaOutcome_FightOn
};

// R3.6 -- a coward, a turncoat, a dullard, a cruel or a maniacal one will offer
// whatever they carry. An honourable or a brave one joins or dies on their feet;
// letting them purchase an exit would contradict the one thing that defines
// them. Anyone else (and anyone unread) cannot buy their life either.
bool CanBuyTheirLife(Temperament personality);

PleaOutcome ResolvePlea(PleaAnswer answer, Temperament personality);
} // namespace totalrecruit
