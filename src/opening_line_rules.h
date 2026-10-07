#pragma once

// PURE: which line a person opens the recruit talk with (priority-one E).
//
// npc_speech.cpp SpeakOpeningLine used to hardcode these lines while
// messages.json shipped the same text, with more variants, under refusal.* and
// currency.* that nothing read. Now each branch names its messages.json key and
// carries the line it always said as the built-in text; npc_speech asks Say(key,
// fallback, npc), so a list in the file picks one entry per character (stable)
// and a missing key says exactly what it said before.
//
// The order is the old one: unreadable traits; a refusal, by its tone
// (reason_catalog.h RefusalToneFor; a faction band's own greeting is the engine
// side's, asked first); a rescue, by temperament; then what they want
// (currency): freedom, care, submission, nothing owed (only a yes speaks) and
// cats, by who they are (bandit, trader, slaver) and then temperament. Animals
// never get here (animal.* lines).
//
// No Kenshi / MyGUI include: tested in tests/rules_tests/opening_line_rules_tests.cpp.

#include "reason_catalog.h"
#include "temperament.h"
#include "verdict.h"

namespace totalrecruit
{
// Who asks for cats, from the engine's npc type (npc_speech maps OT_*).
enum OpeningSpeaker
{
    OpeningSpeaker_Other = 0,
    OpeningSpeaker_Bandit,
    OpeningSpeaker_Trader,
    OpeningSpeaker_Slaver
};

struct OpeningFacts
{
    bool traitsRead;          // false: the traits could not be read
    bool refused;             // verdict.currency == Currency_Refused
    RefusalTone tone;         // RefusalToneFor(verdict.reasonKey), for a refusal
    bool viaRescue;
    Currency currency;
    bool willJoin;
    Temperament temperament;
    OpeningSpeaker speaker;

    OpeningFacts()
        : traitsRead(false), refused(false), tone(RefusalTone_Default), viaRescue(false),
          currency(Currency_None), willJoin(false), temperament(Temperament_Normal),
          speaker(OpeningSpeaker_Other) {}
};

struct OpeningLine
{
    const char* key;       // messages.json path; "" = no line
    const char* fallback;  // the built-in text (UTF-8); "" with no line
};

OpeningLine OpeningLineFor(const OpeningFacts& facts);
} // namespace totalrecruit
