// Tests for src/level_rules.cpp: the overall skill level rounded exactly the way
// TotalSparring's duel card rounds it (chance_card_model.cpp RoundLevel), so both
// mods print the same "Lv N" / "nivel N".

#include "test_support.h"
#include "level_rules.h"

using namespace totalrecruit;

namespace
{
void HalfwayRoundsUp()
{
    const char* name = "HalfwayRoundsUp";
    CHECK(RoundLevel(83.5f) == 84);
    CHECK(RoundLevel(0.5f) == 1);
}

void BelowHalfwayRoundsDown()
{
    const char* name = "BelowHalfwayRoundsDown";
    CHECK(RoundLevel(84.49f) == 84);
    CHECK(RoundLevel(76.2f) == 76);
    CHECK(RoundLevel(0.49f) == 0);
}

void TheEndsOfTheScaleStay()
{
    const char* name = "TheEndsOfTheScaleStay";
    CHECK(RoundLevel(0.0f) == 0);
    CHECK(RoundLevel(100.0f) == 100);
    CHECK(RoundLevel(99.5f) == 100);
}

void NegativesRoundAwayFromZero()
{
    // TotalSparring rounds a negative gap this way; the same formula here.
    const char* name = "NegativesRoundAwayFromZero";
    CHECK(RoundLevel(-0.4f) == 0);
    CHECK(RoundLevel(-0.5f) == -1);
    CHECK(RoundLevel(-8.6f) == -9);
}
} // namespace

void RunLevelRulesTests()
{
    HalfwayRoundsUp();
    BelowHalfwayRoundsDown();
    TheEndsOfTheScaleStay();
    NegativesRoundAwayFromZero();
}
