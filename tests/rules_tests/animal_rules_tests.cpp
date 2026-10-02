// Tests for src/animal_rules.cpp: the taming bar. The first block pins the bar as
// it shipped (the arithmetic moved out of rules.cpp unchanged, with the config
// defaults); the T8 block states that the edge thresholds and the temperament
// factors now come from the config.

#include "test_support.h"
#include "animal_rules.h"
#include "chance_rules.h"

using namespace totalrecruit;

namespace
{
const Animals kAnimals = Animals();

bool Near(float actual, float expected)
{
    const float d = actual - expected;
    return d < 0.0001f && d > -0.0001f;
}

void ACubOrAWeaklingNeedsAMouthful()
{
    const char* name = "taming bar: a cub or a weakling needs cubFullness";
    // Maturity under the cub threshold, whatever its level.
    CHECK(Near(TamingFullnessNeeded(0.10f, 50, 10, kAnimals), 0.70f));
    // A grown animal at or below the weak level.
    CHECK(Near(TamingFullnessNeeded(1.0f, 10, 10, kAnimals), 0.70f));
    CHECK(Near(TamingFullnessNeeded(1.0f, 3, 60, kAnimals), 0.70f));
}

void AGrownBeastWantsMore()
{
    const char* name = "taming bar: base plus maturity weight for a grown animal";
    // 0.80 + 1.0 * 0.15
    CHECK(Near(TamingFullnessNeeded(1.0f, 20, 20, kAnimals), 0.95f));
    // 0.80 + 0.5 * 0.15
    CHECK(Near(TamingFullnessNeeded(0.5f, 20, 20, kAnimals), 0.875f));
}

void AStrongAnimalWantsAPenaltyOnTop()
{
    const char* name = "taming bar: above strongLevel it asks for strongPenalty more, never past a full stomach";
    // 0.80 + 0.15 + 0.05 = 1.0
    CHECK(Near(TamingFullnessNeeded(1.0f, 31, 31, kAnimals), 1.0f));
    CHECK(Near(TamingFullnessNeeded(1.0f, 30, 30, kAnimals), 0.95f));
    CHECK(Near(TamingFullnessNeeded(1.0f, 80, 80, kAnimals), 1.0f));
}

void OutLevellingItDiscountsTheBar()
{
    const char* name = "taming bar: 10 levels over discounts 0.05, 20 over discounts 0.10";
    CHECK(Near(TamingFullnessNeeded(1.0f, 20, 29, kAnimals), 0.95f));
    CHECK(Near(TamingFullnessNeeded(1.0f, 20, 30, kAnimals), 0.90f));
    CHECK(Near(TamingFullnessNeeded(1.0f, 20, 39, kAnimals), 0.90f));
    CHECK(Near(TamingFullnessNeeded(1.0f, 20, 40, kAnimals), 0.85f));
    CHECK(Near(TamingFullnessNeeded(1.0f, 20, 200, kAnimals), 0.85f));
}

void TheDiscountNeverGoesBelowACub()
{
    const char* name = "taming bar: never below cubFullness";
    Animals a;
    a.bigEdgeDiscount = 0.9f;
    CHECK(Near(TamingFullnessNeeded(1.0f, 20, 60, a), a.cubFullness));
}

void TemperamentAroundOneDoesNotMoveTheBar()
{
    const char* name = "temperament: resolve 1.0 leaves the bar where it was";
    CHECK(Near(ApplyAnimalTemperament(0.80f, 1.0f, kAnimals), 0.80f));
}

void TimidSettlesForLessAndStubbornAsksMore()
{
    const char* name = "temperament: a timid animal settles for less, a stubborn one asks for more, capped";
    // 0.85 + 0.6 * 0.15 = 0.94 ; 0.85 + 1.8 * 0.15 = 1.12
    CHECK(Near(ApplyAnimalTemperament(0.80f, 0.6f, kAnimals), 0.80f * 0.94f));
    CHECK(Near(ApplyAnimalTemperament(0.80f, 1.8f, kAnimals), 0.80f * 1.12f));
    CHECK(Near(ApplyAnimalTemperament(0.95f, 1.8f, kAnimals), 1.0f));
}

void MissingPercentRoundsToNearest()
{
    const char* name = "missing stomach: whole percent, rounded to nearest";
    CHECK(MissingStomachPercent(0.95f, 0.50f) == 45);
    CHECK(MissingStomachPercent(0.80f, 0.5549f) == 25);
    CHECK(MissingStomachPercent(0.80f, 0.5551f) == 24);
}

// --- T8: the thresholds and factors are config now.

void EdgeLevelsAreConfigurable()
{
    const char* name = "taming bar: the edge thresholds come from the config";
    Animals a;
    a.edgeLevel = 5;
    a.bigEdgeLevel = 8;
    // 5 levels over: the small discount now applies (0.95 - 0.05).
    CHECK(Near(TamingFullnessNeeded(1.0f, 20, 25, a), 0.90f));
    // 8 levels over: the big one (0.95 - 0.10).
    CHECK(Near(TamingFullnessNeeded(1.0f, 20, 28, a), 0.85f));
    // 4 over: nothing.
    CHECK(Near(TamingFullnessNeeded(1.0f, 20, 24, a), 0.95f));
    // The shipped 10 and 20 no longer decide.
    Animals far;
    far.edgeLevel = 50;
    far.bigEdgeLevel = 60;
    CHECK(Near(TamingFullnessNeeded(1.0f, 20, 45, far), 0.95f));
}

void TemperamentFactorsAreConfigurable()
{
    const char* name = "temperament: the base and weight come from the config";
    Animals a;
    a.temperamentBase = 1.0f;
    a.temperamentWeight = 0.0f;
    // No temperament effect at all.
    CHECK(Near(ApplyAnimalTemperament(0.80f, 0.6f, a), 0.80f));
    CHECK(Near(ApplyAnimalTemperament(0.80f, 1.8f, a), 0.80f));
    a.temperamentBase = 0.5f;
    a.temperamentWeight = 0.5f;
    // 0.5 + 1.0 * 0.5 = 1.0 ; 0.5 + 0.6 * 0.5 = 0.8
    CHECK(Near(ApplyAnimalTemperament(0.60f, 1.0f, a), 0.60f));
    CHECK(Near(ApplyAnimalTemperament(0.60f, 0.6f, a), 0.48f));
}

void DefaultsAreTheShippedValues()
{
    const char* name = "animal config: the shipped thresholds and factors";
    CHECK(kAnimals.edgeLevel == 10);
    CHECK(kAnimals.bigEdgeLevel == 20);
    CHECK(Near(kAnimals.temperamentBase, 0.85f));
    CHECK(Near(kAnimals.temperamentWeight, 0.15f));
}
} // namespace

void RunAnimalRulesTests()
{
    ACubOrAWeaklingNeedsAMouthful();
    AGrownBeastWantsMore();
    AStrongAnimalWantsAPenaltyOnTop();
    OutLevellingItDiscountsTheBar();
    TheDiscountNeverGoesBelowACub();
    TemperamentAroundOneDoesNotMoveTheBar();
    TimidSettlesForLessAndStubbornAsksMore();
    MissingPercentRoundsToNearest();
    EdgeLevelsAreConfigurable();
    TemperamentFactorsAreConfigurable();
    DefaultsAreTheShippedValues();
}
