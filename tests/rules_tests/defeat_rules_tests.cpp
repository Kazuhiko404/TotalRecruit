// Tests for src/defeat_rules.cpp: who gets credit for a knockout.

#include "test_support.h"
#include "defeat_rules.h"

using namespace totalrecruit;

namespace
{
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
} // namespace

void RunDefeatRulesTests()
{
    PlayersOwnPeopleNeverCount();
    UnreadableWinnerIsSkippedWithoutAnExplicitCredit();
    SomebodyElsesVictimIsSkipped();
    ThePlayersVictimCounts();
    ABeatingIsCreditedToThePlayerWhatTheEngineSays();
    AnExplicitCreditNeverCountsAnOwnPerson();
}
