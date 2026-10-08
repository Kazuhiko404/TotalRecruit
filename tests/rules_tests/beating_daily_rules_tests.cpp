// Tests for src/beating_daily_rules: the Balanced cage's daily limits on the
// beating stress (balanced-tuning T2). The n-th blow or knockout of one game day
// weighs beatingDiminishFactor^(n-1) of a fresh one, and one prisoner never gains
// more than beatingDailyStressCap of the bar in one game day. The per-prisoner
// day record and its save rows live in BeatingDayLedger.

#include "test_support.h"
#include "beating_daily_rules.h"
#include "beating_stress_rules.h"

#include <cmath>

using namespace totalrecruit;

namespace
{
bool Near(float a, float b)
{
    return std::fabs(a - b) < 0.0001f;
}

BeatingDay Day(int dayIndex, int blows, float gained)
{
    BeatingDay day;
    day.dayIndex = dayIndex;
    day.blows = blows;
    day.gained = gained;
    return day;
}

// The mechanics tests pin the original T2 numbers (0.7, 0.35) so they keep
// checking order and cap when the shipped defaults are retuned.
Submission MechanicsSub()
{
    Submission sub;
    sub.beatingDiminishFactor = 0.7f;
    sub.beatingDailyStressCap = 0.35f;
    return sub;
}

void TheDefaultsAreTheAgreedOnes()
{
    const char* name = "beating daily: defaults diminish 0.9 (0.3..1.0), daily cap 0.6 (0.1..1.0)";
    const Submission sub;
    CHECK(sub.beatingDiminishFactor == 0.9f);
    CHECK(sub.beatingDailyStressCap == 0.6f);
    CHECK(kBeatingDiminishFactorMin == 0.3f);
    CHECK(kBeatingDiminishFactorMax == 1.0f);
    CHECK(kBeatingDailyStressCapMin == 0.1f);
    CHECK(kBeatingDailyStressCapMax == 1.0f);
}

void AStoredDayFromAnotherDayIsAFreshDay()
{
    const char* name = "beating daily: a record from another day (or none) reads as a fresh day";
    const BeatingDay none;
    CHECK(none.dayIndex == -1 && none.blows == 0 && none.gained == 0.0f);
    const BeatingDay sameDay = BeatingDayFor(Day(5, 3, 0.2f), 5);
    CHECK(sameDay.dayIndex == 5 && sameDay.blows == 3 && Near(sameDay.gained, 0.2f));
    const BeatingDay nextDay = BeatingDayFor(Day(5, 3, 0.2f), 6);
    CHECK(nextDay.dayIndex == 6 && nextDay.blows == 0 && nextDay.gained == 0.0f);
    // A clock that went back (an earlier save) starts that day afresh too.
    const BeatingDay earlier = BeatingDayFor(Day(5, 3, 0.2f), 4);
    CHECK(earlier.dayIndex == 4 && earlier.blows == 0 && earlier.gained == 0.0f);
    const BeatingDay fromNothing = BeatingDayFor(none, 0);
    CHECK(fromNothing.dayIndex == 0 && fromNothing.blows == 0);
}

void TheMultiplierShrinksPerBlowOfTheDay()
{
    const char* name = "beating daily: the n-th blow of the day weighs factor^(n-1)";
    CHECK(Near(BeatingDiminishMultiplier(0, 0.7f), 1.0f));
    CHECK(Near(BeatingDiminishMultiplier(1, 0.7f), 0.7f));
    CHECK(Near(BeatingDiminishMultiplier(2, 0.7f), 0.49f));
    CHECK(Near(BeatingDiminishMultiplier(5, 0.7f), 0.16807f));
    // 1.0 switches the diminishing off; the factor is clamped to 0.3..1.0.
    CHECK(Near(BeatingDiminishMultiplier(9, 1.0f), 1.0f));
    CHECK(Near(BeatingDiminishMultiplier(1, 0.1f), 0.3f));
    CHECK(Near(BeatingDiminishMultiplier(2, 2.0f), 1.0f));
    // A negative count reads as the first blow.
    CHECK(Near(BeatingDiminishMultiplier(-3, 0.7f), 1.0f));
}

void TheCapLeavesOnlyWhatIsLeftOfTheDay()
{
    const char* name = "beating daily: the gain never takes the day past the cap";
    CHECK(Near(CapBeatingGain(0.15f, 0.0f, 0.35f), 0.15f));
    CHECK(Near(CapBeatingGain(0.15f, 0.30f, 0.35f), 0.05f));
    CHECK(Near(CapBeatingGain(0.15f, 0.35f, 0.35f), 0.0f));
    CHECK(Near(CapBeatingGain(0.15f, 0.50f, 0.35f), 0.0f));   // a lower cap after a reload
    CHECK(Near(CapBeatingGain(-0.1f, 0.0f, 0.35f), 0.0f));
    // The cap is clamped to 0.1..1.0.
    CHECK(Near(CapBeatingGain(0.25f, 0.0f, 0.0f), 0.1f));
    CHECK(Near(CapBeatingGain(0.25f, 0.9f, 5.0f), 0.1f));
}

void TheDiminishedGainKeepsThePerBlowClamp()
{
    const char* name = "beating daily: diminish before the per-blow clamp, the cap after it";
    const Submission sub = MechanicsSub();
    // Even levels, a normal prisoner: 0.08, then 0.056, then 0.0392.
    CHECK(Near(BeatingStressGainScaled(BeatingBlow_Hit, 40, 40, Temperament_Normal, 1.0f, sub), 0.08f));
    CHECK(Near(DailyBeatingGain(BeatingBlow_Hit, 40, 40, Temperament_Normal, Day(3, 0, 0.0f), sub), 0.08f));
    CHECK(Near(DailyBeatingGain(BeatingBlow_Hit, 40, 40, Temperament_Normal, Day(3, 1, 0.08f), sub), 0.056f));
    CHECK(Near(DailyBeatingGain(BeatingBlow_Hit, 40, 40, Temperament_Normal, Day(3, 2, 0.136f), sub), 0.0392f));
    // Far into the day the per-blow floor (0.02) still holds while the cap allows it.
    CHECK(Near(DailyBeatingGain(BeatingBlow_Hit, 40, 40, Temperament_Normal, Day(3, 9, 0.30f), sub), 0.02f));
    // ... and the cap is the only thing that goes below it.
    CHECK(Near(DailyBeatingGain(BeatingBlow_Hit, 40, 40, Temperament_Normal, Day(3, 9, 0.34f), sub), 0.01f));
    CHECK(Near(DailyBeatingGain(BeatingBlow_Knockout, 40, 40, Temperament_Normal, Day(3, 9, 0.35f), sub), 0.0f));
    // The max still bounds a fresh knockout on an outclassed coward (0.375 -> 0.25).
    CHECK(Near(DailyBeatingGain(BeatingBlow_Knockout, 40, 20, Temperament_Fearful, Day(3, 0, 0.0f), sub), 0.25f));
    // The second one: 0.375 * 0.7 = 0.2625 -> 0.25, then the cap leaves 0.10.
    CHECK(Near(DailyBeatingGain(BeatingBlow_Knockout, 40, 20, Temperament_Fearful, Day(3, 1, 0.25f), sub), 0.10f));
    // A zero base still adds nothing at all.
    Submission off;
    off.beatStressBase = 0.0f;
    CHECK(DailyBeatingGain(BeatingBlow_Hit, 40, 40, Temperament_Normal, Day(3, 0, 0.0f), off) == 0.0f);
    // The scaled gain with scale 1 is exactly the plain gain.
    CHECK(Near(BeatingStressGainScaled(BeatingBlow_Knockout, 10, 40, Temperament_Brave, 1.0f, sub),
               BeatingStressGain(BeatingBlow_Knockout, 10, 40, Temperament_Brave, sub)));
}

void RecordingCountsEveryBlow()
{
    const char* name = "beating daily: every landed blow counts, even one the cap zeroed";
    const BeatingDay a = RecordBeating(Day(7, 0, 0.0f), 0.08f);
    CHECK(a.dayIndex == 7 && a.blows == 1 && Near(a.gained, 0.08f));
    const BeatingDay b = RecordBeating(a, 0.0f);
    CHECK(b.blows == 2 && Near(b.gained, 0.08f));
    const BeatingDay c = RecordBeating(b, -1.0f);
    CHECK(c.blows == 3 && Near(c.gained, 0.08f));
}

void ADayOfBeatingNeverPassesTheCap()
{
    const char* name = "beating daily: Golpear+Noquear every hour of a day gains at most the cap";
    const Submission sub = MechanicsSub();
    BeatingDay day = BeatingDayFor(BeatingDay(), 2);
    float total = 0.0f;
    for (int h = 0; h < 24; ++h)
    {
        const float hit = DailyBeatingGain(BeatingBlow_Hit, 40, 40, Temperament_Fearful, day, sub);
        day = RecordBeating(day, hit);
        const float ko = DailyBeatingGain(BeatingBlow_Knockout, 40, 40, Temperament_Fearful, day, sub);
        day = RecordBeating(day, ko);
        total += hit + ko;
    }
    CHECK(Near(total, 0.35f));
    CHECK(Near(day.gained, 0.35f));
    CHECK(day.blows == 48);
}

void TheLedgerKeepsTheDayPerPrisoner()
{
    const char* name = "beating daily: the ledger keeps one day record per prisoner";
    BeatingDayLedger ledger;
    const CharKey warden = CharKey::FromId("warden");
    const CharKey thief = CharKey::FromId("thief");
    CHECK(ledger.Today(warden, 4).blows == 0);
    ledger.Record(warden, RecordBeating(ledger.Today(warden, 4), 0.08f));
    ledger.Record(warden, RecordBeating(ledger.Today(warden, 4), 0.056f));
    CHECK(ledger.Today(warden, 4).blows == 2);
    CHECK(Near(ledger.Today(warden, 4).gained, 0.136f));
    CHECK(ledger.Today(thief, 4).blows == 0);
    // Tomorrow starts afresh.
    CHECK(ledger.Today(warden, 5).blows == 0);
    CHECK(ledger.Today(warden, 5).gained == 0.0f);
    CHECK(ledger.Size() == 1);
    ledger.Clear();
    CHECK(ledger.Size() == 0);
}

void TheSaveCarriesTheDay()
{
    const char* name = "beating daily: three save rows (day + 1, blows, thousandths); missing rows = fresh day";
    BeatingDayLedger ledger;
    const CharKey warden = CharKey::FromId("warden");
    CHECK(ledger.SavedDay(warden) == 0);
    CHECK(ledger.SavedBlows(warden) == 0);
    CHECK(ledger.SavedGained(warden) == 0);
    ledger.Record(warden, Day(0, 3, 0.2f));
    CHECK(ledger.SavedDay(warden) == 1);      // day 0 still saves as present
    CHECK(ledger.SavedBlows(warden) == 3);
    CHECK(ledger.SavedGained(warden) == 200);
    ledger.Record(warden, Day(12, 4, 0.3456f));
    CHECK(ledger.SavedDay(warden) == 13);
    CHECK(ledger.SavedGained(warden) == 346);

    // Restored in any order.
    BeatingDayLedger loaded;
    loaded.RestoreGained(warden, 346);
    loaded.RestoreBlows(warden, 4);
    loaded.RestoreDay(warden, 13);
    CHECK(loaded.Today(warden, 12).blows == 4);
    CHECK(Near(loaded.Today(warden, 12).gained, 0.346f));
    // An old save (no day row): counts without a day are a fresh day.
    BeatingDayLedger old;
    old.RestoreBlows(warden, 4);
    old.RestoreGained(warden, 346);
    CHECK(old.Today(warden, 12).blows == 0);
    CHECK(old.Today(warden, 12).gained == 0.0f);
    // Zero rows restore nothing.
    BeatingDayLedger zero;
    zero.RestoreDay(warden, 0);
    zero.RestoreBlows(warden, 0);
    zero.RestoreGained(warden, 0);
    CHECK(zero.Size() == 0);
}
} // namespace

void RunBeatingDailyRulesTests()
{
    TheDefaultsAreTheAgreedOnes();
    AStoredDayFromAnotherDayIsAFreshDay();
    TheMultiplierShrinksPerBlowOfTheDay();
    TheCapLeavesOnlyWhatIsLeftOfTheDay();
    TheDiminishedGainKeepsThePerBlowClamp();
    RecordingCountsEveryBlow();
    ADayOfBeatingNeverPassesTheCap();
    TheLedgerKeepsTheDayPerPrisoner();
    TheSaveCarriesTheDay();
}
