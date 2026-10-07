// Tests for src/cooldown_minutes: the reject and lost-duel cooldowns are kept to
// the game minute (priority-one B), so Cheat's one-minute wait ends a minute
// later instead of at the next whole game hour, while a wait of an hour or more
// still ends on the whole game hour it always did (Balanced unchanged). Old saves
// held whole hours: they read back as that hour's first minute.

#include "test_support.h"
#include "cooldown_minutes.h"
#include "cheat_waits.h"

#include <climits>
#include <cmath>

using namespace totalrecruit;

namespace
{
void CheatsMinuteEndsAMinuteLater()
{
    const char* name = "cooldown minutes: Cheat's one-minute wait ends one game minute later, not at the next hour";
    // 10:00 + 1 min = minute 601.
    CHECK(CooldownUntilMinute(10.0, kCheatWaitHours) == 601);
    // 10:15 + 1 min = minute 616 (the whole-hour store said 11:00, 45 minutes more).
    CHECK(CooldownUntilMinute(10.25, kCheatWaitHours) == 616);
    double left = 0.0;
    CHECK(CooldownActiveAt(616, 10.25, &left));
    CHECK(left > 0.0 && left <= kCheatWaitHours + 1e-6);
    CHECK(!CooldownActiveAt(616, 10.25 + 1.0 / 60.0 + 1e-6, &left));
    CHECK(left == 0.0);
}

void AnHourOrMoreEndsOnTheWholeHourAsBefore()
{
    const char* name = "cooldown minutes: a wait of an hour or more ends on the whole game hour it used to (Balanced unchanged)";
    CHECK(CooldownUntilMinute(100.0, 72.0f) == 172 * 60);
    CHECK(CooldownUntilMinute(100.2, 72.0f) == 173 * 60);
    CHECK(CooldownUntilMinute(100.4, 24.0f) == 125 * 60);
    CHECK(CooldownUntilMinute(10.0, 1.0f) == 11 * 60);
    CHECK(CooldownUntilMinute(10.5, 1.0f) == 12 * 60);
}

void AShortConfiguredWaitIsKeptToTheMinute()
{
    const char* name = "cooldown minutes: a configured wait under an hour is rounded up to the minute";
    CHECK(CooldownUntilMinute(10.0, 0.5f) == 630);
    CHECK(CooldownUntilMinute(10.01, 0.5f) == 631);
}

void NoWaitIsNoCooldown()
{
    const char* name = "cooldown minutes: 0 hours or less is no cooldown";
    CHECK(CooldownUntilMinute(100.0, 0.0f) == 0);
    CHECK(CooldownUntilMinute(100.0, -5.0f) == 0);
    CHECK(!CooldownActiveAt(0, 0.0, NULL));
    CHECK(!CooldownActiveAt(-3, 0.0, NULL));
    // Minute 0 of the game would read as none: the first minute is the floor.
    CHECK(CooldownUntilMinute(0.0, kCheatWaitHours) == 1);
}

void TheHoursLeftAreTheMinutesLeft()
{
    const char* name = "cooldown minutes: the hours left come from the end minute";
    double left = -1.0;
    CHECK(CooldownActiveAt(172 * 60, 100.0, &left));
    CHECK(std::fabs(left - 72.0) < 1e-6);
    CHECK(!CooldownActiveAt(172 * 60, 172.0, &left));
    CHECK(left == 0.0);
    CHECK(!CooldownActiveAt(172 * 60, 500.0, NULL));
}

void OldSavesHeldWholeHours()
{
    const char* name = "cooldown minutes: a legacy whole-hour save value reads as that hour, and is written back as the hour it rounds up to";
    CHECK(UntilMinuteFromSavedHour(172) == 172 * 60);
    CHECK(UntilMinuteFromSavedHour(0) == 0);
    CHECK(UntilMinuteFromSavedHour(-4) == 0);
    CHECK(UntilMinuteFromSavedHour(INT_MAX) == INT_MAX);   // clamped, never wraps
    CHECK(SavedHourFromUntilMinute(172 * 60) == 172);
    CHECK(SavedHourFromUntilMinute(601) == 11);
    CHECK(SavedHourFromUntilMinute(0) == 0);
    CHECK(SavedHourFromUntilMinute(-1) == 0);
}
} // namespace

void RunCooldownMinutesTests()
{
    CheatsMinuteEndsAMinuteLater();
    AnHourOrMoreEndsOnTheWholeHourAsBefore();
    AShortConfiguredWaitIsKeptToTheMinute();
    NoWaitIsNoCooldown();
    TheHoursLeftAreTheMinutesLeft();
    OldSavesHeldWholeHours();
}
