#pragma once

// PURE: how a wait reads to the player (priority-one B).
//
// Under an hour a wait reads in whole minutes rounded up ("1 min": Cheat's waits
// are one game minute, and "(1 h)" for them was wrong); from an hour on it reads
// in whole hours rounded up, exactly as before ("4 h", cage_command_rules
// RestHoursShown). 59.x minutes round up to 60, which reads as "1 h".
//
// The text lives in messages.json. A line that shows a wait has its usual key with
// {hours} and a minute twin under "<key>Minutes" with {minutes}
// (WaitMinutesKey); the button suffix is cage.button.restHours /
// cage.button.restMinutes (WaitTexts, defaults below = the built-in fallbacks).
// The readout card already reads short waits in minutes (readout_format
// FormatHoursLeft) and is not touched.
//
// No Kenshi / MyGUI include: tested in tests/rules_tests/wait_format_tests.cpp.

#include <string>

namespace totalrecruit
{
enum WaitUnit
{
    WaitUnit_None = 0,   // no wait left
    WaitUnit_Minutes,
    WaitUnit_Hours
};

struct WaitShown
{
    WaitUnit unit;
    int value;    // minutes or hours, rounded up; 0 for none

    WaitShown() : unit(WaitUnit_None), value(0) {}
};

// The wait as shown: none, minutes (under an hour) or hours.
WaitShown ShowWait(float hoursLeft);

// The {hours} number for the hours line: the hours shown, 0 unless in hours.
int WaitHoursValue(const WaitShown& shown);

// The two templates of a wait suffix ({hours} / {minutes}).
struct WaitTexts
{
    std::string hours;
    std::string minutes;

    WaitTexts() : hours(" ({hours} h)"), minutes(" ({minutes} min)") {}
};

// The suffix for `shown`: the matching template filled, "" for no wait.
std::string FillWait(const WaitTexts& texts, const WaitShown& shown);

// The minute twin of a message key: "<key>Minutes".
std::string WaitMinutesKey(const std::string& hoursKey);

// Every {minutes} in `text` replaced by `minutes`.
std::string ReplaceMinutes(const std::string& text, int minutes);
} // namespace totalrecruit
