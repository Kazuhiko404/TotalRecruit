// Tests for src/leave_homing_rules.cpp: a temporary member who left walks back to
// where they joined -- the re-issue, the arrival, the timeout -- and the saved spot.

#include "test_support.h"
#include "leave_homing_rules.h"

using namespace totalrecruit;

namespace
{
void ArrivingEndsTheWalk()
{
    const char* name = "homing: within the arrival radius arrives, even past the timeout";
    CHECK(StepHoming(10.0f, 1.0f, kHomingArriveMeters) == Homing_Arrived);
    CHECK(StepHoming(10.0f, 1.0f, 0.0f) == Homing_Arrived);
    CHECK(StepHoming(kHomingTimeoutSeconds + 5.0f, 9.0f, 2.0f) == Homing_Arrived);
}

void TheOrderIsReissuedEveryFewSeconds()
{
    const char* name = "homing: far away waits between orders and re-issues every few seconds";
    CHECK(StepHoming(1.0f, 1.0f, 50.0f) == Homing_Wait);
    CHECK(StepHoming(10.0f, kHomingReissueSeconds - 0.1f, 50.0f) == Homing_Wait);
    CHECK(StepHoming(10.0f, kHomingReissueSeconds, 50.0f) == Homing_Reissue);
    CHECK(StepHoming(10.0f, kHomingReissueSeconds, -1.0f) == Homing_Reissue);
}

void TheWalkGivesUpAfterTheTimeout()
{
    const char* name = "homing: past the timeout, not arrived, gives up; an unread distance never arrives";
    CHECK(StepHoming(kHomingTimeoutSeconds, 0.0f, 50.0f) == Homing_GaveUp);
    CHECK(StepHoming(kHomingTimeoutSeconds + 1.0f, kHomingReissueSeconds, -1.0f) == Homing_GaveUp);
    CHECK(StepHoming(kHomingTimeoutSeconds - 1.0f, 0.0f, -1.0f) == Homing_Wait);
    CHECK(HomingActionText(Homing_Wait)[0] != '\0');
    CHECK(HomingActionText(Homing_Reissue)[0] != '\0');
    CHECK(HomingActionText(Homing_Arrived)[0] != '\0');
    CHECK(HomingActionText(Homing_GaveUp)[0] != '\0');
}

void TheJoinSpotRoundTrips()
{
    const char* name = "join spot: \"x,z\" written and read back; junk is no spot";
    CHECK(FormatJoinSpot(-12345, 678) == "-12345,678");
    int x = 0, z = 0;
    CHECK(ParseJoinSpot("-12345,678", &x, &z) && x == -12345 && z == 678);
    CHECK(ParseJoinSpot(FormatJoinSpot(0, -9), &x, &z) && x == 0 && z == -9);
    CHECK(ParseJoinSpot(" 5 , 6 ", &x, &z) && x == 5 && z == 6);
    x = 1;
    z = 2;
    CHECK(!ParseJoinSpot("", &x, &z));
    CHECK(!ParseJoinSpot("5", &x, &z));
    CHECK(!ParseJoinSpot("5,", &x, &z));
    CHECK(!ParseJoinSpot(",5", &x, &z));
    CHECK(!ParseJoinSpot("5,6,7", &x, &z));
    CHECK(!ParseJoinSpot("a,6", &x, &z));
    CHECK(!ParseJoinSpot("5.5,6", &x, &z));
    CHECK(x == 1 && z == 2);
}
} // namespace

void RunLeaveHomingRulesTests()
{
    ArrivingEndsTheWalk();
    TheOrderIsReissuedEveryFewSeconds();
    TheWalkGivesUpAfterTheTimeout();
    TheJoinSpotRoundTrips();
}
