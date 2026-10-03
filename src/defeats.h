#pragma once

// Counting how many times the player has put a character down.
//
// The count is the spec's own measure of "beat them until they yield", and it
// has to survive a reload -- a prisoner who has been knocked out five times
// must not read as untouched because the player quit for lunch.
//
// The TotalSparring way, three modules with one job each:
//
//   defeat_rules          who gets the credit (DecideDefeatCredit).
//   defeat_episode_rules  one defeat per knockout: counted once while down,
//                         re-armed only when they are seen standing again.
//   defeat_ledger         how many, per CharKey. character_identity.cpp writes
//                         it into the character's own save state next to the
//                         persisted id and reads it back on load, so a reload
//                         restores exactly that save's count.
//
// This file is the engine side: the knockout hook, the right-click fallback and
// the "seen standing" feed. The beating itself is a native order now
// (cage_commands.h); a knockout the player ordered there is credited to the
// player by the hook (ClaimOrderedKnockoutCredit), because the engine's own
// record of who won may name nobody. The engine's own memory tags are no
// longer read: the engine writes LT_SQUAD_LOST_TO_ME_ONCE by itself, and reading
// it as a defeat handed never-beaten guards the field-yield route.

#include <string>

class Character;

namespace totalrecruit
{
// Records that the player may just have put this character down. Safe to call
// more than once for the same knockout: only the first sighting of an episode
// counts (defeat_episode_rules).
void NoteDefeat(Character* character);

// How many times the player has defeated them, as the ledger (and so the save)
// holds it. 0 for a character never counted.
int DefeatCount(Character* character);

// The right-click: a fallback for the knockouts MedicalSystem::knockout does not
// cover -- bleeding out among them. Counts once per episode if they are down and
// the credit is the player's; re-arms the episode if they are standing.
void NoteIfDown(Character* character);

// Somewhere that already looks at this NPC (the hail sweep, the readout card)
// saw them. If they are on their feet the next knockout is a new episode. Never
// counts, never mints an id, and only reads the engine for a character whose
// episode is still marked counted.
void NoteSeenStanding(Character* character);

// R2.3 -- whether the popup offers the GOLPEARLO remedy for this character: in
// the player's own cage, awake, beating allowed, and the cooldown since the last
// counted defeat has run. The beating itself is cage_commands.h.
bool CanBeatPrisoner(Character* character, double* outHoursLeft);

// Who is told when a defeat leaves someone close enough to death to plead. The
// defeat tracker only knows that it happened; what a plea does (a window, three
// answers) is mercy.cpp's business, and mercy.cpp already depends on the recruit
// path, which depends on this module. Naming the listener here, and wiring it in
// startPlugin, keeps that chain one-directional instead of a ring.
typedef void (*NearDeathListener)(Character* character);
void SetNearDeathListener(NearDeathListener listener);

// The save-state bridge, for the saved_ints.h table only (character_identity.cpp
// loops over it, key "total_recruit.player_defeats"): the count to write
// into the save state of the character carrying `persistedId`, and the count its
// save state held when it loaded (absent key: not called). Thread-safe -- the
// serialise hook is not guaranteed to run on the game thread.
int SavedDefeatCount(const std::string& persistedId);
void RestoreSavedDefeatCount(const std::string& persistedId, int count);

// Installs the knockout hook.
bool InstallDefeatTracking();
void RemoveDefeatTracking();

// Drops every defeat count and episode. The load boundary calls it at load
// begin: the counts belong to the world that was just abandoned, and each
// character's load hook puts back what the loaded save holds.
void ForgetAllDefeats();
} // namespace totalrecruit
