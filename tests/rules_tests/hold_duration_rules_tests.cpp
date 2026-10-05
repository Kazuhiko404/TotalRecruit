// Tests for src/hold_duration_rules.cpp: how the card writes how long a prisoner has
// been held, on the game clock.

#include "test_support.h"
#include "hold_duration_rules.h"

using namespace totalrecruit;

namespace
{
void UnderAnHourIsMinutes()
{
    const char* name = "hold duration: under an hour reads in whole minutes, rounded down";
    CHECK(FormatHoldDuration(0.0f) == "0 min");
    CHECK(FormatHoldDuration(0.01f) == "0 min");
    CHECK(FormatHoldDuration(1.0f / 60.0f) == "1 min");
    CHECK(FormatHoldDuration(0.5f) == "30 min");
    CHECK(FormatHoldDuration(0.999f) == "59 min");
}

void UnderADayIsHoursAndMinutes()
{
    const char* name = "hold duration: under a day reads hours and minutes, no \"0 min\"";
    CHECK(FormatHoldDuration(1.0f) == "1 h");
    CHECK(FormatHoldDuration(2.0f) == "2 h");
    CHECK(FormatHoldDuration(2.25f) == "2 h 15 min");
    CHECK(FormatHoldDuration(2.0f + 59.0f / 60.0f) == "2 h 59 min");
    CHECK(FormatHoldDuration(23.99f) == "23 h 59 min");
}

void ADayOrMoreIsDaysAndHours()
{
    const char* name = "hold duration: a day or more reads days and hours, no \"0 h\"";
    CHECK(FormatHoldDuration(24.0f) == "1 d");
    CHECK(FormatHoldDuration(24.5f) == "1 d");
    CHECK(FormatHoldDuration(27.0f) == "1 d 3 h");
    CHECK(FormatHoldDuration(51.9f) == "2 d 3 h");
    CHECK(FormatHoldDuration(240.0f) == "10 d");
}

void ClockNoiseNeverLosesAMinute()
{
    const char* name = "hold duration: a hair under a whole minute is that minute; nothing negative";
    // 2 h 15 min after float arithmetic.
    CHECK(FormatHoldDuration(2.2499999f) == "2 h 15 min");
    CHECK(FormatHoldDuration(0.99999994f) == "1 h");
    CHECK(FormatHoldDuration(-3.0f) == "0 min");
}
} // namespace

void RunHoldDurationRulesTests()
{
    UnderAnHourIsMinutes();
    UnderADayIsHoursAndMinutes();
    ADayOrMoreIsDaysAndHours();
    ClockNoiseNeverLosesAMinute();
}
