#pragma once

// The beating stress, engine side: what Golpear and Noquear leave on a prisoner
// in the player's own cage (the rule is beating_stress_rules.h).
//
//   AddBeatingStress   a blow that landed (cage_commands.cpp LandBeatHit) or a
//                      knockout that landed (ResolveCageKnockout): reads both
//                      levels and the temperament, adds the gain, logs it.
//   BeatingStressNow   the bar as it stands, faded to the current game hour: the
//                      fourth stress term (character_facts.cpp CoreFacts).
//
// The bar is saved with the character, two rows of the saved_ints.h table:
// "total_recruit.beating_stress" (thousandths) and
// "total_recruit.beating_stress_hour" (the game hour it was last written at, in
// tenths). The load boundary clears it (ForgetAllBeatingStress) and each
// character's load hook puts back what the loaded save holds, so a reload never
// keeps a later world's beatings. The serialise hook may run off the game
// thread: the ledger is behind a lock.
//
// Log: "[stress] <name> beating stress +X% (blow|knockout, player P vs npc N,
// resolve R) -> Y%" with probeOnClick; a failure to read the clock or key always.

#include <string>

#include "beating_stress_rules.h"

class Character;

namespace totalrecruit
{
// Adds one blow's or one knockout's stress to `target` and returns the gain (0..1;
// 0 when the character key or the game clock could not be read -- nothing added).
// Mints the persisted id when the character has none, like a counted defeat, so
// the bar reaches the save.
float AddBeatingStress(Character* target, BeatingBlow blow);

// The bar now, 0..1 (0 for someone never beaten, or unreadable). Never mints an id.
float BeatingStressNow(Character* character);

// The save-state bridge, for the saved_ints.h table only. Thread-safe.
int SavedBeatingStress(const std::string& persistedId);
void RestoreSavedBeatingStress(const std::string& persistedId, int value);
int SavedBeatingStressHour(const std::string& persistedId);
void RestoreSavedBeatingStressHour(const std::string& persistedId, int tenths);

// Load begin: the bars belong to the world that was just abandoned.
void ForgetAllBeatingStress();
} // namespace totalrecruit
