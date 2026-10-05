// Tests for src/healing_ledger: which animals the player has treated (first aid or
// doctoring by one of the player's characters), as Cheat and the save state see it, and
// which treatments count at all.

#include "test_support.h"
#include "healing_ledger.h"

using namespace totalrecruit;

namespace
{
void ATreatmentIsRecordedOnce()
{
    const char* name = "healing ledger: the first treatment is new, later ones are not";
    ForgetAllPlayerHealing();
    const CharKey goat(7u, 1u);
    const CharKey bull(8u, 1u);
    CHECK(!HealedByPlayer(goat));
    CHECK(RecordPlayerHealing(goat));
    CHECK(!RecordPlayerHealing(goat));   // every frame of the bandage calls it again
    CHECK(HealedByPlayer(goat));
    CHECK(!HealedByPlayer(bull));
    ForgetAllPlayerHealing();
    CHECK(!HealedByPlayer(goat));
}

void TheSaveStateRoundTripsByPersistedId()
{
    const char* name = "healing ledger: the saved flag comes back under the persisted id";
    ForgetAllPlayerHealing();
    CHECK(SavedPlayerHealing("goat-id") == 0);
    RestoreSavedPlayerHealing("goat-id", 1);
    CHECK(SavedPlayerHealing("goat-id") == 1);
    CHECK(HealedByPlayer(CharKey::FromId("goat-id")));

    // Any positive value is a yes; zero or less forgets it; no id is nothing.
    RestoreSavedPlayerHealing("odd", 42);
    CHECK(SavedPlayerHealing("odd") == 1);
    RestoreSavedPlayerHealing("odd", 0);
    CHECK(SavedPlayerHealing("odd") == 0);
    RestoreSavedPlayerHealing("neg", -3);
    CHECK(!HealedByPlayer(CharKey::FromId("neg")));
    RestoreSavedPlayerHealing("", 1);
    CHECK(SavedPlayerHealing("") == 0);
    ForgetAllPlayerHealing();
}

void OnlyThePlayersTreatmentOfSomeoneElsesAnimalCounts()
{
    const char* name = "healing ledger: a player's character treating an animal that is not the player's";
    CHECK(CountsAsPlayerHealing(true, true, false));
    CHECK(!CountsAsPlayerHealing(false, true, false));   // someone else bandaged it
    CHECK(!CountsAsPlayerHealing(true, false, false));   // a person (the rescue route's business)
    CHECK(!CountsAsPlayerHealing(true, true, true));     // already the player's animal
}
} // namespace

void RunHealingLedgerTests()
{
    ATreatmentIsRecordedOnce();
    TheSaveStateRoundTripsByPersistedId();
    OnlyThePlayersTreatmentOfSomeoneElsesAnimalCounts();
}
