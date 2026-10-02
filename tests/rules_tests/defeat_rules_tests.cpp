// Tests for src/defeat_rules.cpp: what a reload leaves of the defeat count.

#include "test_support.h"
#include "defeat_rules.h"
#include "config.h"

using namespace totalrecruit;

namespace
{
void UnmarkedCharacterHasNoFloor()
{
    const char* name = "floor: no memory tag, no floor";
    CHECK(PersistedDefeatFloor(false, 1, 2) == 0);
    CHECK(PersistedDefeatFloor(false, 5, 2) == 0);
}

// T2c -- the shipped defaults were persistedFloorCount 1 under defeatsToBreak 2.
void MarkedFloorReachesDefeatsToBreak()
{
    const char* name = "floor: a marked character keeps at least defeatsToBreak after a reload";
    Submission sub;
    CHECK(PersistedDefeatFloor(true, sub.persistedFloorCount, sub.defeatsToBreak)
          >= sub.defeatsToBreak);
    CHECK(PersistedDefeatFloor(true, 1, 2) == 2);
    CHECK(PersistedDefeatFloor(true, 0, 3) == 3);
}

void ConfiguredFloorAboveTheThresholdIsHonoured()
{
    const char* name = "floor: a configured floor above defeatsToBreak still applies";
    CHECK(PersistedDefeatFloor(true, 4, 2) == 4);
}

void EffectiveCountNeverBelowTheFloor()
{
    const char* name = "count: the session count wins when higher, the floor when the session was lost";
    CHECK(EffectiveDefeatCount(0, 2) == 2);
    CHECK(EffectiveDefeatCount(1, 2) == 2);
    CHECK(EffectiveDefeatCount(5, 2) == 5);
    CHECK(EffectiveDefeatCount(0, 0) == 0);
}

// --- T8: who gets credit for a knockout (characterization of NoteDefeat's
// filters as they shipped, then the explicit credit of the GOLPEARLO beating).

void PlayersOwnPeopleNeverCount()
{
    const char* name = "credit: one of the player's own going down is skipped";
    CHECK(DecideDefeatCredit(true, true, true, false) == DefeatCredit_SkipPlayersOwn);
    CHECK(DecideDefeatCredit(true, false, false, false) == DefeatCredit_SkipPlayersOwn);
}

void UnreadableWinnerIsSkippedWithoutAnExplicitCredit()
{
    const char* name = "credit: an empty lastGuyWhoDefeatedMe is skipped";
    CHECK(DecideDefeatCredit(false, false, false, false) == DefeatCredit_SkipWinnerUnreadable);
}

void SomebodyElsesVictimIsSkipped()
{
    const char* name = "credit: a knockout somebody else caused is skipped";
    CHECK(DecideDefeatCredit(false, true, false, false) == DefeatCredit_SkipNotThePlayer);
}

void ThePlayersVictimCounts()
{
    const char* name = "credit: lastGuyWhoDefeatedMe being the player counts";
    CHECK(DecideDefeatCredit(false, true, true, false) == DefeatCredit_Counts);
}

// GOLPEARLO is the player's own action, so the engine's opinion of who won is
// not asked: the beating used to count only when lastGuyWhoDefeatedMe happened
// to name the player, and a dropped beating still started the cooldown.
void ABeatingIsCreditedToThePlayerWhatTheEngineSays()
{
    const char* name = "credit: an explicit credit counts whatever the engine records";
    CHECK(DecideDefeatCredit(false, false, false, true) == DefeatCredit_Counts);
    CHECK(DecideDefeatCredit(false, true, false, true) == DefeatCredit_Counts);
    CHECK(DecideDefeatCredit(false, true, true, true) == DefeatCredit_Counts);
}

void AnExplicitCreditNeverCountsAnOwnPerson()
{
    const char* name = "credit: an explicit credit still skips the player's own people";
    CHECK(DecideDefeatCredit(true, false, false, true) == DefeatCredit_SkipPlayersOwn);
}

void AnEchoInsideTheDebounceIsNotASecondDefeat()
{
    const char* name = "debounce: inside the window is an echo, at or past it is a new defeat";
    CHECK(IsEchoOfLastDefeat(10.2, 10.0, 0.5));
    CHECK(!IsEchoOfLastDefeat(10.5, 10.0, 0.5));
    CHECK(!IsEchoOfLastDefeat(12.0, 10.0, 0.5));
    // A character never defeated before has the sentinel "long ago" hour.
    CHECK(!IsEchoOfLastDefeat(3.0, -1000.0, 0.5));
    CHECK(!IsEchoOfLastDefeat(0.0, -1000.0, 0.0));
}
} // namespace

void RunDefeatRulesTests()
{
    UnmarkedCharacterHasNoFloor();
    MarkedFloorReachesDefeatsToBreak();
    ConfiguredFloorAboveTheThresholdIsHonoured();
    EffectiveCountNeverBelowTheFloor();
    PlayersOwnPeopleNeverCount();
    UnreadableWinnerIsSkippedWithoutAnExplicitCredit();
    SomebodyElsesVictimIsSkipped();
    ThePlayersVictimCounts();
    ABeatingIsCreditedToThePlayerWhatTheEngineSays();
    AnExplicitCreditNeverCountsAnOwnPerson();
    AnEchoInsideTheDebounceIsNotASecondDefeat();
}
