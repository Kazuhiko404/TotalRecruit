// Tests for src/temp_member_rules.cpp: a temporary squad member's stay -- the expiry,
// the mistreatment strikes and their cooldowns, the warnings and the leave -- and the
// table of who is a member (TempMemberBook).

#include "test_support.h"
#include "temp_member_rules.h"

using namespace totalrecruit;

namespace
{
const TempMembers kConfig = TempMembers();

// A member who joined at hour 1000 for 5 days.
TempMembership Member()
{
    return NewTempMembership(TempExpiryHour(1000.0, 5));
}

TempMemberSignals At(double hours)
{
    TempMemberSignals s;
    s.nowHours = hours;
    return s;
}

TempMemberSignals Attacked(double hours)
{
    TempMemberSignals s = At(hours);
    s.attackedBySquad = true;
    return s;
}

TempMemberSignals Hungry(double hours)
{
    TempMemberSignals s = At(hours);
    s.hungry = true;
    return s;
}

TempMemberSignals KnockedOut(double hours, bool bySquad)
{
    TempMemberSignals s = At(hours);
    s.knockedOut = true;
    s.knockedOutBySquad = bySquad;
    return s;
}

CharKey Key(const char* id)
{
    return CharKey::FromId(id);
}

void TheStayEndsDaysTimes24HoursAfterJoining()
{
    const char* name = "expiry: now + days * 24, rounded up; days under 1 count as 1";
    CHECK(TempExpiryHour(1000.0, 5) == 1120);
    CHECK(TempExpiryHour(1000.2, 3) == 1073);
    CHECK(TempExpiryHour(10.0, 0) == 34);
    CHECK(TempExpiryHour(10.0, -4) == 34);
    CHECK(TempExpiryHour(0.0, 1) == 24);
    CHECK(NewTempMembership(1120).expiryHour == 1120);
    CHECK(NewTempMembership(1120).IsMember());
    CHECK(NewTempMembership(1120).strikes == 0);
    CHECK(!TempMembership().IsMember());
}

void DaysLeftRoundUp()
{
    const char* name = "days left: rounded up, 0 once the time is up";
    CHECK(TempDaysLeft(1120, 1000.0) == 5);
    CHECK(TempDaysLeft(1120, 1001.0) == 5);
    CHECK(TempDaysLeft(1120, 1096.0) == 1);
    CHECK(TempDaysLeft(1120, 1119.5) == 1);
    CHECK(TempDaysLeft(1120, 1120.0) == 0);
    CHECK(TempDaysLeft(1120, 1500.0) == 0);
    CHECK(TempDaysLeft(0, 10.0) == 0);
}

void HungerIsTheEngineOrTheThreshold()
{
    const char* name = "hungry: the engine's starving, or the stomach under the threshold";
    CHECK(IsTempMemberHungry(0.9f, true, 0.25f));
    CHECK(IsTempMemberHungry(0.2f, false, 0.25f));
    CHECK(!IsTempMemberHungry(0.25f, false, 0.25f));
    CHECK(!IsTempMemberHungry(0.8f, false, 0.25f));
    // Threshold 0: only the engine decides.
    CHECK(!IsTempMemberHungry(0.0f, false, 0.0f));
    CHECK(IsTempMemberHungry(0.0f, true, 0.0f));
}

void ANormalDayChangesNothing()
{
    const char* name = "step: fed, unhurt, before the warning window: nothing happens";
    TempMembership m = Member();
    const TempMemberStep step = StepTempMember(&m, At(1010.0), kConfig);
    CHECK(!step.Struck());
    CHECK(!step.warn);
    CHECK(!step.expiryWarn);
    CHECK(step.leave == TempLeave_Stay);
    CHECK(m.strikes == 0);
}

void AnAttackBySquadIsAStrikeOncePerCooldown()
{
    const char* name = "attacked: one strike, then none until the cooldown has run";
    TempMembership m = Member();
    TempMemberStep step = StepTempMember(&m, Attacked(1010.0), kConfig);
    CHECK(step.StruckBy(TempStrike_Attacked));
    CHECK(m.strikes == 1);
    // Still being hit two seconds later and an hour later: same beating.
    step = StepTempMember(&m, Attacked(1010.001), kConfig);
    CHECK(!step.Struck());
    step = StepTempMember(&m, Attacked(1021.9), kConfig);
    CHECK(!step.Struck());
    CHECK(m.strikes == 1);
    // After the cooldown (12 h) another attack strikes again.
    step = StepTempMember(&m, Attacked(1022.0), kConfig);
    CHECK(step.StruckBy(TempStrike_Attacked));
    CHECK(m.strikes == 2);
}

void HungerStrikesAfterTheStreak()
{
    const char* name = "hunger: a strike only after hungryHours in a row; eating resets the streak";
    TempMembership m = Member();
    TempMemberStep step = StepTempMember(&m, Hungry(1000.0), kConfig);
    CHECK(!step.Struck());
    step = StepTempMember(&m, Hungry(1011.9), kConfig);
    CHECK(!step.Struck());
    // They ate: the streak starts over.
    step = StepTempMember(&m, At(1012.0), kConfig);
    CHECK(!step.Struck());
    step = StepTempMember(&m, Hungry(1013.0), kConfig);
    CHECK(!step.Struck());
    step = StepTempMember(&m, Hungry(1024.9), kConfig);
    CHECK(!step.Struck());
    step = StepTempMember(&m, Hungry(1025.0), kConfig);
    CHECK(step.StruckBy(TempStrike_Hunger));
    CHECK(m.strikes == 1);
    // Still hungry: the next hunger strike waits for the cooldown.
    step = StepTempMember(&m, Hungry(1030.0), kConfig);
    CHECK(!step.Struck());
    step = StepTempMember(&m, Hungry(1037.0), kConfig);
    CHECK(step.StruckBy(TempStrike_Hunger));
    CHECK(m.strikes == 2);
}

void AKnockoutStrikesOnTheEdgeOnlyWhenTheSquadDidIt()
{
    const char* name = "knockout: strikes on the edge into it, only when one of the player's put them down";
    TempMembership m = Member();
    // Put down by an enemy: no strike, and lying there is not a new knockout.
    TempMemberStep step = StepTempMember(&m, KnockedOut(1010.0, false), kConfig);
    CHECK(!step.Struck());
    step = StepTempMember(&m, KnockedOut(1010.01, true), kConfig);
    CHECK(!step.Struck());
    // Up again, then put down by the squad: a strike.
    step = StepTempMember(&m, At(1011.0), kConfig);
    step = StepTempMember(&m, KnockedOut(1011.1, true), kConfig);
    CHECK(step.StruckBy(TempStrike_KnockedOut));
    CHECK(m.strikes == 1);
    // Still down: no second strike.
    step = StepTempMember(&m, KnockedOut(1011.2, true), kConfig);
    CHECK(!step.Struck());
    // Up and down again inside the cooldown: no strike either.
    step = StepTempMember(&m, At(1012.0), kConfig);
    step = StepTempMember(&m, KnockedOut(1013.0, true), kConfig);
    CHECK(!step.Struck());
    CHECK(m.strikes == 1);
}

void KindsHaveTheirOwnCooldowns()
{
    const char* name = "cooldown: per kind -- a beating and a knockout in the same hour are two strikes";
    TempMembership m = Member();
    TempMemberSignals s = KnockedOut(1010.0, true);
    s.attackedBySquad = true;
    const TempMemberStep step = StepTempMember(&m, s, kConfig);
    CHECK(step.StruckBy(TempStrike_Attacked));
    CHECK(step.StruckBy(TempStrike_KnockedOut));
    CHECK(!step.StruckBy(TempStrike_Hunger));
    CHECK(m.strikes == 2);
}

void TheStrikeBeforeTheLastComesWithAWarning()
{
    const char* name = "warning: once, on the strike that brings them to maxStrikes - 1";
    TempMembership m = Member();
    TempMemberStep step = StepTempMember(&m, Attacked(1010.0), kConfig);
    CHECK(step.Struck() && !step.warn);
    step = StepTempMember(&m, Attacked(1030.0), kConfig);
    CHECK(step.Struck() && step.warn);
    CHECK(m.warned);
    CHECK(step.leave == TempLeave_Stay);
}

void TheLastStrikeMakesThemLeave()
{
    const char* name = "leave: maxStrikes strikes and they go, mistreated, before their time";
    TempMembership m = Member();
    StepTempMember(&m, Attacked(1010.0), kConfig);
    StepTempMember(&m, Attacked(1030.0), kConfig);
    const TempMemberStep step = StepTempMember(&m, Attacked(1050.0), kConfig);
    CHECK(m.strikes == 3);
    CHECK(step.leave == TempLeave_Mistreated);
    CHECK(!step.warn);
}

void MaxStrikesIsConfig()
{
    const char* name = "leave: maxStrikes comes from the config (1 = the first strike is the last)";
    TempMembers one;
    one.maxStrikes = 1;
    TempMembership m = Member();
    const TempMemberStep step = StepTempMember(&m, Attacked(1010.0), one);
    CHECK(step.leave == TempLeave_Mistreated);
    CHECK(!step.warn);
}

void TimeUpMakesThemLeave()
{
    const char* name = "leave: at the expiry hour they go; before it they stay";
    TempMembership m = Member();
    CHECK(StepTempMember(&m, At(1119.9), kConfig).leave == TempLeave_Stay);
    CHECK(StepTempMember(&m, At(1120.0), kConfig).leave == TempLeave_Expired);
}

void MistreatmentWinsOverExpiry()
{
    const char* name = "leave: the last strike at the expiry hour reads as mistreated";
    TempMembership m = Member();
    m.strikes = 2;
    m.warned = true;
    CHECK(StepTempMember(&m, Attacked(1120.0), kConfig).leave == TempLeave_Mistreated);
}

void TheExpiryWarningComesOnceBeforeTheEnd()
{
    const char* name = "expiry warning: once, expiryWarningHours before the end";
    TempMembership m = Member();
    CHECK(!StepTempMember(&m, At(1107.9), kConfig).expiryWarn);
    CHECK(StepTempMember(&m, At(1108.0), kConfig).expiryWarn);
    CHECK(m.expiryWarned);
    CHECK(!StepTempMember(&m, At(1110.0), kConfig).expiryWarn);
    // Leaving says goodbye, not "soon".
    TempMembership late = Member();
    const TempMemberStep step = StepTempMember(&late, At(1125.0), kConfig);
    CHECK(step.leave == TempLeave_Expired);
    CHECK(!step.expiryWarn);
}

void ARestoredCountDoesNotWarnAgain()
{
    const char* name = "reload: a member restored at maxStrikes - 1 is not warned again until the next strike leaves";
    TempMembership m = Member();
    m.strikes = 2;
    TempMemberStep step = StepTempMember(&m, At(1010.0), kConfig);
    CHECK(!step.warn);
    step = StepTempMember(&m, Attacked(1011.0), kConfig);
    CHECK(step.leave == TempLeave_Mistreated);
}

void SavedValuesAreSanitised()
{
    const char* name = "save: negative saved values read as none";
    CHECK(SanitiseSavedTempValue(-5) == 0);
    CHECK(SanitiseSavedTempValue(0) == 0);
    CHECK(SanitiseSavedTempValue(1120) == 1120);
}

void TheBookKnowsItsMembers()
{
    const char* name = "book: join, find, update, remove";
    TempMemberBook book;
    CHECK(book.Find(Key("a")) == NULL);
    book.Join(Key("a"), 1120);
    book.Join(Key("b"), 2000);
    CHECK(book.Count() == 2);
    CHECK(book.Find(Key("a")) != NULL && book.Find(Key("a"))->expiryHour == 1120);
    TempMembership m = *book.Find(Key("a"));
    m.strikes = 2;
    book.Update(Key("a"), m);
    CHECK(book.Find(Key("a"))->strikes == 2);
    // Update never creates a member.
    book.Update(Key("c"), m);
    CHECK(book.Find(Key("c")) == NULL);
    book.Remove(Key("a"));
    CHECK(book.Find(Key("a")) == NULL);
    CHECK(book.Members().size() == 1 && book.Members()[0] == Key("b"));
    // Joining again starts the stay over.
    book.Update(Key("b"), m);
    book.Join(Key("b"), 3000);
    CHECK(book.Find(Key("b"))->expiryHour == 3000 && book.Find(Key("b"))->strikes == 0);
    book.Clear();
    CHECK(book.Count() == 0);
}

void TheBookRoundTripsThroughTheSave()
{
    const char* name = "book: saved expiry and strikes, 0 for non-members, restored in either order";
    TempMemberBook book;
    book.Join(Key("a"), 1120);
    TempMembership m = *book.Find(Key("a"));
    m.strikes = 1;
    book.Update(Key("a"), m);
    CHECK(book.SavedExpiry(Key("a")) == 1120);
    CHECK(book.SavedStrikes(Key("a")) == 1);
    CHECK(book.SavedExpiry(Key("z")) == 0);
    CHECK(book.SavedStrikes(Key("z")) == 0);

    TempMemberBook loaded;
    loaded.RestoreExpiry(Key("a"), 1120);
    loaded.RestoreStrikes(Key("a"), 1);
    // Strikes first, then the expiry.
    loaded.RestoreStrikes(Key("b"), 2);
    CHECK(loaded.Find(Key("b")) == NULL);
    loaded.RestoreExpiry(Key("b"), 900);
    CHECK(loaded.Find(Key("a")) != NULL && loaded.Find(Key("a"))->strikes == 1);
    CHECK(loaded.Find(Key("b")) != NULL && loaded.Find(Key("b"))->strikes == 2);
    CHECK(loaded.Find(Key("b"))->expiryHour == 900);
    // A character whose save holds 0 (never a member, or left) is no member.
    loaded.RestoreExpiry(Key("c"), 0);
    loaded.RestoreStrikes(Key("c"), 0);
    loaded.RestoreExpiry(Key("d"), -3);
    CHECK(loaded.Find(Key("c")) == NULL);
    CHECK(loaded.Find(Key("d")) == NULL);
    CHECK(loaded.Count() == 2);
}
} // namespace

void RunTempMemberRulesTests()
{
    TheStayEndsDaysTimes24HoursAfterJoining();
    DaysLeftRoundUp();
    HungerIsTheEngineOrTheThreshold();
    ANormalDayChangesNothing();
    AnAttackBySquadIsAStrikeOncePerCooldown();
    HungerStrikesAfterTheStreak();
    AKnockoutStrikesOnTheEdgeOnlyWhenTheSquadDidIt();
    KindsHaveTheirOwnCooldowns();
    TheStrikeBeforeTheLastComesWithAWarning();
    TheLastStrikeMakesThemLeave();
    MaxStrikesIsConfig();
    TimeUpMakesThemLeave();
    MistreatmentWinsOverExpiry();
    TheExpiryWarningComesOnceBeforeTheEnd();
    ARestoredCountDoesNotWarnAgain();
    SavedValuesAreSanitised();
    TheBookKnowsItsMembers();
    TheBookRoundTripsThroughTheSave();
}
