// Tests for src/defeat_rules.cpp: what a reload leaves of the defeat count.

#include "test_support.h"
#include "defeat_rules.h"
#include "config.h"

using namespace totalrecruit;

namespace
{
void UnmarkedCharacterHasNoFloor()
{
    const char* name = "floor: no memory tag, no floor";
    CHECK(PersistedDefeatFloor(false, 1, 2) == 0);
    CHECK(PersistedDefeatFloor(false, 5, 2) == 0);
}

// T2c -- the shipped defaults were persistedFloorCount 1 under defeatsToBreak 2.
void MarkedFloorReachesDefeatsToBreak()
{
    const char* name = "floor: a marked character keeps at least defeatsToBreak after a reload";
    Submission sub;
    CHECK(PersistedDefeatFloor(true, sub.persistedFloorCount, sub.defeatsToBreak)
          >= sub.defeatsToBreak);
    CHECK(PersistedDefeatFloor(true, 1, 2) == 2);
    CHECK(PersistedDefeatFloor(true, 0, 3) == 3);
}

void ConfiguredFloorAboveTheThresholdIsHonoured()
{
    const char* name = "floor: a configured floor above defeatsToBreak still applies";
    CHECK(PersistedDefeatFloor(true, 4, 2) == 4);
}

void EffectiveCountNeverBelowTheFloor()
{
    const char* name = "count: the session count wins when higher, the floor when the session was lost";
    CHECK(EffectiveDefeatCount(0, 2) == 2);
    CHECK(EffectiveDefeatCount(1, 2) == 2);
    CHECK(EffectiveDefeatCount(5, 2) == 5);
    CHECK(EffectiveDefeatCount(0, 0) == 0);
}
} // namespace

void RunDefeatRulesTests()
{
    UnmarkedCharacterHasNoFloor();
    MarkedFloorReachesDefeatsToBreak();
    ConfiguredFloorAboveTheThresholdIsHonoured();
    EffectiveCountNeverBelowTheFloor();
}
