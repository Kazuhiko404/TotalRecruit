// Characterization tests for src/route_rules.cpp: the order in which Evaluate's
// routes are tried. The first block pins the order as it shipped; the T2 blocks
// state the corrected order for the cases the audit found.

#include "test_support.h"
#include "route_rules.h"
#include "defeat_rules.h"
#include "config.h"

#include <cstring>

using namespace totalrecruit;

namespace
{
void NothingSpecialIsFreeWorld()
{
    const char* name = "route: an ordinary NPC outside a cage is the free-world route";
    RouteFacts f;
    CHECK(ChooseEntryRoute(f) == Route_FreeWorld);
}

void NothingSpecialInMyCageIsCage()
{
    const char* name = "route: an ordinary prisoner is the cage route";
    RouteFacts f;
    f.inMyCage = true;
    CHECK(ChooseEntryRoute(f) == Route_Cage);
}

void UnconsciousWaits()
{
    const char* name = "route: unconscious comes before rescue, easy mode and surrender";
    RouteFacts f;
    f.unconscious = true;
    f.remembersSaviour = true;
    f.dying = true;
    f.inMyCage = true;
    f.easyHealthy = true;
    f.surrendered = true;
    CHECK(ChooseEntryRoute(f) == Route_Unconscious);
}

void RescueOutranksDyingAndEasy()
{
    const char* name = "route: rescue before dying, easy-healthy and surrender";
    RouteFacts f;
    f.remembersSaviour = true;
    f.dying = true;
    f.inMyCage = true;
    f.easyHealthy = true;
    f.surrendered = true;
    CHECK(ChooseEntryRoute(f) == Route_Rescue);
}

void RescueSwitchedOffSkipsRescueDyingAndEasy()
{
    const char* name = "route: rescueEnabled off removes rescue, dying and easy-healthy";
    RouteFacts f;
    f.rescueEnabled = false;
    f.remembersSaviour = true;
    f.dying = true;
    f.inMyCage = true;
    f.easyHealthy = true;
    CHECK(ChooseEntryRoute(f) == Route_Cage);
}

void DyingBeforeEasyHealthy()
{
    const char* name = "route: dying before easy-healthy";
    RouteFacts f;
    f.dying = true;
    f.inMyCage = true;
    f.easyHealthy = true;
    CHECK(ChooseEntryRoute(f) == Route_Dying);
}

void EasyHealthyNeedsMyCage()
{
    const char* name = "route: easy-healthy only applies inside the player's cage";
    RouteFacts f;
    f.easyHealthy = true;
    CHECK(ChooseEntryRoute(f) == Route_FreeWorld);
    f.inMyCage = true;
    CHECK(ChooseEntryRoute(f) == Route_EasyHealthy);
}

void SurrenderedBeatsTheTail()
{
    const char* name = "route: a yielding NPC is the surrendered route, caged or not";
    RouteFacts f;
    f.surrendered = true;
    CHECK(ChooseEntryRoute(f) == Route_Surrendered);
    f.inMyCage = true;
    CHECK(ChooseEntryRoute(f) == Route_Surrendered);
}

void AnimalsNeverSurrender()
{
    const char* name = "route: animals neither yield in the field nor surrender";
    RouteFacts f;
    f.animal = true;
    f.defeats = 5;
    f.surrendered = true;
    CHECK(ChooseEntryRoute(f) == Route_FreeWorld);
}

void GrudgeRefusesOutsideTheCageOnly()
{
    const char* name = "route: an unforgiving grudge refuses outside the cage, not inside";
    RouteFacts f;
    f.grudge = true;
    f.grudgeUnforgivable = true;
    CHECK(ChooseEntryRoute(f) == Route_GrudgeRefusal);
    f.inMyCage = true;
    CHECK(ChooseEntryRoute(f) == Route_Cage);
}

void ForgivingGrudgeIsNotARefusal()
{
    const char* name = "route: a grudge that can be forgiven is a weight, not a route";
    RouteFacts f;
    f.grudge = true;
    f.grudgeUnforgivable = false;
    CHECK(ChooseEntryRoute(f) == Route_FreeWorld);
}

void FieldYieldNeedsEnoughDefeats()
{
    const char* name = "route: field-yield needs defeatsToBreak defeats";
    RouteFacts f;
    f.defeats = 1;
    CHECK(ChooseEntryRoute(f) == Route_FreeWorld);
    f.defeats = 2;
    CHECK(ChooseEntryRoute(f) == Route_FieldYield);
}

void FieldYieldRespectsTheSwitches()
{
    const char* name = "route: field-yield off when surrender or defeat counting is off";
    RouteFacts f;
    f.defeats = 3;
    f.surrenderEnabled = false;
    CHECK(ChooseEntryRoute(f) == Route_FreeWorld);
    f.surrenderEnabled = true;
    f.countDefeats = false;
    CHECK(ChooseEntryRoute(f) == Route_FreeWorld);
}

// T2a -- the grudge check used to sit below field-yield, so an honourable,
// brave or kind NPC beaten twice outside the cage yielded to the very person
// who killed one of theirs.
void GrudgeOutranksFieldYield()
{
    const char* name = "route: an unforgiving grudge refuses before field-yield";
    RouteFacts f;
    f.defeats = 2;
    f.grudge = true;
    f.grudgeUnforgivable = true;
    CHECK(ChooseEntryRoute(f) == Route_GrudgeRefusal);
}

void GrudgeOutranksFieldYieldEvenWhenUnconscious()
{
    const char* name = "route: the grudge refusal also beats field-yield while out cold";
    RouteFacts f;
    f.defeats = 4;
    f.unconscious = true;
    f.grudge = true;
    f.grudgeUnforgivable = true;
    CHECK(ChooseEntryRoute(f) == Route_GrudgeRefusal);
}

void ForgivingGrudgeStillYieldsInTheField()
{
    const char* name = "route: a grudge that can be forgiven does not block field-yield";
    RouteFacts f;
    f.defeats = 2;
    f.grudge = true;
    f.grudgeUnforgivable = false;
    CHECK(ChooseEntryRoute(f) == Route_FieldYield);
}

void FieldYieldStillBeatsUnconsciousOutsideTheCage()
{
    const char* name = "route: outside the cage field-yield is still tried while unconscious";
    RouteFacts f;
    f.defeats = 2;
    f.unconscious = true;
    CHECK(ChooseEntryRoute(f) == Route_FieldYield);
}

// T2b -- field-yield returned for every NPC with enough defeats, caged or not,
// so a prisoner beaten twice never reached the cage fallbacks (easy-mode wait,
// healthy shortcut, care, cats). In the cage the defeats still count -- they
// feed the chance -- but the route is the cage.
void CagedNpcWithDefeatsKeepsTheCageRoute()
{
    const char* name = "route: a caged NPC with enough defeats takes the cage route";
    RouteFacts f;
    f.defeats = 2;
    f.inMyCage = true;
    CHECK(ChooseEntryRoute(f) == Route_Cage);
    f.defeats = 9;
    CHECK(ChooseEntryRoute(f) == Route_Cage);
}

void CagedNpcWithDefeatsKeepsEasyHealthy()
{
    const char* name = "route: defeats do not remove the easy-healthy shortcut in the cage";
    RouteFacts f;
    f.defeats = 3;
    f.inMyCage = true;
    f.easyHealthy = true;
    CHECK(ChooseEntryRoute(f) == Route_EasyHealthy);
}

void CagedNpcWithDefeatsStillDies()
{
    const char* name = "route: defeats do not hide a dying prisoner or a rescue debt";
    RouteFacts f;
    f.defeats = 3;
    f.inMyCage = true;
    f.dying = true;
    CHECK(ChooseEntryRoute(f) == Route_Dying);
    f.remembersSaviour = true;
    CHECK(ChooseEntryRoute(f) == Route_Rescue);
}

void CagedNpcWithDefeatsStillWaitsWhenUnconscious()
{
    const char* name = "route: a caged NPC is not asked anything while out cold";
    RouteFacts f;
    f.defeats = 3;
    f.inMyCage = true;
    f.unconscious = true;
    CHECK(ChooseEntryRoute(f) == Route_Unconscious);
}

// T2c -- after a reload the session count is gone and only the persisted floor
// remains. With a floor under defeatsToBreak the field-yield route was lost.
void ReloadedNpcKeepsTheFieldYieldRoute()
{
    const char* name = "route: a reloaded, previously beaten NPC still yields in the field";
    Submission sub;   // shipped defaults: persistedFloorCount 1 < defeatsToBreak 2
    RouteFacts f;
    f.defeatsToBreak = sub.defeatsToBreak;
    f.defeats = EffectiveDefeatCount(0, PersistedDefeatFloor(true, sub.persistedFloorCount,
                                                             sub.defeatsToBreak));
    CHECK(ChooseEntryRoute(f) == Route_FieldYield);
}

void RouteNamesAreDistinct()
{
    const char* name = "route: every route has its own log name";
    const Route all[] = { Route_FieldYield, Route_Unconscious, Route_GrudgeRefusal,
                          Route_Rescue, Route_Dying, Route_EasyHealthy,
                          Route_Surrendered, Route_FreeWorld, Route_Cage };
    const int n = sizeof(all) / sizeof(all[0]);
    for (int i = 0; i < n; ++i)
    {
        CHECK(RouteName(all[i])[0] != '?');
        for (int j = i + 1; j < n; ++j)
        {
            CHECK(std::strcmp(RouteName(all[i]), RouteName(all[j])) != 0);
        }
    }
}
} // namespace

void RunRouteRulesTests()
{
    NothingSpecialIsFreeWorld();
    NothingSpecialInMyCageIsCage();
    UnconsciousWaits();
    RescueOutranksDyingAndEasy();
    RescueSwitchedOffSkipsRescueDyingAndEasy();
    DyingBeforeEasyHealthy();
    EasyHealthyNeedsMyCage();
    SurrenderedBeatsTheTail();
    AnimalsNeverSurrender();
    GrudgeRefusesOutsideTheCageOnly();
    ForgivingGrudgeIsNotARefusal();
    FieldYieldNeedsEnoughDefeats();
    FieldYieldRespectsTheSwitches();
    GrudgeOutranksFieldYield();
    GrudgeOutranksFieldYieldEvenWhenUnconscious();
    ForgivingGrudgeStillYieldsInTheField();
    FieldYieldStillBeatsUnconsciousOutsideTheCage();
    CagedNpcWithDefeatsKeepsTheCageRoute();
    CagedNpcWithDefeatsKeepsEasyHealthy();
    CagedNpcWithDefeatsStillDies();
    CagedNpcWithDefeatsStillWaitsWhenUnconscious();
    ReloadedNpcKeepsTheFieldYieldRoute();
    RouteNamesAreDistinct();
}
