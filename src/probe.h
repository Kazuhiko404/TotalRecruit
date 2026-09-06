#pragma once

#include <string>

class Character;

namespace totalrecruit
{
// Phase 1 smoke test.
//
// Everything the rule engine will read is verified against the KenshiLib
// headers but not against a running game. This dumps those same fields for
// one character so the pillars can be confirmed before anything is built on
// top of them.
//
// Returns a single-line summary for the status bar; the full report goes to
// the RE_Kenshi log.
std::string ProbeCharacter(Character* character);
} // namespace totalrecruit
