#pragma once

#include "entry_facts.h"
#include "route_rules.h"
#include "verdict.h"

class Character;

namespace totalrecruit
{
// Evaluates an NPC against the active preset: will they join, and if not, what is
// missing? Stateless: everything is read from the world at the moment of the call,
// so there is nothing to persist, nothing to desynchronise on load, and nothing to
// save-scum.
//
// It is a dispatcher. The engine is read once (character_facts.cpp), the route that
// answers first is chosen over plain facts (route_rules.cpp), that route decides
// (route_*.cpp) and the reason it names becomes the player's line (reason_text.cpp).
Verdict Evaluate(Character* character);

// What a peek saw on the way to its verdict, for the readout (readout_model.h).
struct EvaluationDetail
{
    // The route that answered; only meaningful when haveCore (the Cheat preset
    // answers before reading who they are).
    Route route;
    bool haveCore;
    CoreFacts core;
    // The hold clock was read (a peek only reads one that is already running).
    bool holdKnown;
    float hoursHeld;

    EvaluationDetail()
        : route(Route_FreeWorld), haveCore(false), core(), holdKnown(false), hoursHeld(0.0f)
    {
    }
};

// The same question Evaluate answers, through the same facts and the same route
// functions, asked as a bystander: it starts no hold clock, mints no id, speaks and
// logs nothing. Only the rescue route gets a roll key, read from an id the NPC
// already has (character_facts.cpp PeekRollKey), so the card and the click agree on
// a rescue; every other route has none, so its roll can never pass and the verdict
// is "what happens if luck does not carry them": its chance is the real chance,
// its reason is what would be needed next. Safe to ask every few hundred
// milliseconds about anyone in view; it never says whether the roll will pass.
Verdict EvaluatePeek(Character* character, EvaluationDetail* detail);

// Human-readable currency name, for logs and status lines.
const char* CurrencyName(Currency currency);
} // namespace totalrecruit
