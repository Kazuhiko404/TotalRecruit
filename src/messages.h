#pragma once

#include <string>

class Character;

namespace totalrecruit
{
// Every line an NPC speaks and every reason the player reads, loaded from
// messages.json so the text can be rewritten, translated or extended without
// touching the plugin.
//
// The mod ships working defaults for every key it asks for, so a missing,
// truncated or hand-broken file costs the player some flavour and nothing else.
// Text is never a reason to fail a recruit.
void LoadMessages(const std::string& modDirectory);

// Looks up a dotted path ("faction.aliado.greeting"). When the value is a list,
// one entry is chosen stably from `who`, so the same NPC always says the same
// thing while two NPCs sound different. Pass NULL for the first entry.
//
// A key that is missing from the file falls back to `fallback`, which is what
// makes an incomplete translation harmless.
std::string Say(const std::string& path, const std::string& fallback,
                Character* who);

// Logs every key the code asked for that messages.json did not have, once, as
// one line. Called on shutdown, after a session's worth of lookups, so an
// incomplete file names itself instead of hiding behind the fallback.
void ReportMissingKeys();

// Replaces {name}, {cats} and {hours} in a line that has already been chosen.
std::string Fill(const std::string& text, const std::string& name,
                 int cats, int hours);
} // namespace totalrecruit
