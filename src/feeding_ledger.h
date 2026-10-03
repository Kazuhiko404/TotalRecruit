#pragma once

// How many meals the player has given each animal, kept for the life of the
// save.
//
// A wild animal sits at a full stomach on its own, so "full enough" alone made
// every beast tameable by anyone who spoke to it. The taming route asks this
// instead: did it ever eat from the player's hand (AnimalExtra::fedByPlayer,
// route_animal.cpp)?
//
// Units: meals -- one per successful FeedCaptive on an animal (feeding.cpp),
// whatever the item and however much it filled. Capped at kMaxSavedMeals.
//
// Persisted per character as idata "total_recruit.fed_by_player" through the
// saved-int bridge (saved_ints.h), next to the persisted id, exactly like the
// defeat count: cleared at load begin (load_boundary.cpp), refilled by each
// character's load hook, read back by the serialise hook. Keyed by CharKey: a
// handle-keyed count works for the session but only an id reaches a save.
//
// Thread-safe: the serialise hook is not guaranteed to run on the game thread.
// No engine include -- tested in tests/rules_tests/feeding_ledger_tests.cpp.

#include "char_key.h"

#include <string>

namespace totalrecruit
{
// The most meals a save may claim; past this is corruption or another mod's key.
extern const int kMaxSavedMeals;

// One more meal from the player; returns the new count.
int RecordPlayerMeal(const CharKey& key);

// Meals the player has given this character; 0 when never fed.
int PlayerMeals(const CharKey& key);

// The saved-int bridge (saved_ints.cpp): the count to write into the save state
// of the character carrying `persistedId`, and the count that save state held.
// A restored value is sanitised: negative is 0, past kMaxSavedMeals is capped.
int SavedPlayerMeals(const std::string& persistedId);
void RestoreSavedPlayerMeals(const std::string& persistedId, int meals);

// Load begin: the counts belong to the world that was just abandoned.
void ForgetAllPlayerMeals();
} // namespace totalrecruit
