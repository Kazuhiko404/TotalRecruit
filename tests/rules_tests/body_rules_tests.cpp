// Characterization tests for src/body_rules.cpp and src/relation_rules.cpp: the
// small readings Evaluate computed inline between the engine reads and the first
// route. The expectations are the old inline expressions, case by case.

#include "test_support.h"
#include "body_rules.h"
#include "relation_rules.h"

#include <cstring>

using namespace totalrecruit;

namespace
{
void AwakeIsWoundedByHealthOnly()
{
    const char* name = "wounded: awake, health at or under the threshold";
    CHECK(IsWounded(false, false, 0.5f, 0.5f));
    CHECK(IsWounded(false, false, 0.2f, 0.5f));
    CHECK(!IsWounded(false, false, 0.51f, 0.5f));
    // Dying only matters while out cold.
    CHECK(!IsWounded(false, true, 1.0f, 0.5f));
}

void OutColdIsWoundedOnlyWhenDying()
{
    const char* name = "wounded: out cold, health reads 0 and must not count; only dying does";
    CHECK(!IsWounded(true, false, 0.0f, 0.5f));
    CHECK(IsWounded(true, true, 0.0f, 0.5f));
    CHECK(IsWounded(true, true, 1.0f, 0.5f));
}

void EngineHungerAlwaysStarves()
{
    const char* name = "starving: the engine's own answer decides by default";
    CHECK(IsStarving(true, 3.0f, 0.0f));
    CHECK(IsStarving(true, 3.0f, 0.4f));
    CHECK(!IsStarving(false, 0.0f, 0.0f));
}

void ThresholdIsAFractionOfAFullStomach()
{
    const char* name = "starving: an opt-in threshold, as a 0..1 fraction of the 0..3 reading";
    // 0.4 of a full stomach is raw 1.2.
    CHECK(IsStarving(false, 1.2f, 0.4f));
    CHECK(IsStarving(false, 0.6f, 0.4f));
    CHECK(!IsStarving(false, 1.3f, 0.4f));
    CHECK(!IsStarving(false, 3.0f, 0.4f));
    // Off.
    CHECK(!IsStarving(false, 0.0f, 0.0f));
}

void FullyCaredForNeedsEverythingRight()
{
    const char* name = "cared for: fed, unwounded and at full health";
    CHECK(IsFullyCaredFor(false, false, 1.0f));
    CHECK(IsFullyCaredFor(false, false, 1.7f));
    CHECK(!IsFullyCaredFor(true, false, 1.0f));
    CHECK(!IsFullyCaredFor(false, true, 1.0f));
    CHECK(!IsFullyCaredFor(false, false, 0.99f));
}

void RestoredIsBloodWhenOutColdAndHealthOtherwise()
{
    const char* name = "restored: blood while out cold, capped health while awake";
    CHECK(RestoredFraction(true, 0.4f, 0.0f) == 0.4f);
    CHECK(RestoredFraction(false, 0.4f, 0.7f) == 0.7f);
    CHECK(RestoredFraction(false, 0.4f, 1.6f) == 1.0f);
    CHECK(RestoredFraction(false, 0.4f, 0.0f) == 0.0f);
}

void BandsRunFromAllyToEnemy()
{
    const char* name = "relation band: the seven bands and their edges";
    CHECK(std::strcmp(RelationBand(100.0f), "aliado") == 0);
    CHECK(std::strcmp(RelationBand(75.0f), "aliado") == 0);
    CHECK(std::strcmp(RelationBand(74.9f), "amistoso") == 0);
    CHECK(std::strcmp(RelationBand(25.0f), "amistoso") == 0);
    CHECK(std::strcmp(RelationBand(24.9f), "cordial") == 0);
    CHECK(std::strcmp(RelationBand(5.0f), "cordial") == 0);
    CHECK(std::strcmp(RelationBand(4.9f), "neutral") == 0);
    CHECK(std::strcmp(RelationBand(-5.0f), "neutral") == 0);
    CHECK(std::strcmp(RelationBand(-5.1f), "receloso") == 0);
    CHECK(std::strcmp(RelationBand(-15.0f), "receloso") == 0);
    CHECK(std::strcmp(RelationBand(-15.1f), "hostil") == 0);
    CHECK(std::strcmp(RelationBand(-30.0f), "hostil") == 0);
    CHECK(std::strcmp(RelationBand(-30.1f), "enemigo") == 0);
    CHECK(std::strcmp(RelationBand(-100.0f), "enemigo") == 0);
}
} // namespace

void RunBodyRulesTests()
{
    AwakeIsWoundedByHealthOnly();
    OutColdIsWoundedOnlyWhenDying();
    EngineHungerAlwaysStarves();
    ThresholdIsAFractionOfAFullStomach();
    FullyCaredForNeedsEverythingRight();
    RestoredIsBloodWhenOutColdAndHealthOtherwise();
    BandsRunFromAllyToEnemy();
}
