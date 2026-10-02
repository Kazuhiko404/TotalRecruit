#pragma once

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

// Human-readable currency name, for logs and status lines.
const char* CurrencyName(Currency currency);
} // namespace totalrecruit
