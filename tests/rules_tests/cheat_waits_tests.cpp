// Tests for src/cheat_waits.cpp: in Cheat every wait the player can meet is one
// game minute (or the configured value when that is shorter); Balanced reads the
// configured hours untouched.

#include "test_support.h"
#include "cheat_waits.h"
#include "config.h"

using namespace totalrecruit;

namespace
{
const float kMinute = 1.0f / 60.0f;

bool Near(float a, float b)
{
    const float d = a - b;
    return d < 0.0001f && d > -0.0001f;
}

Config Preset(int preset)
{
    Config config;
    config.preset = preset;
    return config;
}

void TheCheatWaitIsOneGameMinute()
{
    const char* name = "cheat waits: kCheatWaitHours is one game minute";
    CHECK(Near(kCheatWaitHours, kMinute));
}

void CheatWaitsOneMinuteForEveryWait()
{
    const char* name = "cheat waits: Cheat makes every mapped wait one game minute";
    const Config c = Preset(Preset_Cheat);
    CHECK(Near(CageWaitHours(c), kMinute));
    CHECK(Near(HailFirstHours(c), kMinute));
    CHECK(Near(HailRepeatHours(c), kMinute));
    CHECK(Near(RejectCooldownHours(c), kMinute));
    CHECK(Near(DuelLossCooldownHours(c), kMinute));
    CHECK(Near(BeatingCooldownHours(c), kMinute));
    CHECK(Near(KnockoutCooldownHours(c), kMinute));
}

void BalancedReadsTheConfiguredHours()
{
    const char* name = "cheat waits: Balanced reads every wait from the config";
    Config c = Preset(Preset_Balanced);
    CHECK(Near(CageWaitHours(c), c.cheatHoursCaged));
    CHECK(Near(HailFirstHours(c), c.hailFirstHours));
    CHECK(Near(HailRepeatHours(c), c.hailRepeatHours));
    CHECK(Near(RejectCooldownHours(c), c.submission.rejectCooldownHours));
    CHECK(Near(DuelLossCooldownHours(c), c.duelChallenges.cooldownHours));
    CHECK(Near(BeatingCooldownHours(c), c.submission.beatingCooldownHours));
    CHECK(Near(KnockoutCooldownHours(c), c.submission.knockoutCooldownHours));

    // Not the defaults by accident: other numbers come straight through.
    c.hailFirstHours = 30.0f;
    c.submission.rejectCooldownHours = 5.0f;
    CHECK(Near(HailFirstHours(c), 30.0f));
    CHECK(Near(RejectCooldownHours(c), 5.0f));
}

void TheShippedDefaultsAreHoursLongOutsideCheat()
{
    const char* name = "cheat waits: the shipped defaults Cheat shortens";
    const Config c;
    CHECK(Near(c.hailFirstHours, 24.0f));
    CHECK(Near(c.submission.rejectCooldownHours, 72.0f));
    CHECK(Near(c.duelChallenges.cooldownHours, 24.0f));
    CHECK(Near(c.submission.beatingCooldownHours, 1.0f));
    CHECK(Near(c.submission.knockoutCooldownHours, 1.0f));
}

void CheatNeverWaitsLongerThanConfigured()
{
    const char* name = "cheat waits: a configured wait under a minute (or 0) stays as it is";
    Config c = Preset(Preset_Cheat);
    c.cheatHoursCaged = 0.0f;
    c.submission.beatingCooldownHours = 0.0f;
    c.submission.knockoutCooldownHours = 0.005f;
    CHECK(Near(CageWaitHours(c), 0.0f));
    CHECK(Near(BeatingCooldownHours(c), 0.0f));
    CHECK(Near(KnockoutCooldownHours(c), 0.005f));
    // The shipped cage wait (0.02 h, about 72 s) is trimmed to the minute.
    c.cheatHoursCaged = 0.02f;
    CHECK(Near(CageWaitHours(c), kMinute));
}

void NegativeHoursReadAsNoWait()
{
    const char* name = "cheat waits: a negative configured wait reads as 0 in Cheat";
    CHECK(Near(EffectiveWaitHours(Preset_Cheat, -3.0f), 0.0f));
    CHECK(Near(EffectiveWaitHours(Preset_Balanced, 6.0f), 6.0f));
    CHECK(Near(EffectiveWaitHours(Preset_Cheat, 6.0f), kMinute));
}
} // namespace

void RunCheatWaitsTests()
{
    TheCheatWaitIsOneGameMinute();
    CheatWaitsOneMinuteForEveryWait();
    BalancedReadsTheConfiguredHours();
    TheShippedDefaultsAreHoursLongOutsideCheat();
    CheatNeverWaitsLongerThanConfigured();
    NegativeHoursReadAsNoWait();
}
