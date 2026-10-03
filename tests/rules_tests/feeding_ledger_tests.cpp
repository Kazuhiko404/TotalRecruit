// Tests for src/feeding_ledger: the meals the player gave each animal, as the
// taming route and the save state see them.

#include "test_support.h"
#include "feeding_ledger.h"

using namespace totalrecruit;

namespace
{
void EachMealCountsOnce()
{
    const char* name = "feeding ledger: every meal from the player adds one, per character";
    ForgetAllPlayerMeals();
    const CharKey goat(7u, 1u);
    const CharKey bull(8u, 1u);
    CHECK(PlayerMeals(goat) == 0);
    CHECK(RecordPlayerMeal(goat) == 1);
    CHECK(RecordPlayerMeal(goat) == 2);
    CHECK(PlayerMeals(goat) == 2);
    CHECK(PlayerMeals(bull) == 0);
    ForgetAllPlayerMeals();
}

void TheSaveStateRoundTripsByPersistedId()
{
    const char* name = "feeding ledger: what the save held comes back under the persisted id";
    ForgetAllPlayerMeals();
    RestoreSavedPlayerMeals("goat-id", 3);
    CHECK(SavedPlayerMeals("goat-id") == 3);
    CHECK(PlayerMeals(CharKey::FromId("goat-id")) == 3);
    RecordPlayerMeal(CharKey::FromId("goat-id"));
    CHECK(SavedPlayerMeals("goat-id") == 4);

    // No id: nothing to write and nothing restored.
    RestoreSavedPlayerMeals("", 5);
    CHECK(SavedPlayerMeals("") == 0);
    ForgetAllPlayerMeals();
}

void ABadSavedValueIsMadeSafe()
{
    const char* name = "feeding ledger: a negative saved value is 0, an absurd one is capped";
    ForgetAllPlayerMeals();
    RestoreSavedPlayerMeals("a", -4);
    CHECK(SavedPlayerMeals("a") == 0);
    RestoreSavedPlayerMeals("b", 999999);
    CHECK(SavedPlayerMeals("b") == kMaxSavedMeals);
    // A save that says 0 forgets a count the session had.
    RecordPlayerMeal(CharKey::FromId("c"));
    RestoreSavedPlayerMeals("c", 0);
    CHECK(SavedPlayerMeals("c") == 0);
    ForgetAllPlayerMeals();
}

void ALoadForgetsEveryMeal()
{
    const char* name = "feeding ledger: load begin forgets every count";
    RecordPlayerMeal(CharKey(9u, 1u));
    ForgetAllPlayerMeals();
    CHECK(PlayerMeals(CharKey(9u, 1u)) == 0);
}
} // namespace

void RunFeedingLedgerTests()
{
    EachMealCountsOnce();
    TheSaveStateRoundTripsByPersistedId();
    ABadSavedValueIsMadeSafe();
    ALoadForgetsEveryMeal();
}
