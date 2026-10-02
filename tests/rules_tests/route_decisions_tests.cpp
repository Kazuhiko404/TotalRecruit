// Tests for src/route_*.cpp: what each route of Evaluate decides, from plain facts.
//
// Evaluate used to be one engine-bound function and none of this could be tested.
// Every expectation below was read off that function before it was split, branch
// by branch (the order, the strict and non-strict comparisons, which input each
// branch ignores), so these are characterization tests of the shipped behaviour.
// Where the old code has a branch no input can reach, that is said in a comment
// instead of being tested.
//
// A roll is seeded from the character, so the tests do not depend on the hash:
// they search for a key that rolls the way the case needs (KeyThatJoins /
// KeyThatFails) at the percent the case is expected to produce.

#include "test_support.h"
#include "route_decisions.h"
#include "route_rules.h"
#include "chance_rules.h"
#include "animal_rules.h"
#include "price_rules.h"

using namespace totalrecruit;

namespace
{
const Config kConfig = Config();

CoreFacts Core()
{
    CoreFacts f;
    f.npcLevel = 20;
    f.playerLevel = 20;
    f.temperament = Temperament_Normal;
    f.archetype = Archetype_Bandit;
    return f;
}

// A key whose stable roll at `percent` comes out as asked.
RollFact KeyRolling(bool joins, int percent)
{
    RollFact roll;
    roll.haveKey = true;
    for (unsigned int i = 1; i < 100000; ++i)
    {
        roll.key = CharKey(i, 1u);
        if (RollsToJoin(true, roll.key, percent) == joins)
        {
            return roll;
        }
    }
    roll.haveKey = false;
    return roll;
}

PurseFact Purse(int cats)
{
    PurseFact p;
    p.known = true;
    p.cats = cats;
    return p;
}

RelationFact Relation(float value)
{
    RelationFact r;
    r.known = true;
    r.value = value;
    return r;
}

// ---------------------------------------------------------------------- gate

void AnimalsSwitchedOffStopsAnAnimal()
{
    const char* name = "gate: an animal with animals switched off is stopped, still marked as an animal";
    CoreFacts core = Core();
    core.animal = true;
    core.starving = true;
    Config config;
    config.animalsEnabled = false;
    Verdict v;
    CHECK(DecideGate(core, config, &v));
    CHECK(!v.willJoin);
    CHECK(v.isAnimal);
    CHECK(!v.wantsFood);
    CHECK(v.reasonKey == Reason_AnimalsDisabled);
}

void UnreadableTraitsFallBackToAPlainRecruit()
{
    const char* name = "gate: when the engine cannot say who they are the mod recruits plainly";
    CoreFacts core = Core();
    core.traitsReadable = false;
    core.animal = true;
    Verdict v;
    CHECK(DecideGate(core, kConfig, &v));
    CHECK(v.willJoin);
    CHECK(v.currency == Currency_None);
    CHECK(v.isAnimal);
    CHECK(v.reasonKey == Reason_UnreadableTraits);
}

void AFakerAndACorpseAreNotRecruitable()
{
    const char* name = "gate: a faker and a corpse stop the question, carrying whether they are starving";
    CoreFacts core = Core();
    core.starving = true;
    core.pretending = true;
    Verdict v;
    CHECK(DecideGate(core, kConfig, &v));
    CHECK(!v.willJoin);
    CHECK(v.wantsFood);
    CHECK(v.reasonKey == Reason_Pretending);

    core.pretending = false;
    core.corpse = true;
    CHECK(DecideGate(core, kConfig, &v));
    CHECK(v.reasonKey == Reason_Dead);
    CHECK(v.wantsFood);
}

void TheGateChecksInTheShippedOrder()
{
    const char* name = "gate: animals off, then unreadable, then faker, then corpse";
    CoreFacts core = Core();
    core.animal = true;
    core.traitsReadable = false;
    core.pretending = true;
    core.corpse = true;
    Config off;
    off.animalsEnabled = false;
    Verdict v;
    CHECK(DecideGate(core, off, &v) && v.reasonKey == Reason_AnimalsDisabled);
    CHECK(DecideGate(core, kConfig, &v) && v.reasonKey == Reason_UnreadableTraits);
    core.traitsReadable = true;
    CHECK(DecideGate(core, kConfig, &v) && v.reasonKey == Reason_Pretending);
    core.pretending = false;
    CHECK(DecideGate(core, kConfig, &v) && v.reasonKey == Reason_Dead);
}

void AnOrdinaryCharacterPassesTheGate()
{
    const char* name = "gate: an ordinary character, and an animal with animals on, pass";
    Verdict v;
    CHECK(!DecideGate(Core(), kConfig, &v));
    CoreFacts animal = Core();
    animal.animal = true;
    CHECK(!DecideGate(animal, kConfig, &v));
}

// --------------------------------------------------------------------- cheat

void CheatWithoutACageJoinsAtOnce()
{
    const char* name = "cheat: with the cage switched off they simply join";
    Config config;
    config.cheatNeedsCage = false;
    const Verdict v = DecideCheat(CheatFacts(), config);
    CHECK(v.willJoin);
    CHECK(v.currency == Currency_None);
    CHECK(v.reasonKey == Reason_CheatJoinsFreely);
}

void CheatAsksForTheCage()
{
    const char* name = "cheat: outside your cage the answer is the cage";
    CheatFacts facts;
    facts.inMyCage = false;
    facts.hoursHeld = 99.0f;
    const Verdict v = DecideCheat(facts, kConfig);
    CHECK(!v.willJoin);
    CHECK(v.currency == Currency_Refused);
    CHECK(v.reasonKey == Reason_CheatNeedsCage);
}

void CheatWaitsAMinuteThenJoins()
{
    const char* name = "cheat: in the cage they wait cheatHoursCaged and then join";
    CheatFacts facts;
    facts.inMyCage = true;
    facts.hoursHeld = kConfig.cheatHoursCaged - 0.001f;
    Verdict v = DecideCheat(facts, kConfig);
    CHECK(!v.willJoin);
    CHECK(v.currency == Currency_Refused);
    CHECK(v.reasonKey == Reason_CheatWaiting);

    facts.hoursHeld = kConfig.cheatHoursCaged;
    v = DecideCheat(facts, kConfig);
    CHECK(v.willJoin);
    CHECK(v.currency == Currency_None);
    CHECK(v.reasonKey == Reason_CheatJoins);
}

// -------------------------------------------------------------------- rescue

void OutColdAndOwingWaitsForThemToWake()
{
    const char* name = "unconscious: someone who owes you their life will say so when they wake";
    CoreFacts core = Core();
    core.unconscious = true;
    core.rememberedAsSaviour = true;
    Verdict v = DecideUnconscious(core, kConfig);
    CHECK(!v.willJoin);
    CHECK(v.viaRescue);
    CHECK(v.currency == Currency_None);
    CHECK(v.reasonKey == Reason_UnconsciousOwesYou);
}

void OutColdAndDyingWantsCare()
{
    const char* name = "unconscious: dying and not yet saved, the price is care";
    CoreFacts core = Core();
    core.unconscious = true;
    core.dying = true;
    Verdict v = DecideUnconscious(core, kConfig);
    CHECK(v.viaRescue);
    CHECK(v.currency == Currency_Care);
    CHECK(v.reasonKey == Reason_UnconsciousDying);

    // The saviour memory only counts with the rescue route switched on.
    core.rememberedAsSaviour = true;
    Config off;
    off.rescueEnabled = false;
    v = DecideUnconscious(core, off);
    CHECK(v.reasonKey == Reason_UnconsciousDying);
}

void OutColdAndNothingElseJustWaits()
{
    const char* name = "unconscious: nobody agrees to anything while out cold";
    CoreFacts core = Core();
    core.unconscious = true;
    const Verdict v = DecideUnconscious(core, kConfig);
    CHECK(!v.willJoin);
    CHECK(!v.viaRescue);
    CHECK(v.currency == Currency_None);
    CHECK(v.reasonKey == Reason_UnconsciousWait);
}

void DyingAndEasyHealthyAreRescueRoutes()
{
    const char* name = "rescue: dying wants care first, a patched-up prisoner joins";
    CoreFacts core = Core();
    Verdict d = DecideDying(core);
    CHECK(!d.willJoin && d.viaRescue && d.currency == Currency_Care && d.reasonKey == Reason_Dying);
    Verdict e = DecideEasyHealthy(core);
    CHECK(e.willJoin && e.viaRescue && e.currency == Currency_Care && e.reasonKey == Reason_EasyHealthy);
}

void ASavedLifeRollsAtTheSavedLifeChance()
{
    const char* name = "rescue: the chance is level + relation + debt + how much was healed";
    CoreFacts core = Core();
    core.health = 1.0f;
    RescueExtra extra;
    extra.relation = Relation(80.0f);
    extra.roll = KeyRolling(true, 95);
    // 45 (even levels) + 20 (a quarter of 80) + 0 (personality) + situation
    // 25 + (int)(1.0 * 30) = 55 -> 120, held to the ceiling of 95.
    Verdict v = DecideRescue(core, extra, kConfig);
    CHECK(v.rolled);
    CHECK(v.chancePercent == 95);
    CHECK(v.chanceInputs.situation == 55);
    CHECK(v.chanceInputs.haveRelation);
    CHECK(v.viaRescue);
    CHECK(v.willJoin);
    CHECK(v.currency == Currency_None);
    CHECK(v.reasonKey == Reason_RescueJoins);
    CHECK(!v.owesGift);
}

void ARescuedOneWhoCannotFollowPaysItForward()
{
    const char* name = "rescue: a failed roll is a refusal that owes a gift, never a charge";
    CoreFacts core = Core();
    RescueExtra extra;
    extra.roll = KeyRolling(false, 95);
    const Verdict v = DecideRescue(core, extra, kConfig);
    CHECK(!v.willJoin);
    CHECK(v.currency == Currency_Refused);
    CHECK(v.owesGift);
    CHECK(v.viaRescue);
    CHECK(v.reasonKey == Reason_RescueRepays);
}

void HealingThemMoreArguesMore()
{
    const char* name = "rescue: the healed fraction scales the situation bonus";
    CoreFacts core = Core();
    RescueExtra extra;
    core.health = 0.0f;
    CHECK(DecideRescue(core, extra, kConfig).chanceInputs.situation == 25);
    core.health = 0.5f;
    CHECK(DecideRescue(core, extra, kConfig).chanceInputs.situation == 40);
    core.health = 3.0f;      // over-full still counts as whole
    CHECK(DecideRescue(core, extra, kConfig).chanceInputs.situation == 55);
}

void WithoutAKeyARescueIsNotAGiven()
{
    const char* name = "rescue: with no key to seed the roll a sub-100 chance is a no";
    CoreFacts core = Core();
    RescueExtra extra;                    // haveKey false
    const Verdict v = DecideRescue(core, extra, kConfig);
    CHECK(v.chancePercent < 100);
    CHECK(!v.willJoin);
}

// --------------------------------------------------------------------- yield

void FieldYieldRollsAtTheBeatingChance()
{
    const char* name = "field yield: level + defeats + stress + the yield bonus";
    CoreFacts core = Core();
    core.defeats = 2;
    YieldExtra extra;
    extra.roll = KeyRolling(true, 79);
    // 45 + 24 (two defeats at 12) + 0 (fed, unhurt, rested) + 10 = 79
    const Verdict v = DecideFieldYield(core, extra, kConfig);
    CHECK(v.rolled);
    CHECK(v.chancePercent == 79);
    CHECK(v.willJoin);
    CHECK(v.currency == Currency_Submission);
    CHECK(v.reasonKey == Reason_FieldYieldJoins);
}

void FieldYieldNotYetNamesTheWayOut()
{
    const char* name = "field yield: a failed roll is submission, not yet";
    CoreFacts core = Core();
    core.defeats = 2;
    YieldExtra extra;
    extra.roll = KeyRolling(false, 79);
    const Verdict v = DecideFieldYield(core, extra, kConfig);
    CHECK(!v.willJoin);
    CHECK(v.currency == Currency_Submission);
    CHECK(v.reasonKey == Reason_FieldYieldNotYet);
}

void FieldYieldReadsStressAndUniqueButNotTheGrudge()
{
    const char* name = "field yield: stress and uniqueness count, the grudge does not";
    CoreFacts core = Core();
    core.defeats = 2;
    YieldExtra extra;
    const int base = DecideFieldYield(core, extra, kConfig).chancePercent;

    CoreFacts starved = core;
    starved.hunger = 0.0f;            // an empty stomach is half the stress bar
    CHECK(DecideFieldYield(starved, extra, kConfig).chancePercent > base);

    CoreFacts unique = core;
    unique.unique = true;
    CHECK(DecideFieldYield(unique, extra, kConfig).chancePercent == base - kConfig.chance.uniquePenalty);

    CoreFacts grudging = core;
    grudging.grudge = true;
    CHECK(DecideFieldYield(grudging, extra, kConfig).chancePercent == base);
}

void GrudgeAndSurrenderAreFixedAnswers()
{
    const char* name = "yield: the grudge is the one honest no, a surrender joins";
    const CoreFacts core = Core();
    const Verdict g = DecideGrudgeRefusal(core);
    CHECK(!g.willJoin && g.currency == Currency_Refused && g.reasonKey == Reason_GrudgeNever);
    const Verdict s = DecideSurrendered(core);
    CHECK(s.willJoin && s.currency == Currency_Submission && s.reasonKey == Reason_Surrendered);
}

// -------------------------------------------------------------------- animal

AnimalExtra Beast()
{
    AnimalExtra a;
    a.maturity = 1.0f;
    return a;
}

CoreFacts AnimalCore(float hunger)
{
    CoreFacts core = Core();
    core.animal = true;
    core.hunger = hunger;
    return core;
}

void AnOwnedAnimalIsOnlyRefusedInTheOpen()
{
    const char* name = "animal: an owned animal is refused in the open and tamed once caged";
    AnimalExtra extra = Beast();
    extra.belongsToSomeone = true;
    CoreFacts open = AnimalCore(3.0f);
    Verdict v = DecideAnimal(open, extra, kConfig);
    CHECK(!v.willJoin);
    CHECK(v.currency == Currency_Refused);
    CHECK(v.isAnimal);
    CHECK(v.reasonKey == Reason_AnimalOwned);

    CoreFacts caged = AnimalCore(3.0f);
    caged.inMyCage = true;
    v = DecideAnimal(caged, extra, kConfig);
    CHECK(v.willJoin);
    CHECK(v.reasonKey == Reason_TameReady);
}

void AFullAnimalFollowsYou()
{
    const char* name = "animal: fed past the bar it follows you, and no longer wants food";
    // Bar for a grown level-20 animal: 0.80 + 0.15 = 0.95; a full stomach is 1.0.
    const Verdict v = DecideAnimal(AnimalCore(3.0f), Beast(), kConfig);
    CHECK(v.willJoin);
    CHECK(!v.wantsFood);
    CHECK(v.currency == Currency_Care);
    CHECK(v.reasonKey == Reason_TameReady);
}

void TheTamingBarTravelsOnTheVerdict()
{
    const char* name = "animal: the verdict carries the fullness and the bar it was judged against";
    // Half a stomach against a 0.95 bar.
    Verdict v = DecideAnimal(AnimalCore(1.5f), Beast(), kConfig);
    CHECK(v.tameFullness > 0.499f && v.tameFullness < 0.501f);
    CHECK(v.tameNeeded > 0.949f && v.tameNeeded < 0.951f);

    // Fed past it: both are still there, the bar is cleared.
    v = DecideAnimal(AnimalCore(3.0f), Beast(), kConfig);
    CHECK(v.tameFullness >= v.tameNeeded);

    // Owned and refused: no stomach was judged, so no bar.
    AnimalExtra extra = Beast();
    extra.belongsToSomeone = true;
    v = DecideAnimal(AnimalCore(3.0f), extra, kConfig);
    CHECK(v.tameFullness < 0.0f && v.tameNeeded < 0.0f);
}

void AHungryAnimalSaysHowMuchIsMissing()
{
    const char* name = "animal: short of the bar it wants food and names the gap in percent";
    // Half a stomach against a 0.95 bar: 45% missing.
    const Verdict v = DecideAnimal(AnimalCore(1.5f), Beast(), kConfig);
    CHECK(!v.willJoin);
    CHECK(v.wantsFood);
    CHECK(v.currency == Currency_Care);
    CHECK(v.reasonKey == Reason_TameHungry);
    CHECK(v.cats == 45);
}

void AWoundedAnimalWantsHealingEvenWhenFull()
{
    const char* name = "animal: a wound outranks a full stomach, and carries no gap";
    CoreFacts core = AnimalCore(3.0f);
    core.wounded = true;
    Verdict v = DecideAnimal(core, Beast(), kConfig);
    CHECK(!v.willJoin);
    CHECK(!v.wantsFood);
    CHECK(v.reasonKey == Reason_TameWounded);
    CHECK(v.cats == 0);

    core.hunger = 1.5f;
    v = DecideAnimal(core, Beast(), kConfig);
    CHECK(v.wantsFood);
    CHECK(v.reasonKey == Reason_TameWounded);
}

void TemperamentMovesTheBarOnlyWhenItMatters()
{
    const char* name = "animal: a stubborn temperament raises the bar, unless personality is switched off";
    CoreFacts core = AnimalCore(2.91f);        // fullness 0.97
    core.temperament = Temperament_Honorable;  // resolve 1.8 -> bar 0.95 * 1.12, held at 1.0
    CHECK(!DecideAnimal(core, Beast(), kConfig).willJoin);

    Config off;
    off.animals.personalityMatters = false;
    CHECK(DecideAnimal(core, Beast(), off).willJoin);
}

// --------------------------------------------------------------------- cats

PriceInputs PriceAtLevel(int level)
{
    PriceInputs p;
    p.level = level;
    return p;
}

void ACatsOfferFollowsThePurse()
{
    const char* name = "cats: the quote is asked for, and joins only when the purse covers it";
    const PriceInputs price = PriceAtLevel(20);
    const int quote = FinalPriceInCats(price, kConfig.prices);

    Verdict rich;
    ApplyCatsOffer(&rich, price, Purse(quote), kConfig.prices);
    CHECK(rich.currency == Currency_Cats);
    CHECK(rich.cats == quote);
    CHECK(rich.willJoin);
    CHECK(rich.reasonKey == Reason_CatsJoin);

    Verdict poor;
    ApplyCatsOffer(&poor, price, Purse(quote - 1), kConfig.prices);
    CHECK(poor.currency == Currency_Cats);
    CHECK(poor.cats == quote);
    CHECK(poor.purse == quote - 1);
    CHECK(!poor.willJoin);
    CHECK(poor.reasonKey == Reason_CatsAsk);
}

void AnUnknownPurseDoesNotBlockTheOffer()
{
    const char* name = "cats: when the purse cannot be read the offer stands";
    Verdict v;
    ApplyCatsOffer(&v, PriceAtLevel(20), PurseFact(), kConfig.prices);
    CHECK(v.willJoin);
    CHECK(v.reasonKey == Reason_CatsJoin);
}

// --------------------------------------------------------------- free world

FreeExtra Free(float relation, bool known = true)
{
    FreeExtra extra;
    extra.relation.known = known;
    extra.relation.value = relation;
    extra.purse = Purse(1000000);
    extra.price = PriceAtLevel(20);
    return extra;
}

void SomeoneInAnotherCageIsNotYoursToAsk()
{
    const char* name = "free world: another faction's prisoner is not yours to talk to";
    FreeExtra extra = Free(0.0f);
    extra.inAnyCage = true;
    const Verdict v = DecideFreeWorld(Core(), extra, kConfig);
    CHECK(!v.willJoin);
    CHECK(v.currency == Currency_None);
    CHECK(v.reasonKey == Reason_InOthersCage);
}

void PesteringIsRefusedBeforeAnythingElse()
{
    const char* name = "free world: pestered once too often, they will not hear you out";
    FreeExtra extra = Free(90.0f);
    extra.ignoringPlayer = true;
    const Verdict v = DecideFreeWorld(Core(), extra, kConfig);
    CHECK(v.currency == Currency_Refused);
    CHECK(v.reasonKey == Reason_Insistence);
    CHECK(v.band.empty());
}

void AnAnimalLeavesTheHumanPath()
{
    const char* name = "free world: an animal is tamed with food, never offered a bribe";
    CoreFacts core = AnimalCore(3.0f);
    const Verdict v = DecideFreeWorld(core, Free(-90.0f), kConfig);
    CHECK(v.isAnimal);
    CHECK(v.currency == Currency_Care);
    CHECK(v.band.empty());
    CHECK(v.reasonKey == Reason_TameReady);
}

void TheBandComesFromTheRelation()
{
    const char* name = "free world: the verdict carries the band the relation falls in";
    CHECK(DecideFreeWorld(Core(), Free(0.0f, false), kConfig).band == "neutral");
    CHECK(DecideFreeWorld(Core(), Free(10.0f), kConfig).band == "cordial");
    CHECK(DecideFreeWorld(Core(), Free(-20.0f), kConfig).band == "hostil");
}

void AnAllyRollsWithTheAlliedBonus()
{
    const char* name = "free world: an ally rolls level + relation + allied bonus";
    FreeExtra extra = Free(40.0f);
    extra.roll = KeyRolling(true, 65);
    // 45 + (int)(40 * 0.25) + 10 = 65
    Verdict v = DecideFreeWorld(Core(), extra, kConfig);
    CHECK(v.rolled);
    CHECK(v.chancePercent == 65);
    CHECK(v.willJoin);
    CHECK(v.currency == Currency_None);
    CHECK(v.reasonKey == Reason_AllyJoins);
    CHECK(v.band == "amistoso");

    extra.roll = KeyRolling(false, 65);
    v = DecideFreeWorld(Core(), extra, kConfig);
    CHECK(!v.willJoin);
    CHECK(v.currency == Currency_Refused);
    CHECK(v.reasonKey == Reason_AllyMaybe);
}

void TheAlliedBonusIsConfigurable()
{
    const char* name = "free world: the allied bonus comes from the config";
    Config config;
    config.chance.alliedBonus = 0;
    FreeExtra extra = Free(40.0f);
    CHECK(DecideFreeWorld(Core(), extra, config).chancePercent == 55);
    config.chance.alliedBonus = 30;
    CHECK(DecideFreeWorld(Core(), extra, config).chancePercent == 85);
}

void FriendlyIsAtOrAboveTheThreshold()
{
    const char* name = "free world: friendly standing is relation >= friendlyRelation, inclusive";
    CHECK(DecideFreeWorld(Core(), Free(20.0f), kConfig).rolled);
    CHECK(!DecideFreeWorld(Core(), Free(19.9f), kConfig).rolled);
    CHECK(FreeWorldMayRoll(Core(), Relation(20.0f), kConfig));
    CHECK(!FreeWorldMayRoll(Core(), Relation(19.9f), kConfig));
    CHECK(!FreeWorldMayRoll(Core(), RelationFact(), kConfig));
    CoreFacts animal = AnimalCore(3.0f);
    CHECK(!FreeWorldMayRoll(animal, Relation(90.0f), kConfig));
}

void TheGoodNaturedStopTalkingBelowTheirCutoff()
{
    const char* name = "free world: honourable and kind NPCs refuse below relationRefuseGoodNatured, strictly";
    CoreFacts honourable = Core();
    honourable.temperament = Temperament_Honorable;
    Verdict v = DecideFreeWorld(honourable, Free(-15.1f), kConfig);
    CHECK(v.currency == Currency_Refused);
    CHECK(v.reasonKey == Reason_FactionHonour);

    // Exactly on the cutoff is not below it: they get the bribe refusal instead.
    v = DecideFreeWorld(honourable, Free(-15.0f), kConfig);
    CHECK(v.reasonKey == Reason_HonourRefusesBribe);

    CoreFacts kind = Core();
    kind.temperament = Temperament_WarmKind;
    CHECK(DecideFreeWorld(kind, Free(-40.0f), kConfig).reasonKey == Reason_FactionHonour);

    // Someone else at the same standing is not stopped by this cutoff.
    CHECK(DecideFreeWorld(Core(), Free(-20.0f), kConfig).reasonKey == Reason_CatsJoin);
}

void NobodyTalksBelowTheSecondCutoff()
{
    const char* name = "free world: below relationRefuseAll even money does not work";
    const Verdict v = DecideFreeWorld(Core(), Free(-30.1f), kConfig);
    CHECK(v.currency == Currency_Refused);
    CHECK(v.reasonKey == Reason_FactionGreedy);
    CHECK(DecideFreeWorld(Core(), Free(-30.0f), kConfig).reasonKey == Reason_CatsJoin);
}

void TheCutoffsAreConfigurable()
{
    const char* name = "free world: both cutoffs come from the config";
    Config config;
    config.relationRefuseGoodNatured = -50.0f;
    config.relationRefuseAll = -60.0f;
    CoreFacts honourable = Core();
    honourable.temperament = Temperament_Honorable;
    CHECK(DecideFreeWorld(honourable, Free(-20.0f), config).reasonKey == Reason_HonourRefusesBribe);
    CHECK(DecideFreeWorld(honourable, Free(-55.0f), config).reasonKey == Reason_FactionHonour);
    CHECK(DecideFreeWorld(Core(), Free(-40.0f), config).reasonKey == Reason_CatsJoin);
    CHECK(DecideFreeWorld(Core(), Free(-61.0f), config).reasonKey == Reason_FactionGreedy);
}

void TheGoodNaturedNeverTakeCats()
{
    const char* name = "free world: honourable and kind NPCs refuse bribes, unless they are starving";
    CoreFacts core = Core();
    core.temperament = Temperament_Honorable;
    Verdict v = DecideFreeWorld(core, Free(0.0f), kConfig);
    CHECK(!v.willJoin);
    CHECK(v.currency == Currency_Refused);
    CHECK(v.reasonKey == Reason_HonourRefusesBribe);

    core.starving = true;
    v = DecideFreeWorld(core, Free(0.0f), kConfig);
    CHECK(!v.willJoin);
    CHECK(v.currency == Currency_Care);
    CHECK(v.reasonKey == Reason_HonourStarving);
}

void TheMilitaryServeTheirFaction()
{
    const char* name = "free world: soldiers and law enforcers refuse cats, and name the way out";
    CoreFacts core = Core();
    core.archetype = Archetype_Military;
    Verdict v = DecideFreeWorld(core, Free(0.0f), kConfig);
    CHECK(v.currency == Currency_Refused);
    CHECK(v.reasonKey == Reason_MilitaryLoyalty);
    core.archetype = Archetype_LawEnforcement;
    CHECK(DecideFreeWorld(core, Free(0.0f), kConfig).reasonKey == Reason_MilitaryLoyalty);

    // An honourable soldier is stopped by the honour rule first.
    core.temperament = Temperament_Honorable;
    CHECK(DecideFreeWorld(core, Free(0.0f), kConfig).reasonKey == Reason_HonourRefusesBribe);
}

void EveryoneElseHasAPrice()
{
    const char* name = "free world: anyone else quotes cats, and joins when the purse covers it";
    FreeExtra extra = Free(0.0f);
    Verdict v = DecideFreeWorld(Core(), extra, kConfig);
    CHECK(v.currency == Currency_Cats);
    CHECK(v.willJoin);
    CHECK(v.cats == FinalPriceInCats(extra.price, kConfig.prices));

    extra.purse = Purse(1);
    v = DecideFreeWorld(Core(), extra, kConfig);
    CHECK(!v.willJoin);
    CHECK(v.reasonKey == Reason_CatsAsk);
    CHECK(v.purse == 1);
}

// --------------------------------------------------------------------- cage

CageExtra InCage()
{
    CageExtra extra;
    extra.purse = Purse(1000000);
    extra.price = PriceAtLevel(20);
    extra.animal = Beast();
    return extra;
}

CoreFacts Prisoner()
{
    CoreFacts core = Core();
    core.inMyCage = true;
    return core;
}

// Past the intimidation roll: a key that fails it, so the currency table answers.
// A roll fails when its hash is at or above the percent, and no chance can pass
// the ceiling of 95, so a key that fails at 95 fails at every chance the case can
// produce.
CageExtra FailingTheRoll(const CoreFacts& core)
{
    CageExtra extra = InCage();
    extra.roll = KeyRolling(false, kConfig.chance.ceiling);
    (void)core;
    return extra;
}

void ACageRouteSwitchedOffIsDisabled()
{
    const char* name = "cage: with captivity switched off the answer is that it is off";
    Config config;
    config.captivityEnabled = false;
    const Verdict v = DecideCage(Prisoner(), InCage(), config);
    CHECK(!v.willJoin);
    CHECK(!v.viaCaptivity);
    CHECK(v.reasonKey == Reason_CaptivityDisabled);
}

void TimeAloneBreaksAnyone()
{
    const char* name = "cage: easy mode, held long enough anyone yields";
    CageExtra extra = InCage();
    extra.hoursHeld = kConfig.prices.easyHoursCaged;
    Verdict v = DecideCage(Prisoner(), extra, kConfig);
    CHECK(v.willJoin);
    CHECK(v.viaCaptivity);
    CHECK(v.currency == Currency_Submission);
    CHECK(v.reasonKey == Reason_EasyWait);
    CHECK(!v.rolled);

    extra.hoursHeld = kConfig.prices.easyHoursCaged - 0.01f;
    v = DecideCage(Prisoner(), extra, kConfig);
    CHECK(v.reasonKey != Reason_EasyWait);

    Config off;
    off.prices.easyHoursCaged = 0.0f;
    extra.hoursHeld = 1000.0f;
    CHECK(DecideCage(Prisoner(), extra, off).reasonKey != Reason_EasyWait);
}

void IntimidationIsAStrongArgumentNotAVerdict()
{
    const char* name = "cage: being caged and outmatched rolls level + defeats + stress + intimidation";
    CoreFacts core = Prisoner();
    CageExtra extra = InCage();
    extra.roll = KeyRolling(true, 60);
    // 45 + 15 = 60 at even levels with nothing else.
    Verdict v = DecideCage(core, extra, kConfig);
    CHECK(v.rolled);
    CHECK(v.chancePercent == 60);
    CHECK(v.willJoin);
    CHECK(v.currency == Currency_Submission);
    CHECK(v.reasonKey == Reason_Intimidated);
    CHECK(v.viaCaptivity);
}

void TheCageRollCountsTheGrudgeAndTheName()
{
    const char* name = "cage: the grudge and being unique both cost chance here";
    CoreFacts core = Prisoner();
    CageExtra extra = InCage();
    const int base = DecideCage(core, extra, kConfig).chancePercent;
    core.grudge = true;
    CHECK(DecideCage(core, extra, kConfig).chancePercent == base - kConfig.chance.grudgePenalty);
    core.grudge = false;
    core.unique = true;
    CHECK(DecideCage(core, extra, kConfig).chancePercent == base - kConfig.chance.uniquePenalty);
    core.unique = false;
    core.defeats = 3;
    CHECK(DecideCage(core, extra, kConfig).chancePercent > base);
}

void ASlaveIsFreedNotBought()
{
    const char* name = "cage: someone you freed from slavers follows you";
    CoreFacts core = Prisoner();
    core.enslaved = true;
    const Verdict v = DecideCage(core, FailingTheRoll(core), kConfig);
    CHECK(v.willJoin);
    CHECK(v.currency == Currency_Freedom);
    CHECK(v.reasonKey == Reason_FreedFollows);
}

void TheHurtAndHungryWantCare()
{
    const char* name = "cage: starving, wounded or both want care, and say which";
    CoreFacts core = Prisoner();
    core.starving = true;
    Verdict v = DecideCage(core, FailingTheRoll(core), kConfig);
    CHECK(!v.willJoin && v.currency == Currency_Care && v.reasonKey == Reason_Starving);
    core.starving = false;
    core.wounded = true;
    v = DecideCage(core, FailingTheRoll(core), kConfig);
    CHECK(!v.willJoin && v.reasonKey == Reason_Wounded);
    core.starving = true;
    v = DecideCage(core, FailingTheRoll(core), kConfig);
    CHECK(!v.willJoin && v.reasonKey == Reason_StarvingAndWounded);
}

void TheKindAreCaredForAndJoin()
{
    const char* name = "cage: an honourable prisoner who is fed and healed joins";
    CoreFacts core = Prisoner();
    core.temperament = Temperament_Honorable;
    const Verdict v = DecideCage(core, FailingTheRoll(core), kConfig);
    CHECK(v.willJoin);
    CHECK(v.currency == Currency_Care);
    CHECK(v.reasonKey == Reason_CaredFor);
}

void AnAnimalInTheCageIsTamedByTheSameBar()
{
    const char* name = "cage: a starving caged animal settles on the taming bar; a healthy wary one does not bond";
    CoreFacts hungry = Prisoner();
    hungry.animal = true;
    hungry.starving = true;
    hungry.hunger = 0.0f;
    CageExtra extra = FailingTheRoll(hungry);
    Verdict v = DecideCage(hungry, extra, kConfig);
    CHECK(v.isAnimal);
    CHECK(v.wantsFood);
    CHECK(v.reasonKey == Reason_TameHungry);

    CoreFacts wary = Prisoner();
    wary.animal = true;
    v = DecideCage(wary, FailingTheRoll(wary), kConfig);
    CHECK(!v.willJoin);
    CHECK(v.currency == Currency_Care);
    CHECK(v.reasonKey == Reason_AnimalWary);

    // Unless it remembers who saved it.
    wary.rememberedAsSaviour = true;
    v = DecideCage(wary, FailingTheRoll(wary), kConfig);
    CHECK(v.willJoin);
    CHECK(v.reasonKey == Reason_CaredForAnimal);
}

void ASoldierIsBrokenNotBought()
{
    const char* name = "cage: a soldier's price is submission; an ordinary one stays defiant";
    CoreFacts core = Prisoner();
    core.archetype = Archetype_Military;
    Verdict v = DecideCage(core, FailingTheRoll(core), kConfig);
    CHECK(!v.willJoin);
    CHECK(v.currency == Currency_Submission);
    CHECK(v.reasonKey == Reason_Defiant);
}

void AZealotHasNoPrice()
{
    const char* name = "cage: a named character who does not trade has no price at all";
    CoreFacts core = Prisoner();
    core.unique = true;
    core.temperament = Temperament_Brave;
    core.archetype = Archetype_Military;
    Verdict v = DecideCage(core, FailingTheRoll(core), kConfig);
    CHECK(v.currency == Currency_Submission);
    CHECK(v.reasonKey == Reason_UniqueZealot);
    CHECK(!v.willJoin);
}

void ABanditKingHasAPrice()
{
    const char* name = "cage: a named bandit trades, so he is quoted twice an ordinary price";
    CoreFacts core = Prisoner();
    core.unique = true;
    CageExtra extra = FailingTheRoll(core);
    extra.price.unique = true;
    const Verdict v = DecideCage(core, extra, kConfig);
    CHECK(v.currency == Currency_Cats);
    CHECK(v.cats == FinalPriceInCats(extra.price, kConfig.prices));
    CHECK(v.willJoin);
}

void AnOrdinaryPrisonerIsQuotedCats()
{
    const char* name = "cage: a bandit in a cage is quoted cats and joins when the purse covers it";
    CoreFacts core = Prisoner();
    CageExtra extra = FailingTheRoll(core);
    Verdict v = DecideCage(core, extra, kConfig);
    CHECK(v.currency == Currency_Cats);
    CHECK(v.willJoin);
    CHECK(v.reasonKey == Reason_CatsJoin);
    extra.purse = Purse(0);
    v = DecideCage(core, extra, kConfig);
    CHECK(!v.willJoin);
    CHECK(v.reasonKey == Reason_CatsAsk);
}

void TheCurrencyTable()
{
    const char* name = "currency: condition first, then temperament, then archetype";
    CoreFacts core = Core();
    CHECK(ChooseCurrency(core) == Currency_Cats);                  // a bandit

    core.animal = true;
    CHECK(ChooseCurrency(core) == Currency_Care);
    core.animal = false;

    core.enslaved = true;
    CHECK(ChooseCurrency(core) == Currency_Freedom);
    core.enslaved = false;
    core.archetype = Archetype_Slave;
    CHECK(ChooseCurrency(core) == Currency_Freedom);

    core = Core();
    core.starving = true;
    CHECK(ChooseCurrency(core) == Currency_Care);
    core = Core();
    core.wounded = true;
    core.archetype = Archetype_Military;
    CHECK(ChooseCurrency(core) == Currency_Care);

    core = Core();
    core.temperament = Temperament_Honorable;
    CHECK(ChooseCurrency(core) == Currency_Care);
    core.temperament = Temperament_WarmKind;
    CHECK(ChooseCurrency(core) == Currency_Care);

    const Archetype pays[] = { Archetype_Bandit, Archetype_Trader, Archetype_Slaver,
                               Archetype_Civilian, Archetype_Adventurer, Archetype_Diplomat };
    for (size_t i = 0; i < sizeof(pays) / sizeof(pays[0]); ++i)
    {
        core = Core();
        core.archetype = pays[i];
        CHECK(ChooseCurrency(core) == Currency_Cats);
    }

    core = Core();
    core.archetype = Archetype_Military;
    CHECK(ChooseCurrency(core) == Currency_Submission);
    core.archetype = Archetype_LawEnforcement;
    CHECK(ChooseCurrency(core) == Currency_Submission);

    core = Core();
    core.archetype = Archetype_Other;
    CHECK(ChooseCurrency(core) == Currency_Cats);
    core.temperament = Temperament_Brave;
    CHECK(ChooseCurrency(core) == Currency_Submission);
}

void WhoTradesAmongTheNamed()
{
    const char* name = "unique: cruel, traitorous, maniacal and clever trade; so do the trades that sell people";
    CHECK(UniqueTakesMoney(Temperament_ColdCruel, Archetype_Military));
    CHECK(UniqueTakesMoney(Temperament_Traitorous, Archetype_Military));
    CHECK(UniqueTakesMoney(Temperament_Maniacal, Archetype_Military));
    CHECK(UniqueTakesMoney(Temperament_Smart, Archetype_Military));
    CHECK(UniqueTakesMoney(Temperament_Normal, Archetype_Bandit));
    CHECK(UniqueTakesMoney(Temperament_Normal, Archetype_Trader));
    CHECK(UniqueTakesMoney(Temperament_Normal, Archetype_Slaver));
    CHECK(!UniqueTakesMoney(Temperament_Normal, Archetype_Military));
    CHECK(!UniqueTakesMoney(Temperament_Brave, Archetype_Civilian));
    CHECK(!UniqueTakesMoney(Temperament_Honorable, Archetype_Diplomat));
}

// ---------------------------------------------------------- route selection

void RouteFactsAreBuiltFromTheCoreAndTheConfig()
{
    const char* name = "route facts: the core and the config give the facts ChooseEntryRoute reads";
    CoreFacts core = Core();
    core.rememberedAsSaviour = true;
    core.inMyCage = true;
    Config config;
    RouteFacts f = MakeRouteFacts(core, config);
    CHECK(f.remembersSaviour);
    CHECK(f.inMyCage);
    CHECK(ChooseEntryRoute(f) == Route_Rescue);

    // The saviour memory only counts with the rescue route on.
    config.rescueEnabled = false;
    f = MakeRouteFacts(core, config);
    CHECK(!f.remembersSaviour);
    CHECK(ChooseEntryRoute(f) == Route_Cage);

    // The grudge only refuses outright for the loyal.
    CoreFacts grudging = Core();
    grudging.grudge = true;
    grudging.temperament = Temperament_Honorable;
    CHECK(ChooseEntryRoute(MakeRouteFacts(grudging, kConfig)) == Route_GrudgeRefusal);
    grudging.temperament = Temperament_Normal;
    CHECK(ChooseEntryRoute(MakeRouteFacts(grudging, kConfig)) == Route_FreeWorld);

    // Easy-healthy: easy mode on, fed, unwounded, full health, in the cage.
    CoreFacts patched = Core();
    patched.inMyCage = true;
    patched.health = 1.0f;
    CHECK(ChooseEntryRoute(MakeRouteFacts(patched, kConfig)) == Route_EasyHealthy);
    patched.health = 0.9f;
    CHECK(ChooseEntryRoute(MakeRouteFacts(patched, kConfig)) == Route_Cage);
}
} // namespace

void RunRouteDecisionsTests()
{
    TheTamingBarTravelsOnTheVerdict();
    AnimalsSwitchedOffStopsAnAnimal();
    UnreadableTraitsFallBackToAPlainRecruit();
    AFakerAndACorpseAreNotRecruitable();
    TheGateChecksInTheShippedOrder();
    AnOrdinaryCharacterPassesTheGate();
    CheatWithoutACageJoinsAtOnce();
    CheatAsksForTheCage();
    CheatWaitsAMinuteThenJoins();
    OutColdAndOwingWaitsForThemToWake();
    OutColdAndDyingWantsCare();
    OutColdAndNothingElseJustWaits();
    DyingAndEasyHealthyAreRescueRoutes();
    ASavedLifeRollsAtTheSavedLifeChance();
    ARescuedOneWhoCannotFollowPaysItForward();
    HealingThemMoreArguesMore();
    WithoutAKeyARescueIsNotAGiven();
    FieldYieldRollsAtTheBeatingChance();
    FieldYieldNotYetNamesTheWayOut();
    FieldYieldReadsStressAndUniqueButNotTheGrudge();
    GrudgeAndSurrenderAreFixedAnswers();
    AnOwnedAnimalIsOnlyRefusedInTheOpen();
    AFullAnimalFollowsYou();
    AHungryAnimalSaysHowMuchIsMissing();
    AWoundedAnimalWantsHealingEvenWhenFull();
    TemperamentMovesTheBarOnlyWhenItMatters();
    ACatsOfferFollowsThePurse();
    AnUnknownPurseDoesNotBlockTheOffer();
    SomeoneInAnotherCageIsNotYoursToAsk();
    PesteringIsRefusedBeforeAnythingElse();
    AnAnimalLeavesTheHumanPath();
    TheBandComesFromTheRelation();
    AnAllyRollsWithTheAlliedBonus();
    TheAlliedBonusIsConfigurable();
    FriendlyIsAtOrAboveTheThreshold();
    TheGoodNaturedStopTalkingBelowTheirCutoff();
    NobodyTalksBelowTheSecondCutoff();
    TheCutoffsAreConfigurable();
    TheGoodNaturedNeverTakeCats();
    TheMilitaryServeTheirFaction();
    EveryoneElseHasAPrice();
    ACageRouteSwitchedOffIsDisabled();
    TimeAloneBreaksAnyone();
    IntimidationIsAStrongArgumentNotAVerdict();
    TheCageRollCountsTheGrudgeAndTheName();
    ASlaveIsFreedNotBought();
    TheHurtAndHungryWantCare();
    TheKindAreCaredForAndJoin();
    AnAnimalInTheCageIsTamedByTheSameBar();
    ASoldierIsBrokenNotBought();
    AZealotHasNoPrice();
    ABanditKingHasAPrice();
    AnOrdinaryPrisonerIsQuotedCats();
    TheCurrencyTable();
    WhoTradesAmongTheNamed();
    RouteFactsAreBuiltFromTheCoreAndTheConfig();
}
