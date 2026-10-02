// Tests for src/mercy_rules.cpp: REQUISITOS R3.6 (who can buy their life) and
// R3.7 (what each answer to a plea does).

#include "test_support.h"
#include "mercy_rules.h"

using namespace totalrecruit;

namespace
{
const Temperament kAll[] = {
    Temperament_Normal, Temperament_Fearful, Temperament_Traitorous,
    Temperament_Dumb, Temperament_Smart, Temperament_Brave,
    Temperament_Honorable, Temperament_Maniacal, Temperament_ColdCruel,
    Temperament_WarmKind
};
const int kAllCount = sizeof(kAll) / sizeof(kAll[0]);

void R36WhoCanBuyTheirLife()
{
    const char* name = "R3.6: coward, turncoat, dullard, cruel and maniacal can buy; honour and bravery cannot";
    CHECK(CanBuyTheirLife(Temperament_Fearful));
    CHECK(CanBuyTheirLife(Temperament_Traitorous));
    CHECK(CanBuyTheirLife(Temperament_Dumb));
    CHECK(CanBuyTheirLife(Temperament_ColdCruel));
    CHECK(CanBuyTheirLife(Temperament_Maniacal));
    CHECK(!CanBuyTheirLife(Temperament_Honorable));
    CHECK(!CanBuyTheirLife(Temperament_Brave));
    CHECK(!CanBuyTheirLife(Temperament_Normal));
}

// T2d -- an accepted plea ran the normal Evaluate and could answer with a cats
// price or "his honour forbids bribes". Accepting means they join, for anyone.
void AcceptedPleaJoinsEveryone()
{
    const char* name = "R3.7: accepting a plea recruits, whatever the temperament";
    for (int i = 0; i < kAllCount; ++i)
    {
        CHECK(ResolvePlea(PleaAnswer_Accept, kAll[i]) == PleaOutcome_Join);
    }
}

void IgnoreCostsTheBuyerTheirPurse()
{
    const char* name = "R3.7: ignoring a plea lets them go; those who can buy their life leave a tribute";
    CHECK(ResolvePlea(PleaAnswer_Ignore, Temperament_Fearful) == PleaOutcome_LetGoWithTribute);
    CHECK(ResolvePlea(PleaAnswer_Ignore, Temperament_Maniacal) == PleaOutcome_LetGoWithTribute);
    CHECK(ResolvePlea(PleaAnswer_Ignore, Temperament_Honorable) == PleaOutcome_LetGo);
    CHECK(ResolvePlea(PleaAnswer_Ignore, Temperament_Brave) == PleaOutcome_LetGo);
}

void RejectKeepsTheFight()
{
    const char* name = "R3.7: rejecting a plea leaves the fight going";
    for (int i = 0; i < kAllCount; ++i)
    {
        CHECK(ResolvePlea(PleaAnswer_Reject, kAll[i]) == PleaOutcome_FightOn);
    }
}
} // namespace

void RunMercyRulesTests()
{
    R36WhoCanBuyTheirLife();
    AcceptedPleaJoinsEveryone();
    IgnoreCostsTheBuyerTheirPurse();
    RejectKeepsTheFight();
}
