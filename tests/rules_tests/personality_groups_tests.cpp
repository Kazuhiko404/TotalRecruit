// Tests for src/personality_groups.cpp: the four groups the ally tiers read the
// ten temperaments as (REQUISITOS.md, "Grupos de personalidad").

#include "test_support.h"
#include "personality_groups.h"

using namespace totalrecruit;

namespace
{
void TheGoodAreTheHonourableAndTheKind()
{
    const char* name = "groups: Honorable and WarmKind are Good";
    CHECK(GroupOf(Temperament_Honorable) == PersonalityGroup_Good);
    CHECK(GroupOf(Temperament_WarmKind) == PersonalityGroup_Good);
}

void TheNeutralAreOrdinaryPeople()
{
    const char* name = "groups: Normal, Dumb, Fearful and Brave are Neutral";
    CHECK(GroupOf(Temperament_Normal) == PersonalityGroup_Neutral);
    CHECK(GroupOf(Temperament_Dumb) == PersonalityGroup_Neutral);
    CHECK(GroupOf(Temperament_Fearful) == PersonalityGroup_Neutral);
    CHECK(GroupOf(Temperament_Brave) == PersonalityGroup_Neutral);
}

void TheGreedyAreTheTraitorousAndTheSmart()
{
    const char* name = "groups: Traitorous and Smart are Greedy";
    CHECK(GroupOf(Temperament_Traitorous) == PersonalityGroup_Greedy);
    CHECK(GroupOf(Temperament_Smart) == PersonalityGroup_Greedy);
}

void TheBadAreTheCruelAndTheManiacs()
{
    const char* name = "groups: ColdCruel and Maniacal are Bad";
    CHECK(GroupOf(Temperament_ColdCruel) == PersonalityGroup_Bad);
    CHECK(GroupOf(Temperament_Maniacal) == PersonalityGroup_Bad);
}

void EveryTemperamentHasExactlyOneGroupAndUnknownIsNeutral()
{
    const char* name = "groups: every temperament maps, 2 Good + 4 Neutral + 2 Greedy + 2 Bad; unknown is Neutral";
    int counts[4] = { 0, 0, 0, 0 };
    for (int i = 0; i < Temperament_Count; ++i)
    {
        const PersonalityGroup g = GroupOf((Temperament)i);
        CHECK(g >= PersonalityGroup_Good && g <= PersonalityGroup_Bad);
        ++counts[g];
    }
    CHECK(counts[PersonalityGroup_Good] == 2);
    CHECK(counts[PersonalityGroup_Neutral] == 4);
    CHECK(counts[PersonalityGroup_Greedy] == 2);
    CHECK(counts[PersonalityGroup_Bad] == 2);
    CHECK(GroupOf(Temperament_Count) == PersonalityGroup_Neutral);
    CHECK(GroupOf((Temperament)99) == PersonalityGroup_Neutral);
}
} // namespace

void RunPersonalityGroupsTests()
{
    TheGoodAreTheHonourableAndTheKind();
    TheNeutralAreOrdinaryPeople();
    TheGreedyAreTheTraitorousAndTheSmart();
    TheBadAreTheCruelAndTheManiacs();
    EveryTemperamentHasExactlyOneGroupAndUnknownIsNeutral();
}
