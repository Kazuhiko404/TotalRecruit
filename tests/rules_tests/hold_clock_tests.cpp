// Tests for src/hold_clock.cpp: the timing behind Cheat's wait.

#include "test_support.h"
#include "hold_clock.h"
#include "config.h"

#include <climits>

using namespace totalrecruit;

namespace
{
const CharKey kAlice(10, 1);
const CharKey kBob(11, 1);

bool Near(float a, float b)
{
    const float d = a - b;
    return d < 0.001f && d > -0.001f;
}

void NotHeldMeansZero()
{
    const char* name = "hold: someone never seen caged has been held for 0 hours";
    HoldClock c;
    CHECK(Near(c.HoursHeld(kAlice, 100.0), 0.0f));
    CHECK(!c.IsTracked(kAlice));
}

void ClockStartsAtTheFirstSightingCaged()
{
    const char* name = "hold: the clock counts from the first sighting in the cage";
    HoldClock c;
    c.Observe(kAlice, true, 10.0);
    CHECK(c.IsTracked(kAlice));
    CHECK(Near(c.HoursHeld(kAlice, 10.0), 0.0f));
    CHECK(Near(c.HoursHeld(kAlice, 16.0), 6.0f));
}

// T4 -- the clock started at the first Evaluate, i.e. whenever the player first
// clicked, so a prisoner caged for a day looked freshly caged.
void AskingNeverStartsTheClock()
{
    const char* name = "hold: asking how long does not start the clock, only a sighting does";
    HoldClock c;
    CHECK(Near(c.HoursHeld(kAlice, 10.0), 0.0f));
    c.Observe(kAlice, true, 12.0);
    CHECK(Near(c.HoursHeld(kAlice, 13.0), 1.0f));
}

void LaterSightingsDoNotRestartTheClock()
{
    const char* name = "hold: seeing them caged again later keeps the original start";
    HoldClock c;
    c.Observe(kAlice, true, 10.0);
    c.Observe(kAlice, true, 14.0);
    c.Observe(kAlice, true, 20.0);
    CHECK(Near(c.HoursHeld(kAlice, 22.0), 12.0f));
}

// T4 -- entries were never pruned: a prisoner released and caged again joined
// instantly on the strength of the previous stay.
void ReleasedPrisonerLosesTheirTime()
{
    const char* name = "hold: seen free, the clock is dropped";
    HoldClock c;
    c.Observe(kAlice, true, 0.0);
    CHECK(Near(c.HoursHeld(kAlice, 5.0), 5.0f));
    c.Observe(kAlice, false, 5.0);
    CHECK(!c.IsTracked(kAlice));
    CHECK(Near(c.HoursHeld(kAlice, 6.0), 0.0f));
}

void RecagedPrisonerStartsOver()
{
    const char* name = "hold: released then caged again counts from the new cage";
    HoldClock c;
    c.Observe(kAlice, true, 0.0);
    c.Observe(kAlice, false, 5.0);
    c.Observe(kAlice, true, 8.0);
    CHECK(Near(c.HoursHeld(kAlice, 9.0), 1.0f));
}

void PrisonersAreIndependent()
{
    const char* name = "hold: each prisoner has their own clock";
    HoldClock c;
    c.Observe(kAlice, true, 0.0);
    c.Observe(kBob, true, 4.0);
    c.Observe(kAlice, false, 6.0);
    CHECK(!c.IsTracked(kAlice));
    CHECK(Near(c.HoursHeld(kBob, 6.0), 2.0f));
    CHECK(c.TrackedCount() == 1);
}

void FreeCharactersAreNeverTracked()
{
    const char* name = "hold: sightings of free characters leave no entry";
    HoldClock c;
    c.Observe(kAlice, false, 3.0);
    CHECK(c.TrackedCount() == 0);
}

void ReusedSlotIsAnotherPrisoner()
{
    const char* name = "hold: a reused handle slot (new serial) does not inherit the clock";
    HoldClock c;
    c.Observe(CharKey(10, 1), true, 0.0);
    CHECK(Near(c.HoursHeld(CharKey(10, 2), 5.0), 0.0f));
}

// A loaded older save puts the game clock behind a start time recorded in this
// session. Counting from the future would leave the prisoner unreachable until
// the clock caught up, so the sighting restarts it instead.
void ClockGoingBackwardsRestartsTheHold()
{
    const char* name = "hold: a game clock that went backwards (load) restarts the hold";
    HoldClock c;
    c.Observe(kAlice, true, 100.0);
    CHECK(Near(c.HoursHeld(kAlice, 40.0), 0.0f));
    c.Observe(kAlice, true, 40.0);
    CHECK(Near(c.HoursHeld(kAlice, 43.0), 3.0f));
}

void ClearForgetsEveryone()
{
    const char* name = "hold: Clear() drops every clock";
    HoldClock c;
    c.Observe(kAlice, true, 0.0);
    c.Observe(kBob, true, 0.0);
    c.Clear();
    CHECK(c.TrackedCount() == 0);
}
// ------------------------------------------- the hold start in the save (P3)
// The hold start rides in the character save (saved_ints, tenths of a game hour);
// load_boundary clears the clock at load begin and each character's load hook
// puts its start back. The sweep's next sighting then keeps it (still caged),
// drops it (no longer caged) or restarts it (the clock is behind it).

void SavedStartRoundTripsToTheSameHours()
{
    const char* name = "hold save: a saved start restores the same hours held";
    HoldClock before;
    before.Observe(kAlice, true, 100.25);
    const int saved = before.SavedStartTenths(kAlice);
    CHECK(saved == 1003);   // rounded to tenths of an hour

    HoldClock after;
    after.RestoreStartTenths(kAlice, saved);
    CHECK(after.IsTracked(kAlice));
    CHECK(Near(after.HoursHeld(kAlice, 106.3), 6.0f));
}

void UntrackedSavesZeroAndZeroRestoresNothing()
{
    const char* name = "hold save: nobody held saves 0, and 0 or less restores nothing";
    HoldClock c;
    CHECK(c.SavedStartTenths(kAlice) == 0);
    c.RestoreStartTenths(kAlice, 0);
    c.RestoreStartTenths(kBob, -5);
    CHECK(!c.IsTracked(kAlice));
    CHECK(!c.IsTracked(kBob));
    CHECK(c.TrackedCount() == 0);
}

void RestoredPrisonerStillCagedKeepsHisHours()
{
    const char* name = "hold save: still in the cage after the load, the hours carry on";
    HoldClock c;
    c.RestoreStartTenths(kAlice, HoldStartToSavedTenths(10.0));
    c.Observe(kAlice, true, 15.0);
    CHECK(Near(c.HoursHeld(kAlice, 15.0), 5.0f));
    CHECK(Near(c.HoursHeld(kAlice, 18.0), 8.0f));
}

void RestoredPrisonerNoLongerCagedIsCleared()
{
    const char* name = "hold save: no longer in the cage after the load, the clock is dropped";
    HoldClock c;
    c.RestoreStartTenths(kAlice, HoldStartToSavedTenths(10.0));
    c.Observe(kAlice, false, 15.0);
    CHECK(!c.IsTracked(kAlice));
    CHECK(Near(c.HoursHeld(kAlice, 15.0), 0.0f));
}

void RestoredStartAfterTheClockRestartsTheHold()
{
    const char* name = "hold save: a start later than the game clock restarts the hold safely";
    HoldClock c;
    c.RestoreStartTenths(kAlice, HoldStartToSavedTenths(200.0));
    CHECK(Near(c.HoursHeld(kAlice, 50.0), 0.0f));   // never negative
    c.Observe(kAlice, true, 50.0);
    CHECK(Near(c.HoursHeld(kAlice, 52.0), 2.0f));
}

void SavedStartStaysAPositiveInt()
{
    const char* name = "hold save: the saved start is a positive int even at hour 0 or far ahead";
    CHECK(HoldStartToSavedTenths(0.0) == 1);
    CHECK(HoldStartToSavedTenths(-4.0) == 1);
    CHECK(HoldStartToSavedTenths(0.04) == 1);
    CHECK(HoldStartToSavedTenths(3.0) == 30);
    CHECK(HoldStartToSavedTenths(1.0e12) == INT_MAX);
    CHECK(HoldStartFromSavedTenths(30) > 2.99 && HoldStartFromSavedTenths(30) < 3.01);
    CHECK(HoldStartFromSavedTenths(0) == 0.0);
}
} // namespace

void RunHoldClockTests()
{
    NotHeldMeansZero();
    ClockStartsAtTheFirstSightingCaged();
    AskingNeverStartsTheClock();
    LaterSightingsDoNotRestartTheClock();
    ReleasedPrisonerLosesTheirTime();
    RecagedPrisonerStartsOver();
    PrisonersAreIndependent();
    FreeCharactersAreNeverTracked();
    ReusedSlotIsAnotherPrisoner();
    ClockGoingBackwardsRestartsTheHold();
    ClearForgetsEveryone();
    SavedStartRoundTripsToTheSameHours();
    UntrackedSavesZeroAndZeroRestoresNothing();
    RestoredPrisonerStillCagedKeepsHisHours();
    RestoredPrisonerNoLongerCagedIsCleared();
    RestoredStartAfterTheClockRestartsTheHold();
    SavedStartStaysAPositiveInt();
}
