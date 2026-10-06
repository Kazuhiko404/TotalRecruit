// Tests for src/readout_model.cpp: what the card shows for the facts and verdict the
// decision produced. Pure: plain structs in, a view out.

#include "test_support.h"
#include "readout_model.h"
#include "readout_catalog.h"
#include "chance_rules.h"

using namespace totalrecruit;

namespace
{
const Config kConfig = Config();

ReadoutInput Input(Route route, ReasonKey reason)
{
    ReadoutInput in;
    in.route = route;
    in.core.npcLevel = 20;
    in.core.playerLevel = 20;
    in.verdict.reasonKey = reason;
    return in;
}

void QuestionsThatAreNotAboutAPersonHideTheCard()
{
    const char* name = "hidden: nobody, a corpse, a faker, animals switched off";
    CHECK(!BuildReadout(Input(Route_FreeWorld, Reason_Nobody), kConfig).visible);
    CHECK(!BuildReadout(Input(Route_FreeWorld, Reason_Dead), kConfig).visible);
    CHECK(!BuildReadout(Input(Route_FreeWorld, Reason_Pretending), kConfig).visible);
    CHECK(!BuildReadout(Input(Route_FreeWorld, Reason_AnimalsDisabled), kConfig).visible);
    CHECK(BuildReadout(Input(Route_FreeWorld, Reason_CatsAsk), kConfig).visible);
}

void EachEntryRouteHasItsName()
{
    const char* name = "route: the entry routes map onto the names the player is told";
    CHECK(BuildReadout(Input(Route_Cage, Reason_CatsAsk), kConfig).route == ReadoutRoute_Captivity);
    CHECK(BuildReadout(Input(Route_FreeWorld, Reason_CatsAsk), kConfig).route == ReadoutRoute_Free);
    CHECK(BuildReadout(Input(Route_GrudgeRefusal, Reason_GrudgeNever), kConfig).route == ReadoutRoute_Free);
    CHECK(BuildReadout(Input(Route_Rescue, Reason_RescueJoins), kConfig).route == ReadoutRoute_Rescue);
    CHECK(BuildReadout(Input(Route_Dying, Reason_Dying), kConfig).route == ReadoutRoute_Rescue);
    CHECK(BuildReadout(Input(Route_Unconscious, Reason_UnconsciousWait), kConfig).route == ReadoutRoute_Rescue);
    CHECK(BuildReadout(Input(Route_FieldYield, Reason_FieldYieldNotYet), kConfig).route == ReadoutRoute_FieldYield);
    CHECK(BuildReadout(Input(Route_FieldYield, Reason_FieldYieldJoins), kConfig).route == ReadoutRoute_FieldYield);
    CHECK(BuildReadout(Input(Route_Surrendered, Reason_Surrendered), kConfig).route == ReadoutRoute_Surrender);
}

// The card's peek has no roll key: a route that rolled could not say yes, so its
// "not yet" is only the absence of the dice, not the answer a click would get.
ReadoutInput Unrolled(Route route, ReasonKey reason, int chance)
{
    ReadoutInput in = Input(route, reason);
    in.verdict.rolled = true;
    in.verdict.chancePercent = chance;
    in.rollKeyed = false;
    return in;
}

void ARollThePeekCouldNotMakeShowsTheOdds()
{
    const char* name = "open roll: a rolled 'not yet' without the roll key is the odds, not a no";
    ReadoutView v = BuildReadout(Unrolled(Route_FieldYield, Reason_FieldYieldNotYet, 41), kConfig);
    CHECK(v.primary.id == kLineRollOpen);
    CHECK(v.primary.chance == 41);
    CHECK(v.primary.tone == ReadoutTone_Plain);
    CHECK(v.showChance);
    CHECK(v.chancePct == 41);

    ReadoutInput ally = Unrolled(Route_FreeWorld, Reason_AllyMaybe, 23);
    ally.verdict.currency = Currency_Refused;
    v = BuildReadout(ally, kConfig);
    CHECK(v.primary.id == kLineRollOpen);
    CHECK(v.primary.chance == 23);
    CHECK(v.primary.tone == ReadoutTone_Plain);
}

void AKeyedRollKeepsItsOwnAnswer()
{
    const char* name = "open roll: with the roll key the verdict is the click's own, shown as it is";
    ReadoutInput keyed = Unrolled(Route_FreeWorld, Reason_AllyMaybe, 23);
    keyed.rollKeyed = true;
    CHECK(BuildReadout(keyed, kConfig).primary.id == (int)Reason_AllyMaybe);

    // A route that did not roll is never turned into odds.
    ReadoutInput price = Input(Route_FreeWorld, Reason_CatsAsk);
    CHECK(BuildReadout(price, kConfig).primary.id == (int)Reason_CatsAsk);

    // A rolled cage verdict that fell through to a real way in (a price) keeps it.
    ReadoutInput cage = Unrolled(Route_Cage, Reason_CatsAsk, 30);
    CHECK(BuildReadout(cage, kConfig).primary.id == (int)Reason_CatsAsk);
}

void StarvingIsOnlyAdvisedInYourOwnCage()
{
    const char* name = "field yield: 'dejalo sin comer' only in your cage, 'vencelo otra vez' outside it";
    ReadoutInput open = Unrolled(Route_FieldYield, Reason_FieldYieldNotYet, 41);
    open.rollKeyed = true;
    open.core.inMyCage = false;
    CHECK(BuildReadout(open, kConfig).primary.id == kLineFieldYieldBeatAgain);

    ReadoutInput caged = open;
    caged.core.inMyCage = true;
    CHECK(BuildReadout(caged, kConfig).primary.id == (int)Reason_FieldYieldNotYet);
}

void AnimalsAndPleasOverrideTheRoute()
{
    const char* name = "route: any animal is Animal, a pending plea is Mercy";
    ReadoutInput beast = Input(Route_Cage, Reason_TameHungry);
    beast.core.animal = true;
    CHECK(BuildReadout(beast, kConfig).route == ReadoutRoute_Animal);

    ReadoutInput plea = Input(Route_Cage, Reason_CatsAsk);
    plea.mercyPending = true;
    CHECK(BuildReadout(plea, kConfig).route == ReadoutRoute_Mercy);
}

void StressIsShownWhereTheRollReadsIt()
{
    const char* name = "stress: only in the cage and the field yield, and only with facts";
    ReadoutInput cage = Input(Route_Cage, Reason_CatsAsk);
    cage.core.hunger = 0.0f;
    cage.core.worstDamage = 0.4f;
    cage.core.restedState = 1.0f;
    ReadoutView v = BuildReadout(cage, kConfig);
    CHECK(v.showStress);
    // starved 1.0 * 0.5 + hurt 0.4 * 0.35 + tired 0 = 0.64
    CHECK(v.stressPct == 64);
    CHECK(v.hungerPts == 50);
    CHECK(v.woundsPts == 14);
    CHECK(v.tiredPts == 0);
    CHECK(v.beatenPts == 0);

    // The beatings are the fourth part: 0.64 + 0.2 = 0.84.
    ReadoutInput beaten = cage;
    beaten.core.beatingStress = 0.2f;
    const ReadoutView b = BuildReadout(beaten, kConfig);
    CHECK(b.stressPct == 84);
    CHECK(b.beatenPts == 20);
    CHECK(b.hungerPts + b.woundsPts + b.tiredPts + b.beatenPts == 84);

    ReadoutInput yield = cage;
    yield.route = Route_FieldYield;
    CHECK(BuildReadout(yield, kConfig).showStress);

    ReadoutInput free = cage;
    free.route = Route_FreeWorld;
    CHECK(!BuildReadout(free, kConfig).showStress);
    ReadoutInput rescue = cage;
    rescue.route = Route_Rescue;
    CHECK(!BuildReadout(rescue, kConfig).showStress);

    ReadoutInput noFacts = cage;
    noFacts.haveCore = false;
    CHECK(!BuildReadout(noFacts, kConfig).showStress);

    ReadoutInput beast = cage;
    beast.core.animal = true;
    CHECK(!BuildReadout(beast, kConfig).showStress);
}

void StressPartsAlwaysAddUp()
{
    const char* name = "stress: the four parts add up to the bar, clamped or not";
    for (int h = 0; h <= 6; ++h)
    {
        for (int d = 0; d <= 4; ++d)
        {
            for (int r = 0; r <= 4; ++r)
            {
                for (int b = 0; b <= 4; ++b)
                {
                    ReadoutInput in = Input(Route_Cage, Reason_CatsAsk);
                    in.core.hunger = (float)h * 0.5f;
                    in.core.worstDamage = (float)d * 0.25f;
                    in.core.restedState = (float)r * 0.25f;
                    in.core.beatingStress = (float)b * 0.25f;
                    const ReadoutView v = BuildReadout(in, kConfig);
                    CHECK(v.hungerPts + v.woundsPts + v.tiredPts + v.beatenPts == v.stressPct);
                    CHECK(v.hungerPts >= 0 && v.woundsPts >= 0 && v.tiredPts >= 0 &&
                          v.beatenPts >= 0);
                    CHECK(v.stressPct >= 0 && v.stressPct <= 100);
                }
            }
        }
    }

    Config heavy;
    heavy.submission.stressFromHunger = 1.0f;
    heavy.submission.stressFromWounds = 1.0f;
    ReadoutInput in = Input(Route_Cage, Reason_CatsAsk);
    in.core.hunger = 0.0f;
    in.core.worstDamage = 1.0f;
    in.core.restedState = 0.0f;
    in.core.beatingStress = 0.5f;
    const ReadoutView v = BuildReadout(in, heavy);
    CHECK(v.stressPct == 100);
    CHECK(v.hungerPts + v.woundsPts + v.tiredPts + v.beatenPts == 100);
    CHECK(v.beatenPts > 0);
}

void TheRolledStressIsTheOneShown()
{
    const char* name = "stress: when the verdict rolled, the bar is the stress it rolled with";
    ReadoutInput in = Input(Route_Cage, Reason_Defiant);
    in.core.hunger = 0.0f;
    in.core.worstDamage = 0.2f;
    in.core.restedState = 0.5f;
    in.verdict.rolled = true;
    in.verdict.chanceInputs.stress = 0.3f;
    const ReadoutView v = BuildReadout(in, kConfig);
    CHECK(v.stressPct == 30);
    CHECK(v.hungerPts + v.woundsPts + v.tiredPts + v.beatenPts == 30);
}

void TheChanceIsTheVerdictsChance()
{
    const char* name = "chance: shown exactly when the verdict rolled, and it is the verdict number";
    ReadoutInput in = Input(Route_Cage, Reason_CatsAsk);
    CHECK(!BuildReadout(in, kConfig).showChance);

    in.verdict.rolled = true;
    in.verdict.chancePercent = 58;
    ReadoutView v = BuildReadout(in, kConfig);
    CHECK(v.showChance);
    CHECK(v.chancePct == 58);

    in.verdict.chancePercent = 95;
    CHECK(BuildReadout(in, kConfig).chancePct == 95);
}

void DefeatsAreShownWhereTheyMatter()
{
    const char* name = "defeats: the count against what it takes to break";
    ReadoutInput cage = Input(Route_Cage, Reason_CatsAsk);
    cage.core.defeats = 1;
    ReadoutView v = BuildReadout(cage, kConfig);
    CHECK(v.showDefeats);
    CHECK(v.defeats == 1);
    CHECK(v.defeatsNeeded == 2);

    ReadoutInput cageNone = Input(Route_Cage, Reason_CatsAsk);
    CHECK(BuildReadout(cageNone, kConfig).showDefeats);
    CHECK(BuildReadout(cageNone, kConfig).defeats == 0);

    ReadoutInput freeNone = Input(Route_FreeWorld, Reason_CatsAsk);
    CHECK(!BuildReadout(freeNone, kConfig).showDefeats);
    ReadoutInput freeBeaten = freeNone;
    freeBeaten.core.defeats = 1;
    CHECK(BuildReadout(freeBeaten, kConfig).showDefeats);

    Config off;
    off.submission.countDefeats = false;
    CHECK(!BuildReadout(cage, off).showDefeats);

    ReadoutInput beast = cage;
    beast.core.animal = true;
    CHECK(!BuildReadout(beast, kConfig).showDefeats);
}

// animal-duel-submission D3: a free animal in the open can be beaten into yielding,
// so its card counts the defeats too; an owned or caged one never.
void AFreeAnimalShowsItsDefeatsInTheOpen()
{
    const char* name = "defeats: a free animal in the open, beaten or on the field-yield route";
    ReadoutInput beast = Input(Route_FreeWorld, Reason_TameHungry);
    beast.core.animal = true;
    CHECK(!BuildReadout(beast, kConfig).showDefeats);   // never beaten: nothing to count
    beast.core.defeats = 1;
    ReadoutView v = BuildReadout(beast, kConfig);
    CHECK(v.showDefeats);
    CHECK(v.defeats == 1);
    CHECK(v.defeatsNeeded == 2);
    CHECK(!v.showStress);                                 // the animal card keeps stress hidden

    ReadoutInput yielding = Unrolled(Route_FieldYield, Reason_AnimalYieldNotYet, 38);
    yielding.core.animal = true;
    yielding.core.defeats = 2;
    v = BuildReadout(yielding, kConfig);
    CHECK(v.showDefeats);
    CHECK(v.primary.id == kLineRollOpen);                 // the peek states the odds
    CHECK(v.primary.chance == 38);

    ReadoutInput owned = beast;
    owned.core.animalOwned = true;
    CHECK(!BuildReadout(owned, kConfig).showDefeats);
    ReadoutInput caged = beast;
    caged.core.inMyCage = true;
    CHECK(!BuildReadout(caged, kConfig).showDefeats);
    Config off;
    off.surrenderEnabled = false;
    CHECK(!BuildReadout(beast, off).showDefeats);
}

void AnAnimalShowsItsStomach()
{
    const char* name = "animal: the stomach bar and the line carry fullness and need, in percent";
    ReadoutInput in = Input(Route_FreeWorld, Reason_TameHungry);
    in.core.animal = true;
    in.verdict.tameFullness = 0.56f;
    in.verdict.tameNeeded = 0.80f;
    in.verdict.cats = 24;
    const ReadoutView v = BuildReadout(in, kConfig);
    CHECK(v.showFood);
    CHECK(v.foodPct == 56);
    CHECK(v.foodNeededPct == 80);
    CHECK(v.primary.id == (int)Reason_TameHungry);
    CHECK(v.primary.food == 56);
    CHECK(v.primary.need == 80);
    CHECK(!v.showChance);

    ReadoutInput noBar = Input(Route_FreeWorld, Reason_AnimalOwned);
    noBar.core.animal = true;
    CHECK(!BuildReadout(noBar, kConfig).showFood);
}

void ThePrimaryLineIsTheVerdictsReason()
{
    const char* name = "primary line: the reason with its numbers and its tone";
    ReadoutInput ask = Input(Route_FreeWorld, Reason_CatsAsk);
    ask.verdict.cats = 9570;
    ask.verdict.purse = 4000;
    ReadoutView v = BuildReadout(ask, kConfig);
    CHECK(v.primary.id == (int)Reason_CatsAsk);
    CHECK(v.primary.cats == 9570);
    CHECK(v.primary.purse == 4000);
    CHECK(v.primary.tone == ReadoutTone_Plain);

    ReadoutInput join = Input(Route_FreeWorld, Reason_CatsJoin);
    join.verdict.willJoin = true;
    join.verdict.cats = 3000;
    CHECK(BuildReadout(join, kConfig).primary.tone == ReadoutTone_Good);

    ReadoutInput no = Input(Route_GrudgeRefusal, Reason_GrudgeNever);
    no.verdict.currency = Currency_Refused;
    CHECK(BuildReadout(no, kConfig).primary.tone == ReadoutTone_Refused);
    CHECK(!BuildReadout(no, kConfig).showChance);
    CHECK(!BuildReadout(no, kConfig).showStress);
}

void ABalancedAllysPriceNamesTheFriendship()
{
    const char* name = "primary line: a tiered ally's price says Aliado, a plain bribe does not";
    ReadoutInput join = Input(Route_FreeWorld, Reason_CatsJoin);
    join.verdict.willJoin = true;
    join.verdict.currency = Currency_Cats;
    join.verdict.cats = 1840;
    join.verdict.allyTiered = true;
    join.verdict.allyDiscountPercent = 14;
    ReadoutView v = BuildReadout(join, kConfig);
    CHECK(v.primary.id == kLineAllyPaidJoin);
    CHECK(v.primary.cats == 1840);
    CHECK(v.primary.tone == ReadoutTone_Good);

    ReadoutInput ask = Input(Route_FreeWorld, Reason_CatsAsk);
    ask.verdict.currency = Currency_Cats;
    ask.verdict.cats = 1840;
    ask.verdict.purse = 300;
    ask.verdict.allyTiered = true;
    v = BuildReadout(ask, kConfig);
    CHECK(v.primary.id == kLineAllyPaidAsk);
    CHECK(v.primary.purse == 300);

    ask.verdict.allyTiered = false;
    CHECK(BuildReadout(ask, kConfig).primary.id == (int)Reason_CatsAsk);
}

void ATemporaryOrRefusingAllyShowsItsTierLine()
{
    const char* name = "primary line: temporary is good news, the tier refusals are refusals";
    ReadoutInput temp = Input(Route_FreeWorld, Reason_AllyTemporary);
    temp.verdict.willJoin = true;
    temp.verdict.allyTiered = true;
    temp.verdict.temporary = true;
    ReadoutView v = BuildReadout(temp, kConfig);
    CHECK(v.visible);
    CHECK(v.primary.id == (int)Reason_AllyTemporary);
    CHECK(v.primary.tone == ReadoutTone_Good);

    const ReasonKey refusals[3] = { Reason_AllyLeaderTooWeak, Reason_AllyCruel, Reason_AllyGreedy };
    for (int i = 0; i < 3; ++i)
    {
        ReadoutInput no = Input(Route_FreeWorld, refusals[i]);
        no.verdict.currency = Currency_Refused;
        no.verdict.allyTiered = true;
        v = BuildReadout(no, kConfig);
        CHECK(v.primary.id == (int)refusals[i]);
        CHECK(v.primary.tone == ReadoutTone_Refused);
    }
}

void ATemporaryMemberShowsTheDaysLeft()
{
    const char* name = "temporary member: one good line with the days left, nothing else on the card";
    ReadoutInput in = Input(Route_FreeWorld, Reason_None);
    in.tempMember = true;
    in.tempDaysLeft = 4;
    const ReadoutView v = BuildReadout(in, kConfig);
    CHECK(v.visible);
    CHECK(v.route == ReadoutRoute_Free);
    CHECK(v.primary.id == kLineTempMemberDaysLeft);
    CHECK(v.primary.need == 4);
    CHECK(v.primary.tone == ReadoutTone_Good);
    CHECK(!v.secondary.Present());
    CHECK(!v.showStress);
    CHECK(!v.showChance);
    CHECK(!v.showDefeats);
    CHECK(!v.showFood);
}

void AHeldPrisonerShowsTheTimeHeld()
{
    const char* name = "hold line: the hours held ride their own line, only when a clock is known";
    ReadoutInput in = Input(Route_Cage, Reason_CatsAsk);
    const ReadoutView none = BuildReadout(in, kConfig);
    CHECK(!none.held.Present());

    in.holdKnown = true;
    in.hoursHeld = 2.25f;
    const ReadoutView v = BuildReadout(in, kConfig);
    CHECK(v.visible);
    CHECK(v.held.id == kLineHoldElapsed);
    CHECK(v.held.hours == 2.25f);
    // It is its own line: the reason and the second line are untouched.
    CHECK(v.primary.id == (int)Reason_CatsAsk);
    CHECK(!v.secondary.Present());

    // Cheat answers without the core facts: the clock still shows.
    ReadoutInput cheat = Input(Route_Cage, Reason_CheatWaiting);
    cheat.haveCore = false;
    cheat.holdKnown = true;
    cheat.hoursHeld = 0.01f;
    CHECK(BuildReadout(cheat, kConfig).held.id == kLineHoldElapsed);

    // A hidden card shows nothing, the clock included.
    ReadoutInput hidden = Input(Route_Cage, Reason_Dead);
    hidden.holdKnown = true;
    CHECK(!BuildReadout(hidden, kConfig).held.Present());

    // A member's or a helper's card only counts their days.
    ReadoutInput member = Input(Route_FreeWorld, Reason_None);
    member.tempMember = true;
    member.holdKnown = true;
    CHECK(!BuildReadout(member, kConfig).held.Present());
}

void AHelperShowsTheDaysLeft()
{
    const char* name = "helper: one good line with the days left, nothing else on the card";
    ReadoutInput in = Input(Route_FreeWorld, Reason_None);
    in.helper = true;
    in.helperDaysLeft = 3;
    const ReadoutView v = BuildReadout(in, kConfig);
    CHECK(v.visible);
    CHECK(v.route == ReadoutRoute_Free);
    CHECK(v.primary.id == kLineHelperDaysLeft);
    CHECK(v.primary.need == 3);
    CHECK(v.primary.tone == ReadoutTone_Good);
    CHECK(!v.secondary.Present());
    CHECK(!v.showStress && !v.showChance && !v.showDefeats && !v.showFood);
}

void AChallengeableAllyShowsTheDuelRecord()
{
    const char* name = "duel: a challengeable 80+ ally's card adds the head-to-head record as its second line";
    ReadoutInput in = Input(Route_FreeWorld, Reason_AllyTemporary);
    in.verdict.allyTiered = true;
    in.verdict.allyLevel = 85;
    in.verdict.willJoin = true;
    in.duelRecordKnown = true;
    in.duelWins = 2;
    in.duelLosses = 1;
    in.duelDraws = 0;
    ReadoutView v = BuildReadout(in, kConfig);
    CHECK(v.primary.id == (int)Reason_AllyTemporary);
    CHECK(v.secondary.id == kLineDuelRecord);
    CHECK(v.secondary.food == 2);
    CHECK(v.secondary.need == 1);

    in.duelDraws = 3;
    v = BuildReadout(in, kConfig);
    CHECK(v.secondary.id == kLineDuelRecordWithDraws);
    CHECK(v.secondary.chance == 3);

    in.duelRecordKnown = false;
    v = BuildReadout(in, kConfig);
    CHECK(!v.secondary.Present());
}

void TheDuelRecordCarriesTheNextWinsChance()
{
    const char* name = "duel: the record line carries the next win's chance (Balanced) or 'se une' (Cheat)";
    ReadoutInput in = Input(Route_FreeWorld, Reason_AllyTemporary);
    in.verdict.allyTiered = true;
    in.verdict.allyLevel = 85;
    in.verdict.willJoin = true;
    in.duelRecordKnown = true;
    in.duelWins = 2;
    in.duelLosses = 1;
    in.duelDraws = 0;
    in.duelJoinKnown = true;
    in.duelJoinSure = false;
    in.duelJoinChance = 65;
    ReadoutView v = BuildReadout(in, kConfig);
    CHECK(v.secondary.id == kLineDuelRecordChance);
    CHECK(v.secondary.food == 2 && v.secondary.need == 1 && v.secondary.chance == 65);

    in.duelDraws = 3;
    v = BuildReadout(in, kConfig);
    CHECK(v.secondary.id == kLineDuelRecordDrawsChance);
    CHECK(v.secondary.purse == 3 && v.secondary.chance == 65);

    in.duelJoinSure = true;
    v = BuildReadout(in, kConfig);
    CHECK(v.secondary.id == kLineDuelRecordDrawsSure);
    CHECK(v.secondary.chance == 3);

    in.duelDraws = 0;
    v = BuildReadout(in, kConfig);
    CHECK(v.secondary.id == kLineDuelRecordSure);
    CHECK(v.secondary.food == 2 && v.secondary.need == 1);

    in.duelJoinKnown = false;
    v = BuildReadout(in, kConfig);
    CHECK(v.secondary.id == kLineDuelRecord);
}

void TheDuelRecordSaysHowManyWinsAreMissing()
{
    const char* name = "duel: under the minimum wins the record line says how many wins are missing";
    ReadoutInput in = Input(Route_FreeWorld, Reason_AllyTemporary);
    in.verdict.allyTiered = true;
    in.verdict.allyLevel = 85;
    in.verdict.willJoin = true;
    in.duelRecordKnown = true;
    in.duelWins = 1;
    in.duelLosses = 2;
    in.duelDraws = 0;
    in.duelJoinKnown = true;
    in.duelJoinSure = false;
    in.duelJoinChance = 0;
    in.duelJoinWinsNeeded = 3;
    ReadoutView v = BuildReadout(in, kConfig);
    CHECK(v.secondary.id == kLineDuelRecordNeedWins);
    CHECK(v.secondary.food == 1 && v.secondary.need == 2 && v.secondary.chance == 3);

    in.duelDraws = 4;
    v = BuildReadout(in, kConfig);
    CHECK(v.secondary.id == kLineDuelRecordDrawsNeedWins);
    CHECK(v.secondary.purse == 4 && v.secondary.chance == 3);

    // Even a "sure" flag yields to the missing wins.
    in.duelJoinSure = true;
    v = BuildReadout(in, kConfig);
    CHECK(v.secondary.id == kLineDuelRecordDrawsNeedWins);
}

void TheDuelRecordNeverRidesAMembersCard()
{
    const char* name = "duel: a temporary member's or helper's card stays one line";
    ReadoutInput in = Input(Route_FreeWorld, Reason_None);
    in.tempMember = true;
    in.tempDaysLeft = 2;
    in.duelRecordKnown = true;
    in.duelWins = 1;
    CHECK(!BuildReadout(in, kConfig).secondary.Present());
}

void NoCageWaitLineAnyMore()
{
    const char* name = "cage: no time-left-in-the-cage line: time alone decides nothing (P4)";
    ReadoutInput in = Input(Route_Cage, Reason_CatsAsk);
    ReadoutView v = BuildReadout(in, kConfig);
    CHECK(v.primary.id == (int)Reason_CatsAsk);
    CHECK(!v.secondary.Present());
    ReadoutInput defiant = Input(Route_Cage, Reason_Defiant);
    CHECK(BuildReadout(defiant, kConfig).primary.id == (int)Reason_Defiant);
}

void TheCaredForBonusIsShownWithTheChance()
{
    const char* name = "card: a caged prisoner's cared-for bonus rides with the chance";
    ReadoutInput in = Input(Route_Cage, Reason_CatsAsk);
    in.verdict.rolled = true;
    in.verdict.chancePercent = 70;
    in.verdict.chanceInputs.caredFor = 12;
    ReadoutView v = BuildReadout(in, kConfig);
    CHECK(v.showChance);
    CHECK(v.caredForPct == 12);
    in.verdict.chanceInputs.caredFor = 0;
    CHECK(BuildReadout(in, kConfig).caredForPct == 0);
    ReadoutInput noRoll = Input(Route_Cage, Reason_CatsAsk);
    noRoll.verdict.chanceInputs.caredFor = 12;
    CHECK(BuildReadout(noRoll, kConfig).caredForPct == 0);
}

void StressTermsMakeStressFromState()
{
    const char* name = "stress terms: their clamped sum is StressFromState";
    const Submission sub;
    for (int h = 0; h <= 6; ++h)
    {
        for (int d = 0; d <= 4; ++d)
        {
            for (int b = 0; b <= 4; ++b)
            {
                const float hunger = (float)h * 0.5f;
                const float dmg = (float)d * 0.25f;
                const float beaten = (float)b * 0.25f;
                const StressTerms t = StressTermsFromState(hunger, dmg, 0.5f, beaten, sub);
                float sum = t.hunger + t.wounds + t.tired + t.beaten;
                if (sum > 1.0f) sum = 1.0f;
                CHECK(sum == StressFromState(hunger, dmg, 0.5f, beaten, sub));
            }
        }
    }
    const StressTerms t = StressTermsFromState(0.0f, 1.0f, 0.0f, 0.4f, sub);
    CHECK(t.hunger == 0.5f);
    CHECK(t.wounds == 0.35f);
    CHECK(t.tired == 0.15f);
    CHECK(t.beaten == 0.4f);
    CHECK(StressTermsFromState(3.0f, 0.0f, 1.0f, 3.0f, sub).beaten == 1.0f);
    CHECK(StressTermsFromState(3.0f, 0.0f, 1.0f, -1.0f, sub).beaten == 0.0f);
}
} // namespace

void RunReadoutModelTests()
{
    AFreeAnimalShowsItsDefeatsInTheOpen();
    AChallengeableAllyShowsTheDuelRecord();
    TheDuelRecordNeverRidesAMembersCard();
    TheDuelRecordCarriesTheNextWinsChance();
    TheDuelRecordSaysHowManyWinsAreMissing();
    AHelperShowsTheDaysLeft();
    AHeldPrisonerShowsTheTimeHeld();
    QuestionsThatAreNotAboutAPersonHideTheCard();
    EachEntryRouteHasItsName();
    ARollThePeekCouldNotMakeShowsTheOdds();
    AKeyedRollKeepsItsOwnAnswer();
    StarvingIsOnlyAdvisedInYourOwnCage();
    AnimalsAndPleasOverrideTheRoute();
    StressIsShownWhereTheRollReadsIt();
    StressPartsAlwaysAddUp();
    TheRolledStressIsTheOneShown();
    TheChanceIsTheVerdictsChance();
    DefeatsAreShownWhereTheyMatter();
    AnAnimalShowsItsStomach();
    ThePrimaryLineIsTheVerdictsReason();
    ABalancedAllysPriceNamesTheFriendship();
    ATemporaryOrRefusingAllyShowsItsTierLine();
    ATemporaryMemberShowsTheDaysLeft();
    NoCageWaitLineAnyMore();
    TheCaredForBonusIsShownWithTheChance();
    StressTermsMakeStressFromState();
}
