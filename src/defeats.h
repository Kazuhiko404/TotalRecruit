#pragma once

// Counting how many times the player has put a character down.
//
// The count is the spec's own measure of "beat them until they yield", and it
// has to survive a reload -- a prisoner who has been knocked out five times
// must not read as untouched because the player quit for lunch.
//
// Nothing here can be one number. Three layers, each covering the previous
// one's blind spot:
//
//   1. A floor written into the engine's per-character memory. Coarse, and the
//      only part that survives a save.
//   2. A precise session count in the plugin, keyed by handle rather than by
//      pointer so a reused address cannot inherit somebody else's history.
//   3. Present condition -- hunger and wounds -- which needs no count at all.
//
// A reload costs precision, never progress.

#include <string>

class Character;

namespace totalrecruit
{
// Records that the player just put this character down. Safe to call more than
// once for the same knockout; a repeat inside the debounce window is ignored.
void NoteDefeat(Character* character);

// How many times, as best we can tell. Never below the persistent floor.
int DefeatCount(Character* character);

// R2.3 -- rough up a prisoner in the player's own cage. Returns false with a
// reason when it is not allowed: too soon, not caged, or already down.
bool BeatPrisoner(Character* character, std::string* outMessage);

// Whether beating this character is currently possible, for deciding whether to
// offer the button at all.
bool CanBeatPrisoner(Character* character, double* outHoursLeft);

// Installs the knockout hook.
bool InstallDefeatTracking();
void RemoveDefeatTracking();
} // namespace totalrecruit
