// Tests for src/haggle_rules.cpp and src/haggle_ledger.cpp: the pure half of
// haggling over a cats price (slider bounds, acceptance odds, the daily haggle
// draw, the offence lock and its markup).

#include "test_support.h"
#include "haggle_rules.h"
#include "haggle_ledger.h"
#include "chance_rules.h"

#include <cstdio>

using namespace totalrecruit;

namespace
{
const Prices kPrices = Prices();

bool Near(float a, float b)
{
    const float d = a - b;
    return d < 0.0001f && d > -0.0001f;
}

CharKey IdKey(const char* format, int i)
{
    char id[96];
    _snprintf_s(id, sizeof(id), _TRUNCATE, format, i);
    return CharKey::FromId(id);
}

// ---------------------------------------------------------------- config

void HaggleDefaultsAreTheDesign()
{
    const char* name = "haggle config: shipped defaults";
    CHECK(kPrices.haggleEnabled);
    CHECK(kPrices.haggleMinFraction == 0.5f);
    CHECK(kPrices.haggleDiscountWeight == 2.0f);
    CHECK(kPrices.haggleLeverageWeight == 0.5f);
    CHECK(kPrices.haggleOffenceMarkup == 0.2f);
}

// ---------------------------------------------------------------- slider

void BoundsRunFromHalfToTheAskInTens()
{
    const char* name = "haggle bounds: min is half the ask rounded up to 10, max the ask, step 10";
    HaggleBounds b = HaggleSliderBounds(1000, kPrices);
    CHECK(b.minOffer == 500);
    CHECK(b.maxOffer == 1000);
    CHECK(b.step == 10);

    b = HaggleSliderBounds(1230, kPrices);      // 615 -> 620
    CHECK(b.minOffer == 620);
    CHECK(b.maxOffer == 1230);

    b = HaggleSliderBounds(205, kPrices);       // 102.5 -> 110, max stays the ask
    CHECK(b.minOffer == 110);
    CHECK(b.maxOffer == 205);
}

void BoundsNeverInvert()
{
    const char* name = "haggle bounds: min never above max, nothing negative";
    Prices whole = kPrices;
    whole.haggleMinFraction = 1.0f;
    HaggleBounds b = HaggleSliderBounds(1005, whole);   // 1005 -> 1010 would pass the ask
    CHECK(b.minOffer == 1005);
    CHECK(b.maxOffer == 1005);

    b = HaggleSliderBounds(0, kPrices);
    CHECK(b.minOffer == 0 && b.maxOffer == 0);
    b = HaggleSliderBounds(-50, kPrices);
    CHECK(b.minOffer == 0 && b.maxOffer == 0);
}

void RoundUpToStepRounds()
{
    const char* name = "haggle bounds: round up to the step";
    CHECK(RoundUpToStep(0, 10) == 0);
    CHECK(RoundUpToStep(1, 10) == 10);
    CHECK(RoundUpToStep(10, 10) == 10);
    CHECK(RoundUpToStep(611, 10) == 620);
    CHECK(RoundUpToStep(37, 0) == 37);
}

void ClampOfferHoldsTheSlider()
{
    const char* name = "haggle offer: clamped to the bounds and snapped to 10";
    const HaggleBounds b = HaggleSliderBounds(1000, kPrices);
    CHECK(ClampOffer(100, b) == 500);
    CHECK(ClampOffer(5000, b) == 1000);
    CHECK(ClampOffer(734, b) == 730);
    CHECK(ClampOffer(735, b) == 740);
    CHECK(ClampOffer(1000, b) == 1000);

    // An ask that is not a multiple of 10 is still reachable as the top.
    const HaggleBounds odd = HaggleSliderBounds(205, kPrices);
    CHECK(ClampOffer(204, odd) == 205);
    CHECK(ClampOffer(205, odd) == 205);
    CHECK(ClampOffer(150, odd) == 150);
}

void SliderStepsRunFromTheFloorToTheAsk()
{
    const char* name = "haggle steps: position 0 is the floor, the last is the ask";
    const HaggleBounds b = HaggleSliderBounds(9570, kPrices);
    CHECK(b.minOffer == 4790);
    CHECK(HaggleStepCount(b) == 478);
    CHECK(OfferAtStep(b, 0) == 4790);
    CHECK(OfferAtStep(b, 1) == 4800);
    CHECK(OfferAtStep(b, 286) == 7650);
    CHECK(OfferAtStep(b, 477) == 9560);
    CHECK(OfferAtStep(b, 478) == 9570);
    CHECK(OfferAtStep(b, -5) == 4790);
    CHECK(OfferAtStep(b, 999) == 9570);

    CHECK(StepOfOffer(b, 4790) == 0);
    CHECK(StepOfOffer(b, 7650) == 286);
    CHECK(StepOfOffer(b, 9570) == 478);
    CHECK(StepOfOffer(b, 100) == 0);
    CHECK(StepOfOffer(b, 99999) == 478);
}

void SliderStepsReachAnOffGridAsk()
{
    const char* name = "haggle steps: an ask off the 10 grid is the last step";
    const HaggleBounds odd = HaggleSliderBounds(205, kPrices);
    // 110, 120, ... 200, then 205.
    CHECK(odd.minOffer == 110);
    CHECK(HaggleStepCount(odd) == 10);
    CHECK(OfferAtStep(odd, 9) == 200);
    CHECK(OfferAtStep(odd, 10) == 205);
    CHECK(StepOfOffer(odd, 205) == 10);
    CHECK(StepOfOffer(odd, 200) == 9);

    // Every step maps back to itself.
    const HaggleBounds b = HaggleSliderBounds(1234, kPrices);
    const int n = HaggleStepCount(b);
    bool roundTrips = true;
    for (int i = 0; i <= n; ++i)
    {
        if (StepOfOffer(b, OfferAtStep(b, i)) != i) roundTrips = false;
    }
    CHECK(roundTrips);
}

void SliderStepsOfNothingAreOnePosition()
{
    const char* name = "haggle steps: no range is a single position";
    const HaggleBounds none = HaggleSliderBounds(0, kPrices);
    CHECK(HaggleStepCount(none) == 0);
    CHECK(OfferAtStep(none, 3) == 0);
    CHECK(StepOfOffer(none, 50) == 0);

    Prices full = kPrices;
    full.haggleMinFraction = 1.0f;
    const HaggleBounds flat = HaggleSliderBounds(500, full);
    CHECK(HaggleStepCount(flat) == 0);
    CHECK(OfferAtStep(flat, 0) == 500);
}

// ---------------------------------------------------------------- odds

void FullPriceIsCertain()
{
    const char* name = "haggle odds: the asking price (or more) is always accepted";
    CHECK(AcceptOdds(1000, 1000, 0, Temperament_Normal, kPrices) == 100);
    CHECK(AcceptOdds(1000, 1200, -20, Temperament_Honorable, kPrices) == 100);
    CHECK(AcceptOdds(0, 0, -20, Temperament_ColdCruel, kPrices) == 100);
}

void OddsAreTheFormula()
{
    const char* name = "haggle odds: 100 - discount% * weight + leverage + temperament";
    CHECK(AcceptOdds(1000, 800, 0, Temperament_Normal, kPrices) == 60);    // 20% off
    CHECK(AcceptOdds(1000, 800, 10, Temperament_Normal, kPrices) == 70);
    CHECK(AcceptOdds(1000, 900, -5, Temperament_Normal, kPrices) == 75);   // 10% off
    Prices soft = kPrices;
    soft.haggleDiscountWeight = 1.0f;
    CHECK(AcceptOdds(1000, 800, 0, Temperament_Normal, soft) == 80);
}

void OddsFallAsTheDiscountGrows()
{
    const char* name = "haggle odds: monotonic, a lower offer is never likelier";
    const Temperament kinds[4] = {
        Temperament_Normal, Temperament_Fearful, Temperament_Brave, Temperament_Honorable
    };
    for (int k = 0; k < 4; ++k)
    {
        for (int lev = -20; lev <= 20; lev += 10)
        {
            int previous = 101;
            for (int offer = 1000; offer >= 500; offer -= 10)
            {
                const int odds = AcceptOdds(1000, offer, lev, kinds[k], kPrices);
                CHECK(odds <= previous);
                previous = odds;
            }
        }
    }
}

void RealDiscountIsNeverCertainNorBelowZero()
{
    const char* name = "haggle odds: a real discount stays inside 0..95";
    CHECK(AcceptOdds(1000, 990, 20, Temperament_Fearful, kPrices) == 95);
    CHECK(AcceptOdds(1000, 500, -20, Temperament_Honorable, kPrices) == 0);
    for (int offer = 500; offer < 1000; offer += 10)
    {
        const int odds = AcceptOdds(1000, offer, 20, Temperament_Fearful, kPrices);
        CHECK(odds >= 0 && odds <= 95);
    }
}

void TemperamentShiftsTheOdds()
{
    const char* name = "haggle odds: fearful +15, brave/cold/maniacal -10, honourable -15";
    CHECK(HagglePersonalityShift(Temperament_Normal) == 0);
    CHECK(HagglePersonalityShift(Temperament_Fearful) == 15);
    CHECK(HagglePersonalityShift(Temperament_Brave) == -10);
    CHECK(HagglePersonalityShift(Temperament_ColdCruel) == -10);
    CHECK(HagglePersonalityShift(Temperament_Maniacal) == -10);
    CHECK(HagglePersonalityShift(Temperament_Honorable) == -15);
    CHECK(HagglePersonalityShift(Temperament_Smart) == 0);
    CHECK(HagglePersonalityShift((Temperament)99) == 0);

    CHECK(AcceptOdds(1000, 800, 0, Temperament_Fearful, kPrices) == 75);
    CHECK(AcceptOdds(1000, 800, 0, Temperament_Brave, kPrices) == 50);
    CHECK(AcceptOdds(1000, 800, 0, Temperament_Honorable, kPrices) == 45);
}

// ---------------------------------------------------------------- leverage

void LeverageIsTheStatGapClamped()
{
    const char* name = "haggle leverage: (speaker best stat - npc level) * 0.5, within +-20";
    CHECK(HaggleLeverage(50, 20, kPrices) == 15);
    CHECK(HaggleLeverage(20, 20, kPrices) == 0);
    CHECK(HaggleLeverage(10, 30, kPrices) == -10);
    CHECK(HaggleLeverage(100, 10, kPrices) == 20);
    CHECK(HaggleLeverage(0, 80, kPrices) == -20);
    Prices flat = kPrices;
    flat.haggleLeverageWeight = 0.0f;
    CHECK(HaggleLeverage(100, 1, flat) == 0);
}

// ---------------------------------------------------------------- resolution

void ResolveHonoursTheExtremes()
{
    const char* name = "haggle resolve: 100 always accepted, 0 always offended";
    for (int i = 0; i < 100; ++i)
    {
        CHECK(ResolveHaggle(100, IdKey("x-%d", i), i) == Haggle_Accepted);
        CHECK(ResolveHaggle(0, IdKey("x-%d", i), i) == Haggle_Offended);
    }
}

void ResolveIsDeterministicPerDay()
{
    const char* name = "haggle resolve: same key, day and odds always answer the same";
    for (int i = 0; i < 200; ++i)
    {
        const HaggleOutcome first = ResolveHaggle(55, IdKey("deal-%d", i), 8);
        CHECK(ResolveHaggle(55, IdKey("deal-%d", i), 8) == first);
        CHECK(ResolveHaggle(55, IdKey("deal-%d", i), 8) == first);
    }
}

void ResolveIsMonotonicInTheOdds()
{
    const char* name = "haggle resolve: accepted at p means accepted at every higher p";
    for (int i = 0; i < 200; ++i)
    {
        bool accepted = false;
        for (int p = 1; p < 100; ++p)
        {
            const bool ok = ResolveHaggle(p, IdKey("m-%d", i), 2) == Haggle_Accepted;
            if (accepted)
            {
                CHECK(ok);
            }
            accepted = accepted || ok;
        }
    }
}

void ResolveIsNotTheJoinDraw()
{
    const char* name = "haggle resolve: a different coin from the join roll on the same day";
    int agree = 0;
    const int total = 1000;
    for (int i = 0; i < total; ++i)
    {
        const CharKey key = IdKey("both-%d", i);
        const bool haggled = ResolveHaggle(50, key, 6) == Haggle_Accepted;
        if (haggled == RollsToJoin(true, key, 6, 50)) ++agree;
    }
    CHECK(agree > total * 40 / 100 && agree < total * 60 / 100);
}

void ResolveApproximatesTheOdds()
{
    const char* name = "haggle resolve: about 60% of haggles pass at 60";
    int passes = 0;
    const int total = 2000;
    for (int i = 0; i < total; ++i)
    {
        if (ResolveHaggle(60, IdKey("rate-%d", i), i % 30) == Haggle_Accepted) ++passes;
    }
    CHECK(passes > total * 55 / 100 && passes < total * 65 / 100);
}

// ---------------------------------------------------------------- offence

void OffendedAskIsMarkedUp()
{
    const char* name = "haggle offence: an offended NPC asks +20%, rounded up to 10";
    CHECK(OffendedAsk(1000, false, kPrices) == 1000);
    CHECK(OffendedAsk(1000, true, kPrices) == 1200);
    CHECK(OffendedAsk(1005, true, kPrices) == 1210);   // 1206 -> 1210
    Prices none = kPrices;
    none.haggleOffenceMarkup = 0.0f;
    CHECK(OffendedAsk(1005, true, none) == 1005);
}

void LedgerLocksOnlyTheOffendingDay()
{
    const char* name = "haggle ledger: locked and marked up on the offending day only";
    HaggleLedger ledger;
    const CharKey key = IdKey("proud-%d", 1);
    const CharKey other = IdKey("proud-%d", 2);

    CHECK(!ledger.IsHaggleLocked(key, 5));
    CHECK(Near(ledger.OffendedMarkup(key, 5, kPrices), 1.0f));

    ledger.RecordOffence(key, 5);
    CHECK(ledger.IsHaggleLocked(key, 5));
    CHECK(Near(ledger.OffendedMarkup(key, 5, kPrices), 1.2f));
    CHECK(!ledger.IsHaggleLocked(key, 6));
    CHECK(Near(ledger.OffendedMarkup(key, 6, kPrices), 1.0f));
    CHECK(!ledger.IsHaggleLocked(key, 4));
    CHECK(!ledger.IsHaggleLocked(other, 5));
    CHECK(ledger.Count() == 1);
}

void LedgerMovesTheLockToALaterOffence()
{
    const char* name = "haggle ledger: a new offence on a later day replaces the old one";
    HaggleLedger ledger;
    const CharKey key(3u, 9u);
    ledger.RecordOffence(key, 5);
    ledger.RecordOffence(key, 9);
    CHECK(!ledger.IsHaggleLocked(key, 5));
    CHECK(ledger.IsHaggleLocked(key, 9));
    CHECK(ledger.Count() == 1);
}

void LedgerClears()
{
    const char* name = "haggle ledger: Clear forgets every offence (load boundary)";
    HaggleLedger ledger;
    ledger.RecordOffence(IdKey("a-%d", 0), 1);
    ledger.RecordOffence(IdKey("a-%d", 1), 1);
    CHECK(ledger.Count() == 2);
    ledger.Clear();
    CHECK(ledger.Count() == 0);
    CHECK(!ledger.IsHaggleLocked(IdKey("a-%d", 0), 1));
}

void SessionLedgerIsOneInstance()
{
    const char* name = "haggle ledger: the session ledger is one process-wide table";
    SessionHaggleLedger().Clear();
    SessionHaggleLedger().RecordOffence(IdKey("s-%d", 0), 2);
    CHECK(SessionHaggleLedger().IsHaggleLocked(IdKey("s-%d", 0), 2));
    SessionHaggleLedger().Clear();
    CHECK(SessionHaggleLedger().Count() == 0);
}
} // namespace

void RunHaggleRulesTests()
{
    HaggleDefaultsAreTheDesign();
    BoundsRunFromHalfToTheAskInTens();
    BoundsNeverInvert();
    RoundUpToStepRounds();
    ClampOfferHoldsTheSlider();
    SliderStepsRunFromTheFloorToTheAsk();
    SliderStepsReachAnOffGridAsk();
    SliderStepsOfNothingAreOnePosition();
    FullPriceIsCertain();
    OddsAreTheFormula();
    OddsFallAsTheDiscountGrows();
    RealDiscountIsNeverCertainNorBelowZero();
    TemperamentShiftsTheOdds();
    LeverageIsTheStatGapClamped();
    ResolveHonoursTheExtremes();
    ResolveIsDeterministicPerDay();
    ResolveIsMonotonicInTheOdds();
    ResolveIsNotTheJoinDraw();
    ResolveApproximatesTheOdds();
    OffendedAskIsMarkedUp();
    LedgerLocksOnlyTheOffendingDay();
    LedgerMovesTheLockToALaterOffence();
    LedgerClears();
    SessionLedgerIsOneInstance();
}
