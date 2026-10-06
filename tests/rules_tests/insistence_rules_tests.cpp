// Tests for src/insistence_rules.cpp: which refused talks count towards insistence
// (R2.4). Only a real decision against the player counts; a state block or a
// "do this first" never does, an accepted talk never does, and an animal never
// accumulates insistence at all.

#include "test_support.h"
#include "insistence_rules.h"
#include "route_decisions.h"

using namespace totalrecruit;

namespace
{
Verdict Refusal(ReasonKey key)
{
    Verdict v;
    v.willJoin = false;
    v.reasonKey = key;
    return v;
}

const ReasonKey kCounting[] = {
    Reason_GrudgeNever,
    Reason_FieldYieldNotYet,
    Reason_AllyMaybe,
    Reason_FactionHonour,
    Reason_FactionGreedy,
    Reason_HonourRefusesBribe,
    Reason_MilitaryLoyalty,
    Reason_Defiant,
    Reason_UniqueZealot,
    Reason_AllyLeaderTooWeak,
    Reason_AllyCruel,
    Reason_AllyGreedy,
};
const int kCountingSize = (int)(sizeof(kCounting) / sizeof(kCounting[0]));

bool InCountingList(ReasonKey key)
{
    for (int i = 0; i < kCountingSize; ++i)
    {
        if (kCounting[i] == key)
        {
            return true;
        }
    }
    return false;
}

void RealRefusalsCount()
{
    const char* name = "insistence: a failed roll or a personality refusal counts";
    for (int i = 0; i < kCountingSize; ++i)
    {
        CHECK(ReasonCountsAsInsistence(kCounting[i]));
        CHECK(CountsAsInsistence(Refusal(kCounting[i]), false));
    }
}

void StateBlocksDoNotCount()
{
    const char* name = "insistence: wounded, out cold, dying, hungry or caged elsewhere never counts";
    CHECK(!ReasonCountsAsInsistence(Reason_Wounded));
    CHECK(!ReasonCountsAsInsistence(Reason_Starving));
    CHECK(!ReasonCountsAsInsistence(Reason_StarvingAndWounded));
    CHECK(!ReasonCountsAsInsistence(Reason_UnconsciousWait));
    CHECK(!ReasonCountsAsInsistence(Reason_UnconsciousDying));
    CHECK(!ReasonCountsAsInsistence(Reason_UnconsciousOwesYou));
    CHECK(!ReasonCountsAsInsistence(Reason_Dying));
    CHECK(!ReasonCountsAsInsistence(Reason_InOthersCage));
    CHECK(!ReasonCountsAsInsistence(Reason_RescueRepays));
    CHECK(!ReasonCountsAsInsistence(Reason_HonourStarving));
}

void InstructionsDoNotCount()
{
    const char* name = "insistence: a 'do this first' (cage, wait, feed, pay) never counts";
    CHECK(!ReasonCountsAsInsistence(Reason_CheatNeedsCage));
    CHECK(!ReasonCountsAsInsistence(Reason_CheatNeedsAnyCage));
    CHECK(!ReasonCountsAsInsistence(Reason_CheatWaiting));
    CHECK(!ReasonCountsAsInsistence(Reason_CatsAsk));
    CHECK(!ReasonCountsAsInsistence(Reason_TameWounded));
    CHECK(!ReasonCountsAsInsistence(Reason_TameHungry));
    CHECK(!ReasonCountsAsInsistence(Reason_TameNotFed));
    CHECK(!ReasonCountsAsInsistence(Reason_AnimalOwned));
}

void LockoutsAndGatesDoNotCount()
{
    const char* name = "insistence: the reject cooldown, insistence itself and the gates never count";
    CHECK(!ReasonCountsAsInsistence(Reason_RejectCooldown));
    CHECK(!ReasonCountsAsInsistence(Reason_Insistence));
    CHECK(!ReasonCountsAsInsistence(Reason_None));
    CHECK(!ReasonCountsAsInsistence(Reason_Nobody));
    CHECK(!ReasonCountsAsInsistence(Reason_AnimalsDisabled));
    CHECK(!ReasonCountsAsInsistence(Reason_Pretending));
    CHECK(!ReasonCountsAsInsistence(Reason_Dead));
    CHECK(!ReasonCountsAsInsistence(Reason_CaptivityDisabled));
}

// The whole table: every key not in the counting list is a no, so a reason added
// later starts out as a no until it is classified.
void EveryOtherReasonDoesNotCount()
{
    const char* name = "insistence: every reason outside the counting list is a no";
    for (int k = 0; k < (int)Reason_Count; ++k)
    {
        const ReasonKey key = (ReasonKey)k;
        CHECK(ReasonCountsAsInsistence(key) == InCountingList(key));
    }
    CHECK(!ReasonCountsAsInsistence(Reason_Count));
    CHECK(!ReasonCountsAsInsistence((ReasonKey)(Reason_Count + 7)));
    CHECK(!ReasonCountsAsInsistence((ReasonKey)-1));
}

void AYesNeverCounts()
{
    const char* name = "insistence: a talk that ends in a yes never counts";
    Verdict v = Refusal(Reason_AllyMaybe);
    v.willJoin = true;
    CHECK(!CountsAsInsistence(v, false));
}

void AnAnimalNeverCounts()
{
    const char* name = "insistence: an animal never accumulates insistence";
    for (int i = 0; i < kCountingSize; ++i)
    {
        CHECK(!CountsAsInsistence(Refusal(kCounting[i]), true));
    }
    Verdict v = Refusal(Reason_GrudgeNever);
    v.isAnimal = true;
    CHECK(!CountsAsInsistence(v, false));
}

// A record left in memory from before the fix must not keep an animal locked out.
void AStoredLockoutDoesNotBlockAnAnimal()
{
    const char* name = "insistence: an animal with a stored lockout is still heard";
    CoreFacts core;
    core.animal = true;
    AccessFact access;
    access.ignoringPlayer = true;
    Verdict v;
    CHECK(!DecideAccessRefusal(core, access, &v));

    core.animal = false;
    CHECK(DecideAccessRefusal(core, access, &v));
    CHECK(v.reasonKey == Reason_Insistence);
}
} // namespace

void RunInsistenceRulesTests()
{
    RealRefusalsCount();
    StateBlocksDoNotCount();
    InstructionsDoNotCount();
    LockoutsAndGatesDoNotCount();
    EveryOtherReasonDoesNotCount();
    AYesNeverCounts();
    AnAnimalNeverCounts();
    AStoredLockoutDoesNotBlockAnAnimal();
}
