// Characterization tests for src/route_rules.cpp: the order in which Evaluate's
// routes are tried. The first block pins the order as it shipped; the T2 blocks
// state the corrected order for the cases the audit found.

#include "test_support.h"
#include "route_rules.h"
#include "defeat_ledger.h"
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
    const char* name = "route: unconscious comes before rescue and surrender";
    RouteFacts f;
    f.unconscious = true;
    f.remembersSaviour = true;
    f.dying = true;
    f.inMyCage = true;
    f.surrendered = true;
    CHECK(ChooseEntryRoute(f) == Route_Unconscious);
}

void RescueOutranksDying()
{
    const char* name = "route: rescue before dying and surrender";
    RouteFacts f;
    f.remembersSaviour = true;
    f.dying = true;
    f.inMyCage = true;
    f.surrendered = true;
    CHECK(ChooseEntryRoute(f) == Route_Rescue);
}

void RescueSwitchedOffSkipsRescueAndDying()
{
    const char* name = "route: rescueEnabled off removes rescue and dying";
    RouteFacts f;
    f.rescueEnabled = false;
    f.remembersSaviour = true;
    f.dying = true;
    f.inMyCage = true;
    CHECK(ChooseEntryRoute(f) == Route_Cage);
}

void ACaredForPrisonerTakesTheCageRoute()
{
    const char* name = "route: a healthy, fed prisoner in your cage takes the cage route (no shortcut, P4)";
    RouteFacts f;
    f.inMyCage = true;
    CHECK(ChooseEntryRoute(f) == Route_Cage);
    f.dying = true;
    CHECK(ChooseEntryRoute(f) == Route_Dying);
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

// cage-animals-submission T2: an animal is tamed with the player's food and
// nothing else, so the rescue debt may not answer for it.
void AnimalsSkipRescue()
{
    const char* name = "route: an animal never takes the rescue route";
    RouteFacts f;
    f.animal = true;
    f.remembersSaviour = true;
    CHECK(ChooseEntryRoute(f) == Route_FreeWorld);
    f.inMyCage = true;
    CHECK(ChooseEntryRoute(f) == Route_Cage);

    // A person in the same spot still does.
    RouteFacts person;
    person.inMyCage = true;
    person.remembersSaviour = true;
    CHECK(ChooseEntryRoute(person) == Route_Rescue);
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
// so a prisoner beaten twice never reached the cage fallbacks (care, cats). In the cage the defeats still count -- they
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

// T2c, then recruit-gating T1 -- the count a beaten NPC carries is the one their
// own save state holds: serialised from the ledger, cleared at load begin and put
// back by their load hook. A reload keeps the field-yield route it unlocked.
void ReloadedNpcKeepsTheFieldYieldRoute()
{
    const char* name = "route: a reloaded NPC keeps the defeat count their save holds";
    const Submission sub;
    const CharKey gwyn = CharKey::FromId("6f1c2a9e-guard-gwynnever");
    DefeatLedger ledger;
    for (int i = 0; i < sub.defeatsToBreak; ++i)
    {
        ledger.Add(gwyn);
    }
    const int saved = ledger.Get(gwyn);   // the serialise hook writes this

    ledger.Clear();                       // load begin
    ledger.Set(gwyn, saved);              // the load hook reads it back

    RouteFacts f;
    f.defeatsToBreak = sub.defeatsToBreak;
    f.defeats = ledger.Get(gwyn);
    CHECK(ChooseEntryRoute(f) == Route_FieldYield);
}

// The Gwynnever bug: the engine wrote its own "lost to me" tag and the mod read it
// as two defeats. A character whose save holds no count has none.
void NpcWithNoSavedCountHasNoDefeats()
{
    const char* name = "route: an NPC whose save holds no count is not field-yield";
    const Submission sub;
    const CharKey guard = CharKey::FromId("guard-never-beaten");
    DefeatLedger ledger;
    ledger.Clear();                       // load begin; her load hook found no key

    RouteFacts f;
    f.defeatsToBreak = sub.defeatsToBreak;
    f.defeats = ledger.Get(guard);
    CHECK(f.defeats == 0);
    CHECK(ChooseEntryRoute(f) != Route_FieldYield);
}

void RouteNamesAreDistinct()
{
    const char* name = "route: every route has its own log name";
    const Route all[] = { Route_FieldYield, Route_Unconscious, Route_GrudgeRefusal,
                          Route_Rescue, Route_Dying,
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
    RescueOutranksDying();
    RescueSwitchedOffSkipsRescueAndDying();
    ACaredForPrisonerTakesTheCageRoute();
    SurrenderedBeatsTheTail();
    AnimalsNeverSurrender();
    AnimalsSkipRescue();
    GrudgeRefusesOutsideTheCageOnly();
    ForgivingGrudgeIsNotARefusal();
    FieldYieldNeedsEnoughDefeats();
    FieldYieldRespectsTheSwitches();
    GrudgeOutranksFieldYield();
    GrudgeOutranksFieldYieldEvenWhenUnconscious();
    ForgivingGrudgeStillYieldsInTheField();
    FieldYieldStillBeatsUnconsciousOutsideTheCage();
    CagedNpcWithDefeatsKeepsTheCageRoute();
    CagedNpcWithDefeatsStillDies();
    CagedNpcWithDefeatsStillWaitsWhenUnconscious();
    ReloadedNpcKeepsTheFieldYieldRoute();
    NpcWithNoSavedCountHasNoDefeats();
    RouteNamesAreDistinct();
}
