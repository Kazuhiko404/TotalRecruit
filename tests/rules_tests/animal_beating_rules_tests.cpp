// Tests for src/animal_beating_rules and the caged-animal route that uses it
// (priority-one D): Golpear / Noquear on an animal in the player's cage leave
// beating stress, and in Balanced that stress lowers the stomach bar the animal
// must reach, divided by a resolve that grows with the animal's level. A loose
// animal and Cheat are untouched.

#include "test_support.h"
#include "animal_beating_rules.h"
#include "route_decisions.h"

#include <cmath>

using namespace totalrecruit;

namespace
{
bool Near(float a, float b)
{
    return std::fabs(a - b) < 0.001f;
}

Animals Defaults()
{
    return Animals();
}

void TheResolveGrowsWithTheLevel()
{
    const char* name = "animal beating: resolve = base + level * perLevel, never below the floor";
    const Animals a = Defaults();
    CHECK(a.cageBeatingCounts);
    CHECK(Near(a.cageBeatingWeight, 0.5f));
    CHECK(Near(AnimalBeatingResolve(0, a), 1.0f));
    CHECK(Near(AnimalBeatingResolve(50, a), 2.0f));
    CHECK(Near(AnimalBeatingResolve(-10, a), 1.0f));
    Animals low = a;
    low.cageBeatingResolveBase = 0.0f;
    low.cageBeatingResolvePerLevel = 0.0f;
    CHECK(Near(AnimalBeatingResolve(30, low), kAnimalBeatingResolveFloor));
}

void TheStressDiscountsTheBar()
{
    const char* name = "animal beating: discount = weight * stress / resolve, clamped; none without stress";
    const Animals a = Defaults();
    CHECK(Near(AnimalBeatingDiscount(0.0f, 20, a), 0.0f));
    CHECK(Near(AnimalBeatingDiscount(1.0f, 0, a), 0.5f));
    CHECK(Near(AnimalBeatingDiscount(1.0f, 50, a), 0.25f));
    CHECK(Near(AnimalBeatingDiscount(0.5f, 0, a), 0.25f));
    // A bar above 1 or below 0 reads clamped.
    CHECK(Near(AnimalBeatingDiscount(3.0f, 0, a), 0.5f));
    CHECK(Near(AnimalBeatingDiscount(-1.0f, 0, a), 0.0f));
    // Never the whole bar, whatever the config says.
    Animals big = a;
    big.cageBeatingWeight = 5.0f;
    CHECK(Near(AnimalBeatingDiscount(1.0f, 0, big), kAnimalBeatingMaxDiscount));
    CHECK(Near(ApplyAnimalBeating(0.9f, 1.0f, 0, a), 0.45f));
}

void SwitchedOffItDoesNothing()
{
    const char* name = "animal beating: off, or a weight of 0, leaves the bar alone";
    Animals off = Defaults();
    off.cageBeatingCounts = false;
    CHECK(Near(ApplyAnimalBeating(0.9f, 1.0f, 0, off), 0.9f));
    Animals none = Defaults();
    none.cageBeatingWeight = 0.0f;
    CHECK(Near(ApplyAnimalBeating(0.9f, 1.0f, 0, none), 0.9f));
}

CoreFacts CagedBeast(float hunger, float beatingStress)
{
    CoreFacts core;
    core.npcLevel = 20;
    core.playerLevel = 20;
    core.temperament = Temperament_Normal;
    core.animal = true;
    core.inMyCage = true;
    core.hunger = hunger;
    core.beatingStress = beatingStress;
    return core;
}

AnimalExtra Fed()
{
    AnimalExtra a;
    a.maturity = 1.0f;
    a.fedByPlayer = 1;
    return a;
}

void ABeatenCagedAnimalNeedsLessFood()
{
    const char* name = "caged animal: in Balanced the beating stress lowers its bar, so a beaten one joins sooner";
    Config config;
    config.preset = Preset_Balanced;
    // 80% full against a 95% bar: hungry without a beating.
    const Verdict calm = DecideCagedAnimal(CagedBeast(2.4f, 0.0f), Fed(), config);
    CHECK(!calm.willJoin);
    CHECK(calm.reasonKey == Reason_TameHungry);
    // A full beating bar on a level-20 animal (resolve 1.4): the bar drops ~36%.
    const Verdict beaten = DecideCagedAnimal(CagedBeast(2.4f, 1.0f), Fed(), config);
    CHECK(beaten.willJoin);
    CHECK(beaten.reasonKey == Reason_TameReady);
    CHECK(beaten.tameNeeded < calm.tameNeeded);
    CHECK(Near(beaten.tameNeeded, calm.tameNeeded * (1.0f - 0.5f / 1.4f)));
}

void TheBeatingNeverReplacesTheFirstMeal()
{
    const char* name = "caged animal: a beaten animal the player never fed still takes the first meal";
    Config config;
    config.preset = Preset_Balanced;
    AnimalExtra unfed = Fed();
    unfed.fedByPlayer = 0;
    const Verdict v = DecideCagedAnimal(CagedBeast(2.4f, 1.0f), unfed, config);
    CHECK(!v.willJoin);
    CHECK(v.reasonKey == Reason_TameNotFed);
}

void ALooseAnimalIsNotTamedByBlows()
{
    const char* name = "loose animal: the same stress does not move the taming bar outside the cage";
    Config config;
    config.preset = Preset_Balanced;
    CoreFacts loose = CagedBeast(2.4f, 1.0f);
    loose.inMyCage = false;
    const Verdict v = DecideAnimal(loose, Fed(), config);
    CHECK(!v.willJoin);
    CHECK(v.reasonKey == Reason_TameHungry);
}

void TheCageRouteUsesIt()
{
    const char* name = "cage route: DecideCage hands a caged animal to the beaten-animal bar";
    Config config;
    config.preset = Preset_Balanced;
    CageExtra extra;
    extra.animal = Fed();
    const Verdict v = DecideCage(CagedBeast(2.4f, 1.0f), extra, config);
    CHECK(v.isAnimal);
    CHECK(v.willJoin);
    config.animals.cageBeatingCounts = false;
    CHECK(!DecideCage(CagedBeast(2.4f, 1.0f), extra, config).willJoin);
}
} // namespace

void RunAnimalBeatingRulesTests()
{
    TheResolveGrowsWithTheLevel();
    TheStressDiscountsTheBar();
    SwitchedOffItDoesNothing();
    ABeatenCagedAnimalNeedsLessFood();
    TheBeatingNeverReplacesTheFirstMeal();
    ALooseAnimalIsNotTamedByBlows();
    TheCageRouteUsesIt();
}
