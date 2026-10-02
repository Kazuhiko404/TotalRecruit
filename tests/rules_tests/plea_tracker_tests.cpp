// Tests for src/plea_tracker.cpp: whose plea for mercy is open, and that every
// way of ending it frees the next one.

#include "test_support.h"
#include "plea_tracker.h"

using namespace totalrecruit;

namespace
{
const CharKey kAlice(10, 1);
const CharKey kBob(11, 1);

void StartsEmpty()
{
    const char* name = "plea: nothing pending to begin with";
    PleaTracker t;
    CHECK(!t.HasPending());
    CHECK(!t.IsPending(kAlice));
}

void OnePleaAtATime()
{
    const char* name = "plea: a second plea cannot start while one is open";
    PleaTracker t;
    CHECK(t.Begin(kAlice));
    CHECK(!t.Begin(kBob));
    CHECK(t.IsPending(kAlice));
    CHECK(!t.IsPending(kBob));
}

// T3 -- OnAccept never cleared the pending plea, so no further plea fired until
// the game was reloaded.
void ClearingFreesTheNextPlea()
{
    const char* name = "plea: after the plea ends, the next one can begin";
    PleaTracker t;
    CHECK(t.Begin(kAlice));
    CHECK(t.ClearIf(kAlice));
    CHECK(!t.HasPending());
    CHECK(!t.IsPending(kAlice));
    CHECK(t.Begin(kBob));
}

void ClearIfIgnoresOtherCharacters()
{
    const char* name = "plea: clearing for someone else leaves the open plea alone";
    PleaTracker t;
    CHECK(t.Begin(kAlice));
    CHECK(!t.ClearIf(kBob));
    CHECK(t.IsPending(kAlice));
}

void ClearEndsWhoeverIsPending()
{
    const char* name = "plea: Clear() ends the plea when the window closes with no name";
    PleaTracker t;
    CHECK(t.Begin(kAlice));
    t.Clear();
    CHECK(!t.HasPending());
    CHECK(t.Begin(kBob));
}

void ClearOnEmptyIsHarmless()
{
    const char* name = "plea: clearing when nothing is pending changes nothing";
    PleaTracker t;
    t.Clear();
    CHECK(!t.ClearIf(kAlice));
    CHECK(!t.HasPending());
}

// T3 -- the pending plea was a raw Character*. The engine reuses the address of
// a freed character, so a stranger at that address matched the dead one's plea.
// The handle's serial tells a reused slot from its previous occupant.
void ReusedSlotWithNewSerialDoesNotMatch()
{
    const char* name = "plea: a reused slot (same index, new serial) is not the pleading character";
    PleaTracker t;
    CHECK(t.Begin(CharKey(10, 1)));
    CHECK(!t.IsPending(CharKey(10, 2)));
    CHECK(!t.ClearIf(CharKey(10, 2)));
    CHECK(t.IsPending(CharKey(10, 1)));
}
} // namespace

void RunPleaTrackerTests()
{
    StartsEmpty();
    OnePleaAtATime();
    ClearingFreesTheNextPlea();
    ClearIfIgnoresOtherCharacters();
    ClearEndsWhoeverIsPending();
    ClearOnEmptyIsHarmless();
    ReusedSlotWithNewSerialDoesNotMatch();
}
