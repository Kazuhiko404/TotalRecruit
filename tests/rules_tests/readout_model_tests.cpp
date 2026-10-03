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
    CHECK(BuildReadout(Input(Route_EasyHealthy, Reason_EasyHealthy), kConfig).route == ReadoutRoute_Rescue);
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
    const char* name = "stress: the three parts add up to the bar, clamped or not";
    for (int h = 0; h <= 6; ++h)
    {
        for (int d = 0; d <= 4; ++d)
        {
            for (int r = 0; r <= 4; ++r)
            {
                ReadoutInput in = Input(Route_Cage, Reason_CatsAsk);
                in.core.hunger = (float)h * 0.5f;
                in.core.worstDamage = (float)d * 0.25f;
                in.core.restedState = (float)r * 0.25f;
                const ReadoutView v = BuildReadout(in, kConfig);
                CHECK(v.hungerPts + v.woundsPts + v.tiredPts == v.stressPct);
                CHECK(v.hungerPts >= 0 && v.woundsPts >= 0 && v.tiredPts >= 0);
                CHECK(v.stressPct >= 0 && v.stressPct <= 100);
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
    const ReadoutView v = BuildReadout(in, heavy);
    CHECK(v.stressPct == 100);
    CHECK(v.hungerPts + v.woundsPts + v.tiredPts == 100);
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
    CHECK(v.hungerPts + v.woundsPts + v.tiredPts == 30);
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

void TheEasyWaitIsASecondLineWhileItRuns()
{
    const char* name = "easy wait: second line beside another way in, with the hours left";
    ReadoutInput in = Input(Route_Cage, Reason_CatsAsk);
    in.holdKnown = true;
    in.hoursHeld = 3.0f;
    ReadoutView v = BuildReadout(in, kConfig);
    CHECK(v.primary.id == (int)Reason_CatsAsk);
    CHECK(v.secondary.id == kLineWaitCage);
    CHECK(v.secondary.hours > 2.99f && v.secondary.hours < 3.01f);

    ReadoutInput done = in;
    done.hoursHeld = 6.0f;
    CHECK(!BuildReadout(done, kConfig).secondary.Present());

    ReadoutInput joins = in;
    joins.verdict.willJoin = true;
    CHECK(!BuildReadout(joins, kConfig).secondary.Present());

    ReadoutInput unknown = in;
    unknown.holdKnown = false;
    CHECK(!BuildReadout(unknown, kConfig).secondary.Present());

    ReadoutInput open = in;
    open.route = Route_FreeWorld;
    CHECK(!BuildReadout(open, kConfig).secondary.Present());

    ReadoutInput cheat = in;
    cheat.haveCore = false;
    CHECK(!BuildReadout(cheat, kConfig).secondary.Present());

    ReadoutInput beast = in;
    beast.core.animal = true;
    CHECK(!BuildReadout(beast, kConfig).secondary.Present());

    Config off;
    off.prices.easyHoursCaged = 0.0f;
    CHECK(!BuildReadout(in, off).secondary.Present());
}

void TheWaitIsPromotedWhenTheReasonNamesNoNumber()
{
    const char* name = "easy wait: promoted to the main line when the reason has no number of its own";
    ReadoutInput defiant = Input(Route_Cage, Reason_Defiant);
    defiant.holdKnown = true;
    defiant.hoursHeld = 4.5f;
    ReadoutView v = BuildReadout(defiant, kConfig);
    CHECK(v.primary.id == kLineWaitCage);
    CHECK(v.primary.hours > 1.49f && v.primary.hours < 1.51f);
    CHECK(!v.secondary.Present());

    ReadoutInput zealot = defiant;
    zealot.verdict.reasonKey = Reason_UniqueZealot;
    CHECK(BuildReadout(zealot, kConfig).primary.id == kLineWaitCage);

    // Nothing to promote once the wait is over: the reason stays.
    ReadoutInput over = defiant;
    over.hoursHeld = 6.0f;
    CHECK(BuildReadout(over, kConfig).primary.id == (int)Reason_Defiant);
}

void StressTermsMakeStressFromState()
{
    const char* name = "stress terms: their clamped sum is StressFromState";
    const Submission sub;
    for (int h = 0; h <= 6; ++h)
    {
        for (int d = 0; d <= 4; ++d)
        {
            const float hunger = (float)h * 0.5f;
            const float dmg = (float)d * 0.25f;
            const StressTerms t = StressTermsFromState(hunger, dmg, 0.5f, sub);
            float sum = t.hunger + t.wounds + t.tired;
            if (sum > 1.0f) sum = 1.0f;
            CHECK(sum == StressFromState(hunger, dmg, 0.5f, sub));
        }
    }
    const StressTerms t = StressTermsFromState(0.0f, 1.0f, 0.0f, sub);
    CHECK(t.hunger == 0.5f);
    CHECK(t.wounds == 0.35f);
    CHECK(t.tired == 0.15f);
}
} // namespace

void RunReadoutModelTests()
{
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
    TheEasyWaitIsASecondLineWhileItRuns();
    TheWaitIsPromotedWhenTheReasonNamesNoNumber();
    StressTermsMakeStressFromState();
}
