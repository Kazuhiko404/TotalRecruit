#pragma once

// Which animals the player has treated, kept for the life of the save.
//
// User request (2026-10-04, animal-duel-submission D4): in Cheat an animal the player
// HEALED joins like one the player fed. The engine names the healer itself: the rescue
// hooks on MedicalSystem::applyFirstAid / applyDoctoring (rescue.cpp) receive the
// character doing the treatment, so "healed by the player" is a fact, not a guess.
// animal_healing.cpp records it here when one of the player's characters treats an
// animal that is not the player's (CountsAsPlayerHealing); route_cheat.cpp reads it
// through CheatFacts::healedByPlayer.
//
// A flag, not a count: the hook runs on every frame of a treatment, and one bandage is
// the gesture. Persisted per character as idata "total_recruit.healed_by_player"
// (1 or 0) through the saved-int bridge (saved_ints.h), next to the persisted id, like
// the meals of feeding_ledger.h: cleared at load begin (load_boundary.cpp), refilled by
// each character's load hook. Keyed by CharKey.
//
// Thread-safe: the serialise hook is not guaranteed to run on the game thread.
// No engine include -- tested in tests/rules_tests/healing_ledger_tests.cpp.

#include "char_key.h"

#include <string>

namespace totalrecruit
{
// Whether a treatment counts as the player healing an animal: the healer is one of the
// player's characters, the patient is an animal, and that animal is not already the
// player's. A person is the rescue route's business (rescue.cpp), not this.
bool CountsAsPlayerHealing(bool healerIsPlayers, bool patientIsAnimal, bool patientIsPlayers);

// Marks this character as healed by the player; true the first time only.
bool RecordPlayerHealing(const CharKey& key);

// Whether the player ever healed this character.
bool HealedByPlayer(const CharKey& key);

// The saved-int bridge (saved_ints.cpp): 1 or 0 to write for the character carrying
// `persistedId`, and the value its save state held (positive = healed, else forgotten).
int SavedPlayerHealing(const std::string& persistedId);
void RestoreSavedPlayerHealing(const std::string& persistedId, int value);

// Load begin: the marks belong to the world that was just abandoned.
void ForgetAllPlayerHealing();
} // namespace totalrecruit
