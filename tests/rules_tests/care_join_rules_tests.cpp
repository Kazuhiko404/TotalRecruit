// Tests for src/care_join_rules.cpp: when care alone makes a caged honourable or
// warm/kind prisoner join after a failed roll (balanced-tuning T6): only once the
// care streak reaches submissionCareJoinStreakDays.

#include "test_support.h"
#include "care_join_rules.h"
#include "config.h"

using namespace totalrecruit;

namespace
{
Submission JoinAfter(int days)
{
    Submission sub;
    sub.careJoinStreakDays = days;
    return sub;
}

void ShippedThresholdIsFourDays()
{
    const char* name = "care join: the shipped streak is 4 days, configurable 0..30";
    const Submission sub;
    CHECK(sub.careJoinStreakDays == 4);
    CHECK(kCareJoinStreakDaysMin == 0 && kCareJoinStreakDaysMax == 30);
    CHECK(CareJoinStreakDaysNeeded(sub) == 4);
}

void BelowTheStreakDaysAreMissing()
{
    const char* name = "care join: below the streak it says how many cared-for days are missing";
    const Submission sub;
    CHECK(CareJoinDaysMissing(0, sub) == 4);
    CHECK(CareJoinDaysMissing(1, sub) == 3);
    CHECK(CareJoinDaysMissing(3, sub) == 1);
    CHECK(CareJoinDaysMissing(-5, sub) == 4);
}

void FromTheStreakCareJoins()
{
    const char* name = "care join: from the streak on, nothing is missing";
    const Submission sub;
    CHECK(CareJoinDaysMissing(4, sub) == 0);
    CHECK(CareJoinDaysMissing(5, sub) == 0);
    CHECK(CareJoinDaysMissing(400, sub) == 0);
}

void ZeroIsTheOldBehaviour()
{
    const char* name = "care join: 0 days joins as soon as they are cared for (the old rule)";
    const Submission sub = JoinAfter(0);
    CHECK(CareJoinStreakDaysNeeded(sub) == 0);
    CHECK(CareJoinDaysMissing(0, sub) == 0);
}

void TheConfigIsClamped()
{
    const char* name = "care join: the threshold is clamped to 0..30";
    CHECK(CareJoinStreakDaysNeeded(JoinAfter(-3)) == 0);
    CHECK(CareJoinDaysMissing(0, JoinAfter(-3)) == 0);
    CHECK(CareJoinStreakDaysNeeded(JoinAfter(99)) == 30);
    CHECK(CareJoinDaysMissing(0, JoinAfter(99)) == 30);
    CHECK(CareJoinDaysMissing(2, JoinAfter(7)) == 5);
}
} // namespace

void RunCareJoinRulesTests()
{
    ShippedThresholdIsFourDays();
    BelowTheStreakDaysAreMissing();
    FromTheStreakCareJoins();
    ZeroIsTheOldBehaviour();
    TheConfigIsClamped();
}
