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
//   2. A precise session count in the plugin, keyed by CharKey (the persisted
//      character id, the engine handle as fallback) rather than by pointer so a
//      reused address cannot inherit somebody else's history. Dropped when a
//      save loads (load_boundary.cpp).
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

// The same, for a knockout the player's own action caused (the beating,
// BeatPrisoner): the engine is not asked who put them down, because the plugin
// already knows. The debounce and the own-side filter still apply.
void CreditDefeatToPlayer(Character* character);

// How many times, as best we can tell. Never below the persistent floor.
int DefeatCount(Character* character);

// Counts a defeat if this character is currently out cold. A fallback for the
// knockouts MedicalSystem::knockout does not cover -- bleeding out among them.
void NoteIfDown(Character* character);

// R2.3 -- rough up a prisoner in the player's own cage. Returns false with a
// reason when it is not allowed: too soon, not caged, or already down.
bool BeatPrisoner(Character* character, std::string* outMessage);

// Whether beating this character is currently possible, for deciding whether to
// offer the button at all.
bool CanBeatPrisoner(Character* character, double* outHoursLeft);

// Who is told when a defeat leaves someone close enough to death to plead. The
// defeat tracker only knows that it happened; what a plea does (a window, three
// answers) is mercy.cpp's business, and mercy.cpp already depends on the recruit
// path, which depends on this module. Naming the listener here, and wiring it in
// startPlugin, keeps that chain one-directional instead of a ring.
typedef void (*NearDeathListener)(Character* character);
void SetNearDeathListener(NearDeathListener listener);

// Installs the knockout hook.
bool InstallDefeatTracking();
void RemoveDefeatTracking();

// Drops every session defeat count. The load boundary calls it: the counts
// belong to the world that was just abandoned (the engine's own per-character
// floor, layer 1 above, is what carries over a load).
void ForgetAllDefeats();
} // namespace totalrecruit
