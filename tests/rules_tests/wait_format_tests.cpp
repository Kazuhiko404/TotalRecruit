// Tests for src/wait_format: how a wait reads to the player (priority-one B). A
// wait under an hour reads in minutes ("1 min"), from an hour on in whole hours
// rounded up, as before ("4 h"); the minute lines are the "<key>Minutes" keys of
// messages.json.

#include "test_support.h"
#include "wait_format.h"
#include "cheat_waits.h"

#include <string>

using namespace totalrecruit;

namespace
{
void AWaitUnderAnHourReadsInMinutes()
{
    const char* name = "wait format: under an hour, whole minutes rounded up";
    WaitShown w = ShowWait(kCheatWaitHours);
    CHECK(w.unit == WaitUnit_Minutes && w.value == 1);
    w = ShowWait(0.5f);
    CHECK(w.unit == WaitUnit_Minutes && w.value == 30);
    w = ShowWait(0.001f);
    CHECK(w.unit == WaitUnit_Minutes && w.value == 1);
}

void AnHourOrMoreReadsInHoursAsBefore()
{
    const char* name = "wait format: an hour or more, whole hours rounded up (as RestHoursShown)";
    WaitShown w = ShowWait(1.0f);
    CHECK(w.unit == WaitUnit_Hours && w.value == 1);
    w = ShowWait(3.2f);
    CHECK(w.unit == WaitUnit_Hours && w.value == 4);
    // 59.9 minutes round up to 60: that is an hour.
    w = ShowWait(0.999f);
    CHECK(w.unit == WaitUnit_Hours && w.value == 1);
}

void NoWaitReadsAsNone()
{
    const char* name = "wait format: no wait left is none";
    CHECK(ShowWait(0.0f).unit == WaitUnit_None);
    CHECK(ShowWait(-1.0f).unit == WaitUnit_None);
    CHECK(ShowWait(0.0f).value == 0);
}

void TheSuffixFillsTheRightTemplate()
{
    const char* name = "wait format: FillWait picks the hours or the minutes template";
    WaitTexts texts;
    CHECK(texts.hours == " ({hours} h)");
    CHECK(texts.minutes == " ({minutes} min)");
    CHECK(FillWait(texts, ShowWait(kCheatWaitHours)) == " (1 min)");
    CHECK(FillWait(texts, ShowWait(3.2f)) == " (4 h)");
    CHECK(FillWait(texts, ShowWait(0.0f)).empty());
    texts.minutes = " [{minutes}m]";
    CHECK(FillWait(texts, ShowWait(0.25f)) == " [15m]");
}

void TheMinuteLineHasItsOwnKey()
{
    const char* name = "wait format: the minute line of a key is <key>Minutes; {minutes} is replaced";
    CHECK(WaitMinutesKey("cage.beatResting") == "cage.beatRestingMinutes");
    CHECK(WaitMinutesKey("duel.animal.lost") == "duel.animal.lostMinutes");
    CHECK(ReplaceMinutes("en {minutes} min, {minutes}", 5) == "en 5 min, 5");
    CHECK(ReplaceMinutes("sin nada", 5) == "sin nada");
    CHECK(WaitHoursValue(ShowWait(3.2f)) == 4);
    CHECK(WaitHoursValue(ShowWait(0.25f)) == 0);
}
} // namespace

void RunWaitFormatTests()
{
    AWaitUnderAnHourReadsInMinutes();
    AnHourOrMoreReadsInHoursAsBefore();
    NoWaitReadsAsNone();
    TheSuffixFillsTheRightTemplate();
    TheMinuteLineHasItsOwnKey();
}
