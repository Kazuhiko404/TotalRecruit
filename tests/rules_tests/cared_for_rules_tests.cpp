// Tests for src/cared_for_rules.cpp: the Balanced cage's cared-for bonus, a stable
// daily draw between submissionCaredForBonusMin and submissionCaredForBonusMax.

#include "test_support.h"
#include "cared_for_rules.h"
#include "chance_rules.h"
#include "config.h"

using namespace totalrecruit;

namespace
{
Submission Range(int lo, int hi)
{
    Submission sub;
    sub.caredForBonusMin = lo;
    sub.caredForBonusMax = hi;
    return sub;
}

void ShippedRangeIsFiveToTwenty()
{
    const char* name = "cared for: the shipped range is 5..20";
    const Submission sub;
    CHECK(sub.caredForBonusMin == 5);
    CHECK(sub.caredForBonusMax == 20);
}

void EveryDrawIsInsideTheRange()
{
    const char* name = "cared for: every draw lands in [min, max], both ends reachable";
    bool sawLow = false, sawHigh = false;
    for (unsigned int seed = 0; seed < 5000; ++seed)
    {
        const int b = CaredForBonusFromSeed(seed * 2654435761u, 5, 20);
        CHECK(b >= 5 && b <= 20);
        sawLow = sawLow || b == 5;
        sawHigh = sawHigh || b == 20;
    }
    CHECK(sawLow);
    CHECK(sawHigh);
}

void TheSameSeedGivesTheSameValue()
{
    const char* name = "cared for: the same NPC on the same day always draws the same value";
    const CharKey key(42, 7);
    const Submission sub;
    const int first = CaredForBonus(true, true, key, 11, sub);
    for (int i = 0; i < 20; ++i)
    {
        CHECK(CaredForBonus(true, true, key, 11, sub) == first);
    }
    CHECK(first == CaredForBonusFromSeed(DailySeed(key, 11, kCaredForBonusSalt), 5, 20));
}

void ADifferentDayCanDiffer()
{
    const char* name = "cared for: another game day is a fresh draw";
    const CharKey key(42, 7);
    const Submission sub;
    const int day0 = CaredForBonus(true, true, key, 0, sub);
    bool differs = false;
    for (int day = 1; day < 60 && !differs; ++day)
    {
        differs = CaredForBonus(true, true, key, day, sub) != day0;
    }
    CHECK(differs);
}

void ItIsNotTheJoinRoll()
{
    const char* name = "cared for: its own salt, apart from the join and haggle rolls";
    CHECK(kCaredForBonusSalt != kJoinRollSalt);
    CHECK(kCaredForBonusSalt != kHaggleRollSalt);
}

void MaxBelowMinMeansTheMin()
{
    const char* name = "cared for: a max below the min draws the min";
    for (unsigned int seed = 0; seed < 200; ++seed)
    {
        CHECK(CaredForBonusFromSeed(seed * 40503u, 15, 3) == 15);
    }
    CHECK(CaredForBonus(true, true, CharKey(3, 1), 4, Range(15, 3)) == 15);
}

void MinEqualMaxIsExactlyThatValue()
{
    const char* name = "cared for: min == max draws exactly that value";
    for (unsigned int seed = 0; seed < 200; ++seed)
    {
        CHECK(CaredForBonusFromSeed(seed * 40503u, 12, 12) == 12);
    }
    CHECK(CaredForBonusFromSeed(99u, 0, 0) == 0);
}

void NegativeEndsReadAsZero()
{
    const char* name = "cared for: negative ends read as 0, never a penalty";
    for (unsigned int seed = 0; seed < 200; ++seed)
    {
        const int b = CaredForBonusFromSeed(seed * 40503u, -10, -2);
        CHECK(b == 0);
    }
}

void NotCaredForOrUnkeyedGetsNothing()
{
    const char* name = "cared for: no bonus when not cared for, or with no key to seed from";
    const Submission sub;
    CHECK(CaredForBonus(false, true, CharKey(42, 7), 11, sub) == 0);
    CHECK(CaredForBonus(true, false, CharKey(42, 7), 11, sub) == 0);
}
} // namespace

void RunCaredForRulesTests()
{
    ShippedRangeIsFiveToTwenty();
    EveryDrawIsInsideTheRange();
    TheSameSeedGivesTheSameValue();
    ADifferentDayCanDiffer();
    ItIsNotTheJoinRoll();
    MaxBelowMinMeansTheMin();
    MinEqualMaxIsExactlyThatValue();
    NegativeEndsReadAsZero();
    NotCaredForOrUnkeyedGetsNothing();
}
