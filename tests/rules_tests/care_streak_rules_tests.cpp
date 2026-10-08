// Tests for src/care_streak_rules.cpp: the Balanced cage's care streak
// (balanced-tuning T3), game days in a row a prisoner was kept unhurt, fed and at
// full health, and what they add to the cared-for bonus.

#include "test_support.h"
#include "care_streak_rules.h"
#include "char_key.h"
#include "config.h"

using namespace totalrecruit;

namespace
{
CareStreak CaredOn(int firstDay, int lastDay)
{
    CareStreak s;
    for (int day = firstDay; day <= lastDay; ++day)
    {
        s = ObserveCare(s, day, true);
        s = ObserveCare(s, day, true);   // several sightings a day add nothing
    }
    return s;
}

Submission Streak(int perDay, int maxPct)
{
    Submission sub;
    sub.caredForStreakPerDay = perDay;
    sub.caredForStreakMax = maxPct;
    return sub;
}

void ShippedStreakIsThreeADayUpToFifteen()
{
    const char* name = "care streak: the shipped values are +3 a day up to +15";
    const Submission sub;
    CHECK(sub.caredForStreakPerDay == 3);
    CHECK(sub.caredForStreakMax == 15);
    CHECK(kCareStreakPerDayMin == 0 && kCareStreakPerDayMax == 10);
    CHECK(kCareStreakMaxMin == 0 && kCareStreakMaxMax == 40);
}

void ANeverSeenPrisonerHasNoStreak()
{
    const char* name = "care streak: no record means no streak";
    const CareStreak none;
    CHECK(CareStreakDays(none, 0) == 0);
    CHECK(CareStreakDays(none, 12) == 0);
}

void TodayCountsFromTomorrow()
{
    const char* name = "care streak: the first cared-for day adds nothing until it is over";
    const CareStreak s = CaredOn(5, 5);
    CHECK(CareStreakDays(s, 5) == 0);
    CHECK(CareStreakDays(s, 6) == 1);
}

void DaysInARowAddUp()
{
    const char* name = "care streak: kept days in a row add up, one per day however many sightings";
    const CareStreak s = CaredOn(5, 8);
    CHECK(CareStreakDays(s, 8) == 3);
    CHECK(CareStreakDays(s, 9) == 4);
    CHECK(CareStreakDays(CaredOn(0, 9), 10) == 10);
}

void ANotCaredSightingResetsTheStreak()
{
    const char* name = "care streak: one sighting hurt, hungry or below full health resets it to 0";
    CareStreak s = CaredOn(5, 7);
    CHECK(CareStreakDays(s, 7) == 2);
    s = ObserveCare(s, 7, false);
    CHECK(CareStreakDays(s, 7) == 0);
    CHECK(CareStreakDays(s, 8) == 0);
    // Cared for again later the same day: the day stays lost.
    s = ObserveCare(s, 7, true);
    CHECK(CareStreakDays(s, 8) == 0);
    // And the chain starts over from the next kept day.
    s = ObserveCare(s, 8, true);
    CHECK(CareStreakDays(s, 8) == 0);
    CHECK(CareStreakDays(s, 9) == 1);
}

void ANewDaySeenNotCaredForStartsAtZero()
{
    const char* name = "care streak: a new day first seen not cared for has no streak";
    CareStreak s = CaredOn(1, 4);
    s = ObserveCare(s, 5, false);
    CHECK(CareStreakDays(s, 5) == 0);
    CHECK(CareStreakDays(s, 6) == 0);
}

void AnUnseenDayBreaksTheChain()
{
    const char* name = "care streak: a game day with no sighting breaks the chain";
    CareStreak s = CaredOn(5, 6);
    CHECK(CareStreakDays(s, 8) == 0);   // day 7 was never seen
    s = ObserveCare(s, 8, true);
    CHECK(CareStreakDays(s, 8) == 0);
    CHECK(CareStreakDays(s, 9) == 1);
}

void AClockThatWentBackStartsAfresh()
{
    const char* name = "care streak: a day before the record (clock went back) starts afresh";
    CareStreak s = CaredOn(10, 14);
    s = ObserveCare(s, 3, true);
    CHECK(s.dayIndex == 3);
    CHECK(CareStreakDays(s, 3) == 0);
    CHECK(CareStreakDays(s, 4) == 1);
    CHECK(CareStreakDays(CaredOn(10, 14), 9) == 0);
}

void TheBonusGrowsAndIsCapped()
{
    const char* name = "care streak: +3 a day, +15 from a five-day streak on";
    const Submission sub;
    CHECK(CareStreakBonus(0, sub) == 0);
    CHECK(CareStreakBonus(-2, sub) == 0);
    CHECK(CareStreakBonus(1, sub) == 3);
    CHECK(CareStreakBonus(4, sub) == 12);
    CHECK(CareStreakBonus(5, sub) == 15);
    CHECK(CareStreakBonus(6, sub) == 15);
    CHECK(CareStreakBonus(100000, sub) == 15);
}

void TheBonusFollowsTheConfig()
{
    const char* name = "care streak: per day and cap come from the config, clamped to their ranges";
    CHECK(CareStreakBonus(3, Streak(4, 10)) == 10);
    CHECK(CareStreakBonus(2, Streak(4, 10)) == 8);
    CHECK(CareStreakBonus(9, Streak(0, 15)) == 0);
    CHECK(CareStreakBonus(9, Streak(3, 0)) == 0);
    CHECK(CareStreakBonus(1, Streak(50, 99)) == kCareStreakPerDayMax);
    CHECK(CareStreakBonus(100, Streak(50, 99)) == kCareStreakMaxMax);
    CHECK(CareStreakBonus(5, Streak(-4, 15)) == 0);
    CHECK(CareStreakBonus(5, Streak(3, -1)) == 0);
}

void TheLedgerKeepsOneRecordPerPrisoner()
{
    const char* name = "care streak ledger: one record per prisoner";
    CareStreakLedger ledger;
    const CharKey a = CharKey::FromId("care-a");
    const CharKey b = CharKey::FromId("care-b");
    for (int day = 2; day <= 5; ++day)
    {
        ledger.Observe(a, day, true);
    }
    ledger.Observe(b, 5, true);
    ledger.Observe(b, 5, false);
    CHECK(ledger.StreakDays(a, 5) == 3);
    CHECK(ledger.StreakDays(a, 6) == 4);
    CHECK(ledger.StreakDays(b, 6) == 0);
    CHECK(ledger.StreakDays(CharKey::FromId("care-none"), 6) == 0);
    CHECK(ledger.Size() == 2);
    ledger.Clear();
    CHECK(ledger.Size() == 0);
    CHECK(ledger.StreakDays(a, 6) == 0);
}

void TheLedgerRoundTripsThroughTheSave()
{
    const char* name = "care streak ledger: the three save rows put the record back";
    CareStreakLedger ledger;
    const CharKey a = CharKey::FromId("care-save");
    for (int day = 0; day <= 3; ++day)
    {
        ledger.Observe(a, day, true);
    }
    CHECK(ledger.SavedDay(a) == 4);      // day 3 + 1
    CHECK(ledger.SavedStreak(a) == 3);
    CHECK(ledger.SavedKept(a) == 1);

    CareStreakLedger loaded;
    loaded.RestoreKept(a, ledger.SavedKept(a));
    loaded.RestoreStreak(a, ledger.SavedStreak(a));
    loaded.RestoreDay(a, ledger.SavedDay(a));
    CHECK(loaded.StreakDays(a, 3) == 3);
    CHECK(loaded.StreakDays(a, 4) == 4);

    // Someone not in the ledger writes zeros.
    const CharKey none = CharKey::FromId("care-unknown");
    CHECK(ledger.SavedDay(none) == 0 && ledger.SavedStreak(none) == 0 && ledger.SavedKept(none) == 0);
}

void AnOldSaveHasNoStreak()
{
    const char* name = "care streak ledger: an old save (no rows, or zeros) reads as no streak";
    CareStreakLedger loaded;
    const CharKey a = CharKey::FromId("care-old");
    loaded.RestoreDay(a, 0);
    loaded.RestoreStreak(a, 0);
    loaded.RestoreKept(a, 0);
    CHECK(loaded.StreakDays(a, 7) == 0);
    // A streak without its day row is no streak either.
    CareStreakLedger partial;
    partial.RestoreStreak(a, 4);
    partial.RestoreKept(a, 1);
    CHECK(partial.StreakDays(a, 7) == 0);
}
} // namespace

void RunCareStreakRulesTests()
{
    ShippedStreakIsThreeADayUpToFifteen();
    ANeverSeenPrisonerHasNoStreak();
    TodayCountsFromTomorrow();
    DaysInARowAddUp();
    ANotCaredSightingResetsTheStreak();
    ANewDaySeenNotCaredForStartsAtZero();
    AnUnseenDayBreaksTheChain();
    AClockThatWentBackStartsAfresh();
    TheBonusGrowsAndIsCapped();
    TheBonusFollowsTheConfig();
    TheLedgerKeepsOneRecordPerPrisoner();
    TheLedgerRoundTripsThroughTheSave();
    AnOldSaveHasNoStreak();
}
