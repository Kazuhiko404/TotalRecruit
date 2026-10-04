// Tests for src/beating_stress_rules: what one Golpear blow or one Noquear
// knockout adds to a prisoner's beating stress (levels, temperament, min and max),
// how the bar fades over game hours, and the ledger that keeps it per character
// and hands it to the save.

#include "test_support.h"
#include "beating_stress_rules.h"

#include <cmath>

using namespace totalrecruit;

namespace
{
bool Near(float a, float b)
{
    return std::fabs(a - b) < 0.0001f;
}

bool NearD(double a, double b)
{
    return std::fabs(a - b) < 0.0001;
}

void TheDefaultsAreTheAgreedOnes()
{
    const char* name = "beating stress: defaults 0.08 / 0.15, gain 0.02..0.25, levels 0.5..1.5, 24 h fade";
    const Submission sub;
    CHECK(sub.beatStressBase == 0.08f);
    CHECK(sub.knockoutStressBase == 0.15f);
    CHECK(sub.beatingStressMin == 0.02f);
    CHECK(sub.beatingStressMax == 0.25f);
    CHECK(sub.beatingStressLevelMin == 0.5f);
    CHECK(sub.beatingStressLevelMax == 1.5f);
    CHECK(sub.beatingStressFadeHours == 24.0f);
}

void TheLevelFactorIsTheRatioClamped()
{
    const char* name = "beating stress: level factor = player / npc, clamped 0.5..1.5";
    const Submission sub;
    CHECK(Near(BeatingLevelFactor(20, 20, sub), 1.0f));
    CHECK(Near(BeatingLevelFactor(30, 20, sub), 1.5f));
    CHECK(Near(BeatingLevelFactor(60, 20, sub), 1.5f));    // capped
    CHECK(Near(BeatingLevelFactor(10, 20, sub), 0.5f));
    CHECK(Near(BeatingLevelFactor(1, 80, sub), 0.5f));     // floored
    CHECK(Near(BeatingLevelFactor(25, 20, sub), 1.25f));
    // An unreadable or zero level reads as 1, never a division by zero.
    CHECK(Near(BeatingLevelFactor(1, 0, sub), 1.0f));
    CHECK(Near(BeatingLevelFactor(0, 1, sub), 1.0f));
    CHECK(Near(BeatingLevelFactor(-5, -5, sub), 1.0f));
    // A max below the min reads as the min.
    Submission odd;
    odd.beatingStressLevelMin = 0.8f;
    odd.beatingStressLevelMax = 0.4f;
    CHECK(Near(BeatingLevelFactor(60, 20, odd), 0.8f));
    CHECK(Near(BeatingLevelFactor(10, 20, odd), 0.8f));
}

void TheGainWeighsBaseLevelsAndTemperament()
{
    const char* name = "beating stress: gain = clamp(base * levels / resolve, min, max)";
    const Submission sub;
    // Even levels, an ordinary temperament: the base itself.
    CHECK(Near(BeatingStressGain(BeatingBlow_Hit, 20, 20, Temperament_Normal, sub), 0.08f));
    CHECK(Near(BeatingStressGain(BeatingBlow_Knockout, 20, 20, Temperament_Normal, sub), 0.15f));
    // A coward (resolve 0.6) feels it more: 0.08 / 0.6.
    CHECK(Near(BeatingStressGain(BeatingBlow_Hit, 20, 20, Temperament_Fearful, sub),
               0.08f / 0.6f));
    // The honourable (1.8) feel it less: 0.08 / 1.8 = 0.044.
    CHECK(Near(BeatingStressGain(BeatingBlow_Hit, 20, 20, Temperament_Honorable, sub),
               0.08f / 1.8f));
    // Outclassed by the player: 0.08 * 1.5.
    CHECK(Near(BeatingStressGain(BeatingBlow_Hit, 40, 20, Temperament_Normal, sub), 0.12f));
    // A knockout on a coward the player outclasses: 0.15 * 1.5 / 0.6 = 0.375 -> max.
    CHECK(Near(BeatingStressGain(BeatingBlow_Knockout, 40, 20, Temperament_Fearful, sub), 0.25f));
    // A blow on an honourable stronger than the player: 0.08 * 0.5 / 1.8 = 0.022.
    CHECK(Near(BeatingStressGain(BeatingBlow_Hit, 10, 40, Temperament_Honorable, sub),
               0.08f * 0.5f / 1.8f));
    // ... and on a brave one: 0.08 * 0.5 / 1.6 = 0.025; with a higher min, the min.
    Submission floorHigh;
    floorHigh.beatingStressMin = 0.05f;
    CHECK(Near(BeatingStressGain(BeatingBlow_Hit, 10, 40, Temperament_Brave, floorHigh), 0.05f));
    // The knockout always weighs more than the blow, everything else equal.
    CHECK(BeatingStressGain(BeatingBlow_Knockout, 20, 30, Temperament_Brave, sub) >
          BeatingStressGain(BeatingBlow_Hit, 20, 30, Temperament_Brave, sub));
}

void AZeroBaseSwitchesTheTermOff()
{
    const char* name = "beating stress: a base of 0 adds nothing (the min does not apply)";
    Submission sub;
    sub.beatStressBase = 0.0f;
    sub.knockoutStressBase = -1.0f;
    CHECK(BeatingStressGain(BeatingBlow_Hit, 40, 20, Temperament_Fearful, sub) == 0.0f);
    CHECK(BeatingStressGain(BeatingBlow_Knockout, 40, 20, Temperament_Fearful, sub) == 0.0f);
    // A max below the min reads as the min; a resolve of 0 reads as 1.
    Submission odd;
    odd.beatingStressMin = 0.1f;
    odd.beatingStressMax = 0.05f;
    odd.braveResolve = 0.0f;
    CHECK(Near(BeatingStressGain(BeatingBlow_Knockout, 40, 20, Temperament_Brave, odd), 0.1f));
}

void TheBarFadesLinearly()
{
    const char* name = "beating stress: fades linearly, a full bar in fadeHours, never below 0";
    CHECK(Near(DecayedBeatingStress(1.0f, 100.0, 100.0, 24.0f), 1.0f));
    CHECK(Near(DecayedBeatingStress(1.0f, 100.0, 112.0, 24.0f), 0.5f));
    CHECK(Near(DecayedBeatingStress(0.3f, 100.0, 106.0, 24.0f), 0.05f));
    CHECK(DecayedBeatingStress(0.3f, 100.0, 124.0, 24.0f) == 0.0f);
    CHECK(DecayedBeatingStress(1.0f, 100.0, 1000.0, 24.0f) == 0.0f);
    // The clock reads earlier than the last blow (a load of an earlier save): no
    // decay, and never a rise.
    CHECK(Near(DecayedBeatingStress(0.4f, 100.0, 90.0, 24.0f), 0.4f));
    // A fade of 0 or less: no decay at all.
    CHECK(Near(DecayedBeatingStress(0.4f, 100.0, 500.0, 0.0f), 0.4f));
    // A stored value out of range is clamped first.
    CHECK(Near(DecayedBeatingStress(3.0f, 100.0, 100.0, 24.0f), 1.0f));
    CHECK(DecayedBeatingStress(-1.0f, 100.0, 100.0, 24.0f) == 0.0f);
}

void TheLedgerAccumulatesAndClampsAtOne()
{
    const char* name = "beating stress ledger: adds per character, fades on read, stops at 1";
    BeatingStressLedger ledger;
    const CharKey warden = CharKey::FromId("warden");
    const CharKey other = CharKey::FromId("other");
    CHECK(ledger.Get(warden, 100.0, 24.0f) == 0.0f);

    CHECK(Near(ledger.Add(warden, 0.25f, 100.0, 24.0f), 0.25f));
    CHECK(Near(ledger.Add(warden, 0.25f, 100.0, 24.0f), 0.5f));
    CHECK(ledger.Get(other, 100.0, 24.0f) == 0.0f);
    // Six hours later a quarter of the bar has faded: 0.5 - 0.25.
    CHECK(Near(ledger.Get(warden, 106.0, 24.0f), 0.25f));
    // A blow then adds to what is left, not to what was.
    CHECK(Near(ledger.Add(warden, 0.1f, 106.0, 24.0f), 0.35f));
    CHECK(Near(ledger.Get(warden, 106.0, 24.0f), 0.35f));
    // Never past a full bar.
    for (int i = 0; i < 10; ++i)
    {
        ledger.Add(warden, 0.25f, 106.0, 24.0f);
    }
    CHECK(ledger.Get(warden, 106.0, 24.0f) == 1.0f);
    // A gain of 0 or less changes nothing but the decay.
    CHECK(Near(ledger.Add(warden, -0.5f, 118.0, 24.0f), 0.5f));
    CHECK(ledger.Size() == 1);
    ledger.Clear();
    CHECK(ledger.Size() == 0);
    CHECK(ledger.Get(warden, 118.0, 24.0f) == 0.0f);
}

void TheClockGoingBackDoesNotBreakTheLedger()
{
    const char* name = "beating stress ledger: a clock earlier than the last blow neither decays nor rises";
    BeatingStressLedger ledger;
    const CharKey warden = CharKey::FromId("warden");
    ledger.Add(warden, 0.4f, 200.0, 24.0f);
    CHECK(Near(ledger.Get(warden, 150.0, 24.0f), 0.4f));
    // Adding then restarts the fade from the earlier clock.
    CHECK(Near(ledger.Add(warden, 0.1f, 150.0, 24.0f), 0.5f));
    CHECK(Near(ledger.Get(warden, 156.0, 24.0f), 0.25f));
}

void TheSaveCarriesValueAndHour()
{
    const char* name = "beating stress ledger: the save holds the value (milli) and its game hour (tenths)";
    CHECK(BeatingStressToSaved(0.0f) == 0);
    CHECK(BeatingStressToSaved(0.25f) == 250);
    CHECK(BeatingStressToSaved(1.0f) == 1000);
    CHECK(BeatingStressToSaved(2.0f) == 1000);
    CHECK(BeatingStressToSaved(-1.0f) == 0);
    CHECK(Near(BeatingStressFromSaved(250), 0.25f));
    CHECK(BeatingStressFromSaved(5000) == 1.0f);
    CHECK(BeatingStressFromSaved(-3) == 0.0f);
    CHECK(GameHourToSavedTenths(123.46) == 1235);
    CHECK(GameHourToSavedTenths(-4.0) == 0);
    CHECK(NearD(GameHourFromSavedTenths(1235), 123.5));

    BeatingStressLedger ledger;
    const CharKey warden = CharKey::FromId("warden");
    ledger.Add(warden, 0.6f, 100.0, 24.0f);
    CHECK(ledger.SavedValue(warden) == 600);
    CHECK(ledger.SavedHourTenths(warden) == 1000);
    CHECK(ledger.SavedValue(CharKey::FromId("nobody")) == 0);
    CHECK(ledger.SavedHourTenths(CharKey::FromId("nobody")) == 0);

    // A load: cleared, then each row put back in either order.
    BeatingStressLedger loaded;
    loaded.RestoreHourTenths(warden, 1000);
    loaded.RestoreValue(warden, 600);
    CHECK(Near(loaded.Get(warden, 112.0, 24.0f), 0.1f));
    BeatingStressLedger other;
    other.RestoreValue(warden, 600);
    other.RestoreHourTenths(warden, 1000);
    CHECK(Near(other.Get(warden, 112.0, 24.0f), 0.1f));
    // A value without its hour: shown undecayed rather than guessed.
    BeatingStressLedger noHour;
    noHour.RestoreValue(warden, 600);
    CHECK(Near(noHour.Get(warden, 500.0, 24.0f), 0.6f));
    // A zero value restores nothing.
    BeatingStressLedger zero;
    zero.RestoreValue(warden, 0);
    CHECK(zero.Size() == 0);
}
} // namespace

void RunBeatingStressRulesTests()
{
    TheDefaultsAreTheAgreedOnes();
    TheLevelFactorIsTheRatioClamped();
    TheGainWeighsBaseLevelsAndTemperament();
    AZeroBaseSwitchesTheTermOff();
    TheBarFadesLinearly();
    TheLedgerAccumulatesAndClampsAtOne();
    TheClockGoingBackDoesNotBreakTheLedger();
    TheSaveCarriesValueAndHour();
}
