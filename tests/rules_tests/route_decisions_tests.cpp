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
#include "cared_for_rules.h"
#include "route_rules.h"
#include "chance_rules.h"
#include "animal_rules.h"
#include "price_rules.h"
#include "haggle_rules.h"

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

// A key whose daily roll at `percent` comes out as asked, on day 3 (any day
// works: the search runs on the same day the route will roll on).
RollFact KeyRolling(bool joins, int percent)
{
    RollFact roll;
    roll.haveKey = true;
    roll.dayIndex = 3;
    for (unsigned int i = 1; i < 100000; ++i)
    {
        roll.key = CharKey(i, 1u);
        if (RollsToJoin(true, roll.key, roll.dayIndex, percent) == joins)
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
    CHECK(v.reasonKey == Reason_CheatNeedsAnyCage);
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

// cheat-any-cage: with cheatAnyCage on (the default) another faction's cage,
// shackles or a kidnap is a cage too, with the same minute.
void CheatAnyCageWaitsAMinuteThenJoins()
{
    const char* name = "cheat any cage: in someone else's cage they wait the minute and join";
    Config config;
    CHECK(config.cheatAnyCage);
    CheatFacts facts;
    facts.inMyCage = false;
    facts.inAnyCage = true;
    facts.hoursHeld = config.cheatHoursCaged - 0.001f;
    Verdict v = DecideCheat(facts, config);
    CHECK(!v.willJoin);
    CHECK(v.currency == Currency_Refused);
    CHECK(v.reasonKey == Reason_CheatWaiting);

    facts.hoursHeld = config.cheatHoursCaged;
    v = DecideCheat(facts, config);
    CHECK(v.willJoin);
    CHECK(v.currency == Currency_None);
    CHECK(v.reasonKey == Reason_CheatJoins);
}

void CheatAnyCageOffKeepsYourCageOnly()
{
    const char* name = "cheat any cage: switched off, someone else's cage still asks for yours";
    Config config;
    config.cheatAnyCage = false;
    CheatFacts facts;
    facts.inMyCage = false;
    facts.inAnyCage = true;
    facts.hoursHeld = 99.0f;
    const Verdict v = DecideCheat(facts, config);
    CHECK(!v.willJoin);
    CHECK(v.reasonKey == Reason_CheatNeedsCage);
}

void CheatAnyCageAsksForAnyCage()
{
    const char* name = "cheat any cage: the cage answer names any cage on, your cage off";
    Config config;
    CheatFacts facts;
    facts.inMyCage = false;
    facts.inAnyCage = false;
    config.cheatAnyCage = true;
    Verdict v = DecideCheat(facts, config);
    CHECK(v.reasonKey == Reason_CheatNeedsAnyCage);
    CHECK(v.currency == Currency_Refused);
    config.cheatAnyCage = false;
    v = DecideCheat(facts, config);
    CHECK(v.reasonKey == Reason_CheatNeedsCage);
    CHECK(v.currency == Currency_Refused);
    // A free animal keeps food as the other way in, with either text.
    CheatFacts animal;
    animal.animal = true;
    animal.fedByPlayer = false;
    animal.inMyCage = false;
    config.cheatAnyCage = true;
    v = DecideCheat(animal, config);
    CHECK(v.reasonKey == Reason_CheatNeedsAnyCage);
    CHECK(v.wantsFood);
}

void CheatAnyCageStillNeedsACage()
{
    const char* name = "cheat any cage: free in the open is still the cage answer";
    Config config;
    CheatFacts facts;
    facts.inMyCage = false;
    facts.inAnyCage = false;
    facts.hoursHeld = 99.0f;
    const Verdict v = DecideCheat(facts, config);
    CHECK(!v.willJoin);
    CHECK(v.reasonKey == Reason_CheatNeedsAnyCage);
}

void CheatCageCountsRule()
{
    const char* name = "cheat any cage: which cage satisfies Cheat";
    Config config;
    CHECK(CheatCageSatisfied(true, true, config));
    CHECK(CheatCageSatisfied(true, false, config));
    CHECK(CheatCageSatisfied(false, true, config));
    CHECK(!CheatCageSatisfied(false, false, config));
    config.cheatAnyCage = false;
    CHECK(CheatCageSatisfied(true, true, config));
    CHECK(!CheatCageSatisfied(false, true, config));
}

void CheatAnyCageHoldClockRule()
{
    const char* name = "cheat any cage: the hold clock counts any cage only in Cheat with the switch on";
    Config config;
    config.preset = Preset_Balanced;
    CHECK(!HoldClockCountsAnyCage(config));
    config.preset = Preset_Cheat;
    CHECK(HoldClockCountsAnyCage(config));
    config.cheatAnyCage = false;
    CHECK(!HoldClockCountsAnyCage(config));
    config.cheatAnyCage = true;
    config.cheatNeedsCage = false;
    CHECK(!HoldClockCountsAnyCage(config));
}

void TheHoldClockCountsThisCage()
{
    const char* name = "hold clock: the player's cage always counts, any cage only where the clock runs there";
    Config config;
    config.preset = Preset_Balanced;
    CHECK(HoldClockCounts(true, true, config));
    CHECK(HoldClockCounts(true, false, config));
    CHECK(!HoldClockCounts(false, true, config));
    CHECK(!HoldClockCounts(false, false, config));
    config.preset = Preset_Cheat;
    CHECK(HoldClockCounts(false, true, config));
    CHECK(!HoldClockCounts(false, false, config));
    config.cheatAnyCage = false;
    CHECK(!HoldClockCounts(false, true, config));
    CHECK(HoldClockCounts(true, false, config));
}

// An animal that ate from the player's hand (the same PlayerMeals fact Balanced's
// animal route reads) joins in Cheat at once: no cage, no wait, no bar.
CheatFacts FedAnimal(bool inCage)
{
    CheatFacts facts;
    facts.animal = true;
    facts.fedByPlayer = true;
    facts.inMyCage = inCage;
    return facts;
}

void CheatAFedAnimalJoinsWithoutTheCage()
{
    const char* name = "cheat: an animal fed by the player joins at once, no cage";
    const Verdict v = DecideCheat(FedAnimal(false), kConfig);
    CHECK(v.willJoin);
    CHECK(v.isAnimal);
    CHECK(v.currency == Currency_Care);
    CHECK(!v.wantsFood);
    CHECK(v.reasonKey == Reason_TameReady);
}

void CheatAFedAnimalInTheCageDoesNotWait()
{
    const char* name = "cheat: a fed animal in the cage joins without the minute";
    CheatFacts facts = FedAnimal(true);
    facts.hoursHeld = 0.0f;
    const Verdict v = DecideCheat(facts, kConfig);
    CHECK(v.willJoin);
    CHECK(v.reasonKey == Reason_TameReady);
}

void CheatAnUnfedAnimalOutsideStillNeedsTheCage()
{
    const char* name = "cheat: an unfed animal outside the cage asks for the cage, and takes food";
    CheatFacts facts = FedAnimal(false);
    facts.fedByPlayer = false;
    const Verdict v = DecideCheat(facts, kConfig);
    CHECK(!v.willJoin);
    CHECK(v.isAnimal);
    CHECK(v.reasonKey == Reason_CheatNeedsAnyCage);
    // The feed button (RemedyFor) needs care and an appetite: food is the other way in.
    CHECK(v.currency == Currency_Care);
    CHECK(v.wantsFood);
}

void CheatAnUnfedAnimalInTheCageWaitsTheMinute()
{
    const char* name = "cheat: an unfed animal in the cage keeps the cage path";
    CheatFacts facts = FedAnimal(true);
    facts.fedByPlayer = false;
    facts.hoursHeld = 0.0f;
    Verdict v = DecideCheat(facts, kConfig);
    CHECK(!v.willJoin);
    CHECK(v.reasonKey == Reason_CheatWaiting);
    facts.hoursHeld = kConfig.cheatHoursCaged;
    v = DecideCheat(facts, kConfig);
    CHECK(v.willJoin);
    CHECK(v.reasonKey == Reason_CheatJoins);
}

void CheatAnOwnedAnimalOutsideNeedsTheCageFedOrNot()
{
    const char* name = "cheat: an animal that belongs to someone is only taken through the cage";
    CheatFacts facts = FedAnimal(false);
    facts.belongsToSomeone = true;
    const Verdict v = DecideCheat(facts, kConfig);
    CHECK(!v.willJoin);
    CHECK(v.currency == Currency_Refused);
    CHECK(!v.wantsFood);
    CHECK(v.reasonKey == Reason_CheatNeedsAnyCage);
}

void CheatFoodMeansNothingToAPerson()
{
    const char* name = "cheat: a person outside the cage still needs the cage";
    CheatFacts facts;
    facts.fedByPlayer = true;
    const Verdict v = DecideCheat(facts, kConfig);
    CHECK(!v.willJoin);
    CHECK(!v.isAnimal);
    CHECK(v.currency == Currency_Refused);
    CHECK(!v.wantsFood);
    CHECK(v.reasonKey == Reason_CheatNeedsAnyCage);
}

void CheatWithoutACageIsUnchangedForAnimals()
{
    const char* name = "cheat: with the cage switched off a fed or unfed animal simply joins";
    Config config;
    config.cheatNeedsCage = false;
    CheatFacts facts = FedAnimal(false);
    CHECK(DecideCheat(facts, config).reasonKey == Reason_CheatJoinsFreely);
    facts.fedByPlayer = false;
    CHECK(DecideCheat(facts, config).reasonKey == Reason_CheatJoinsFreely);
}

// animal-duel-submission D4: in Cheat a cure or one knockout by the squad is the
// gesture too, like a meal.
CheatFacts FreeAnimalOutside()
{
    CheatFacts facts;
    facts.animal = true;
    return facts;
}

void CheatAHealedAnimalJoinsWithoutTheCage()
{
    const char* name = "cheat: an animal the player healed joins at once, no cage";
    CheatFacts facts = FreeAnimalOutside();
    facts.healedByPlayer = true;
    const Verdict v = DecideCheat(facts, kConfig);
    CHECK(v.willJoin);
    CHECK(v.isAnimal);
    CHECK(v.currency == Currency_None);
    CHECK(!v.wantsFood);
    CHECK(v.reasonKey == Reason_CheatAnimalHealed);
}

void CheatADefeatedAnimalJoinsWhenSpokenTo()
{
    const char* name = "cheat: an animal the squad knocked out joins when spoken to";
    CheatFacts facts = FreeAnimalOutside();
    facts.defeatedByPlayer = true;
    Verdict v = DecideCheat(facts, kConfig);
    CHECK(v.willJoin);
    CHECK(v.isAnimal);
    CHECK(v.currency == Currency_None);
    CHECK(v.reasonKey == Reason_CheatAnimalDefeated);

    // In the cage it does not wait the minute either.
    facts.inMyCage = true;
    facts.hoursHeld = 0.0f;
    v = DecideCheat(facts, kConfig);
    CHECK(v.willJoin);
    CHECK(v.reasonKey == Reason_CheatAnimalDefeated);
}

void CheatTheMealIsNamedFirst()
{
    const char* name = "cheat: fed, healed and beaten reads as the meal; healed and beaten as the cure";
    CheatFacts facts = FreeAnimalOutside();
    facts.fedByPlayer = true;
    facts.healedByPlayer = true;
    facts.defeatedByPlayer = true;
    CHECK(DecideCheat(facts, kConfig).reasonKey == Reason_TameReady);
    facts.fedByPlayer = false;
    CHECK(DecideCheat(facts, kConfig).reasonKey == Reason_CheatAnimalHealed);
}

void CheatAnOwnedAnimalIsStillCageOnly()
{
    const char* name = "cheat: someone else's animal, healed or beaten, still needs the cage";
    CheatFacts facts = FreeAnimalOutside();
    facts.belongsToSomeone = true;
    facts.healedByPlayer = true;
    facts.defeatedByPlayer = true;
    const Verdict v = DecideCheat(facts, kConfig);
    CHECK(!v.willJoin);
    CHECK(v.currency == Currency_Refused);
    CHECK(v.reasonKey == Reason_CheatNeedsAnyCage);
}

void CheatAPersonIsNotMovedByACureOrABeating()
{
    const char* name = "cheat: a healed or beaten person outside the cage still needs the cage";
    CheatFacts facts;
    facts.healedByPlayer = true;
    facts.defeatedByPlayer = true;
    Verdict v = DecideCheat(facts, kConfig);
    CHECK(!v.willJoin);
    CHECK(!v.isAnimal);
    CHECK(v.reasonKey == Reason_CheatNeedsAnyCage);
    facts.inMyCage = true;
    facts.hoursHeld = 0.0f;
    v = DecideCheat(facts, kConfig);
    CHECK(v.reasonKey == Reason_CheatWaiting);
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

void DyingIsARescueRoute()
{
    const char* name = "rescue: dying wants care first";
    CoreFacts core = Core();
    Verdict d = DecideDying(core);
    CHECK(!d.willJoin && d.viaRescue && d.currency == Currency_Care && d.reasonKey == Reason_Dying);
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

// The bug: the card's peek took no roll key, RollsToJoin said no without one, and
// the card read "cannot follow you: pays with what they have" while the click on
// the same NPC said CAN JOIN. The peek now builds the roll from the id the NPC
// already has (never minting one), the click from the id it mints or finds: for an
// NPC with an id both are the same key on the same day, so the verdicts agree.
void RescuePeekAndDecideAgreeForAKeyedNpc()
{
    const char* name = "rescue: the card's peek and the click give the same verdict for a keyed NPC";
    const double hours = 3.0 * 24.0 + 7.5;   // day 3
    CharKey key;
    for (unsigned int i = 1; i < 100000; ++i)
    {
        key = CharKey(i, 1u);
        if (RollsToJoin(true, key, DayIndexFromHours(hours), 95))
        {
            break;
        }
    }
    CoreFacts core = Core();
    core.health = 1.0f;
    RescueExtra decide;
    decide.relation = Relation(80.0f);
    decide.roll = RollFactFrom(true, key, true, hours);   // the click: minted or found
    RescueExtra peek;
    peek.relation = Relation(80.0f);
    peek.roll = RollFactFrom(true, key, true, hours);     // the card: found only

    const Verdict byClick = DecideRescue(core, decide, kConfig);
    const Verdict byCard = DecideRescue(core, peek, kConfig);
    CHECK(byClick.chancePercent == 95);
    CHECK(byClick.willJoin);
    CHECK(byCard.willJoin == byClick.willJoin);
    CHECK(byCard.reasonKey == byClick.reasonKey);
    CHECK(byCard.currency == byClick.currency);
}

void ARollNeedsTheKeyAndTheClock()
{
    const char* name = "roll fact: no key or no game clock -> no stable seed, no key";
    const CharKey key(7u, 1u);
    const RollFact both = RollFactFrom(true, key, true, 50.0);
    CHECK(both.haveKey);
    CHECK(both.key == key);
    CHECK(both.dayIndex == 2);
    CHECK(!RollFactFrom(false, key, true, 50.0).haveKey);
    CHECK(!RollFactFrom(true, key, false, 50.0).haveKey);
}

// --------------------------------------------------------------------- yield

void FieldYieldRollsAtTheBeatingChance()
{
    const char* name = "field yield: level + defeats + stress + the yield bonus";
    CoreFacts core = Core();
    core.defeats = 2;
    YieldExtra extra;
    extra.roll = KeyRolling(true, 69);
    // 35 + 24 (two defeats at 12) + 0 (fed, unhurt, rested) + 10 = 69
    const Verdict v = DecideFieldYield(core, extra, kConfig);
    CHECK(v.rolled);
    CHECK(v.chancePercent == 69);
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

// recruit-gating T2 -- field-yield ran before every free-world check, so a guard
// in somebody else's cage, or one who had stopped listening, still yielded.
void FieldYieldRefusesSomeoneInAnotherCage()
{
    const char* name = "field yield: another faction's prisoner is not yours to ask";
    CoreFacts core = Core();
    core.defeats = 2;
    YieldExtra extra;
    extra.roll = KeyRolling(true, 79);
    extra.access.inAnyCage = true;
    const Verdict v = DecideFieldYield(core, extra, kConfig);
    CHECK(!v.willJoin);
    CHECK(!v.rolled);
    CHECK(v.reasonKey == Reason_InOthersCage);
}

void FieldYieldRefusesUnderThePesteringLockout()
{
    const char* name = "field yield: pestered once too often, they will not hear you out";
    CoreFacts core = Core();
    core.defeats = 2;
    YieldExtra extra;
    extra.roll = KeyRolling(true, 79);
    extra.access.ignoringPlayer = true;
    const Verdict v = DecideFieldYield(core, extra, kConfig);
    CHECK(!v.willJoin);
    CHECK(!v.rolled);
    CHECK(v.currency == Currency_Refused);
    CHECK(v.reasonKey == Reason_Insistence);
}

// Loyalty to a standing organisation is a weight on the roll, not a wall: the
// free world refuses them outright, but a beating is the one argument they hear.
void FieldYieldLoyaltyIsAPenaltyNotASkip()
{
    const char* name = "field yield: military and law loyalty lower the chance, they still roll";
    CHECK(kConfig.submission.fieldYieldLoyaltyPenalty == 25);

    CoreFacts bandit = Core();
    bandit.defeats = 2;
    YieldExtra extra;
    const int base = DecideFieldYield(bandit, extra, kConfig).chancePercent;
    CHECK(base == 69);

    const Archetype loyal[] = { Archetype_Military, Archetype_LawEnforcement };
    for (int i = 0; i < 2; ++i)
    {
        CoreFacts guard = bandit;
        guard.archetype = loyal[i];
        YieldExtra joins;
        joins.roll = KeyRolling(true, base - kConfig.submission.fieldYieldLoyaltyPenalty);
        const Verdict v = DecideFieldYield(guard, joins, kConfig);
        CHECK(v.rolled);
        CHECK(v.chancePercent == base - kConfig.submission.fieldYieldLoyaltyPenalty);
        CHECK(v.currency == Currency_Submission);
        CHECK(v.willJoin);
        CHECK(v.reasonKey == Reason_FieldYieldJoins);
    }

    Config noPenalty;
    noPenalty.submission.fieldYieldLoyaltyPenalty = 0;
    CoreFacts guard = bandit;
    guard.archetype = Archetype_Military;
    CHECK(DecideFieldYield(guard, extra, noPenalty).chancePercent == base);
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

// An animal the player has fed once: the bar alone decides (cage-animals T2).
AnimalExtra Beast()
{
    AnimalExtra a;
    a.maturity = 1.0f;
    a.fedByPlayer = 1;
    return a;
}

// The same animal, never fed from the player's hand.
AnimalExtra Unfed()
{
    AnimalExtra a = Beast();
    a.fedByPlayer = 0;
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

// cage-animals-submission T2: a wild animal sits at a full stomach on its own, so
// fullness alone handed one to anyone who spoke to it. Only the player's food tames.
void AFullUnfedWildAnimalRefuses()
{
    const char* name = "animal: a full wild animal the player never fed does not join";
    const Verdict v = DecideAnimal(AnimalCore(3.0f), Unfed(), kConfig);
    CHECK(!v.willJoin);
    CHECK(v.isAnimal);
    CHECK(v.currency == Currency_Care);
    CHECK(v.reasonKey == Reason_TameNotFed);
    CHECK(v.cats == 0);
    // The bar is still judged, so the card can show the stomach.
    CHECK(v.tameFullness >= v.tameNeeded && v.tameNeeded > 0.0f);
}

void AFedFullAnimalJoins()
{
    const char* name = "animal: fed once by the player and past the bar, it joins";
    AnimalExtra fed = Unfed();
    fed.fedByPlayer = 3;
    const Verdict v = DecideAnimal(AnimalCore(3.0f), fed, kConfig);
    CHECK(v.willJoin);
    CHECK(v.reasonKey == Reason_TameReady);
}

void FeedingAFullUnfedAnimalIsOffered()
{
    const char* name = "animal: food is wanted from a full animal the player never fed";
    const Verdict full = DecideAnimal(AnimalCore(3.0f), Unfed(), kConfig);
    CHECK(full.wantsFood);
    // Fed and past the bar: nothing more to give.
    CHECK(!DecideAnimal(AnimalCore(3.0f), Beast(), kConfig).wantsFood);
    // Fed but short of the bar: still hungry for more.
    const Verdict shortOfIt = DecideAnimal(AnimalCore(1.5f), Beast(), kConfig);
    CHECK(shortOfIt.wantsFood);
    CHECK(shortOfIt.reasonKey == Reason_TameHungry);
}

void AnUnfedHungryAnimalNamesTheGap()
{
    const char* name = "animal: unfed and short of the bar, the missing stomach is still the line";
    const Verdict v = DecideAnimal(AnimalCore(1.5f), Unfed(), kConfig);
    CHECK(!v.willJoin);
    CHECK(v.wantsFood);
    CHECK(v.reasonKey == Reason_TameHungry);
    CHECK(v.cats == 45);
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
    ApplyCatsOffer(&rich, price, Purse(quote), HaggleFact(), kConfig.prices);
    CHECK(rich.currency == Currency_Cats);
    CHECK(rich.cats == quote);
    CHECK(rich.willJoin);
    CHECK(rich.reasonKey == Reason_CatsJoin);

    Verdict poor;
    ApplyCatsOffer(&poor, price, Purse(quote - 1), HaggleFact(), kConfig.prices);
    CHECK(poor.currency == Currency_Cats);
    CHECK(poor.cats == quote);
    CHECK(poor.purse == quote - 1);
    CHECK(!poor.willJoin);
    CHECK(poor.reasonKey == Reason_CatsAsk);
}

// haggle-and-daily-roll T2: an NPC offended by a lowball asks more for the rest
// of that day, and the purse is checked against the raised price.
void AnOffendedNpcQuotesTheMarkedUpAsk()
{
    const char* name = "cats: an offended NPC quotes the asking price plus the offence markup";
    const PriceInputs price = PriceAtLevel(20);
    const int quote = FinalPriceInCats(price, kConfig.prices);
    const int raised = OffendedAsk(quote, true, kConfig.prices);
    CHECK(raised > quote);

    HaggleFact offended;
    offended.offended = true;

    Verdict v;
    ApplyCatsOffer(&v, price, Purse(raised), offended, kConfig.prices);
    CHECK(v.cats == raised);
    CHECK(v.willJoin);

    Verdict shortOfIt;
    ApplyCatsOffer(&shortOfIt, price, Purse(quote), offended, kConfig.prices);
    CHECK(shortOfIt.cats == raised);
    CHECK(!shortOfIt.willJoin);
    CHECK(shortOfIt.reasonKey == Reason_CatsAsk);
}

void AnUnknownPurseDoesNotBlockTheOffer()
{
    const char* name = "cats: when the purse cannot be read the offer stands";
    Verdict v;
    ApplyCatsOffer(&v, PriceAtLevel(20), PurseFact(), HaggleFact(), kConfig.prices);
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
    extra.access.inAnyCage = true;
    const Verdict v = DecideFreeWorld(Core(), extra, kConfig);
    CHECK(!v.willJoin);
    CHECK(v.currency == Currency_None);
    CHECK(v.reasonKey == Reason_InOthersCage);
}

void PesteringIsRefusedBeforeAnythingElse()
{
    const char* name = "free world: pestered once too often, they will not hear you out";
    FreeExtra extra = Free(90.0f);
    extra.access.ignoringPlayer = true;
    const Verdict v = DecideFreeWorld(Core(), extra, kConfig);
    CHECK(v.currency == Currency_Refused);
    CHECK(v.reasonKey == Reason_Insistence);
    CHECK(v.band.empty());
}

void AnAnimalLeavesTheHumanPath()
{
    const char* name = "free world: an animal is tamed with food, never offered a bribe";
    CoreFacts core = AnimalCore(3.0f);
    FreeExtra extra = Free(-90.0f);
    extra.animal = Beast();
    const Verdict v = DecideFreeWorld(core, extra, kConfig);
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

// The single ally roll every preset had before the Balanced level tiers
// (ally_tier_rules.h). Balanced with the tiers switched off keeps it unchanged.
Config TiersOff()
{
    Config config;
    config.preset = Preset_Balanced;
    config.allyTiers.enabled = false;
    return config;
}

void AnAllyRollsWithTheAlliedBonus()
{
    const char* name = "free world (tiers off): an ally rolls level + relation + allied bonus";
    const Config kConfig = TiersOff();
    FreeExtra extra = Free(40.0f);
    extra.roll = KeyRolling(true, 55);
    // 35 + (int)(40 * 0.25) + 10 = 55
    Verdict v = DecideFreeWorld(Core(), extra, kConfig);
    CHECK(v.rolled);
    CHECK(v.chancePercent == 55);
    CHECK(v.willJoin);
    CHECK(v.currency == Currency_None);
    CHECK(v.reasonKey == Reason_AllyJoins);
    CHECK(v.band == "amistoso");

    extra.roll = KeyRolling(false, 55);
    v = DecideFreeWorld(Core(), extra, kConfig);
    CHECK(!v.willJoin);
    CHECK(v.currency == Currency_Refused);
    CHECK(v.reasonKey == Reason_AllyMaybe);
}

void TheAlliedBonusIsConfigurable()
{
    const char* name = "free world (tiers off): the allied bonus comes from the config";
    Config config = TiersOff();
    config.chance.alliedBonus = 0;
    FreeExtra extra = Free(40.0f);
    CHECK(DecideFreeWorld(Core(), extra, config).chancePercent == 45);
    config.chance.alliedBonus = 30;
    CHECK(DecideFreeWorld(Core(), extra, config).chancePercent == 75);
}

void FriendlyIsAtOrAboveTheThreshold()
{
    const char* name = "free world: friendly standing is relation >= friendlyRelation, inclusive";
    CHECK(DecideFreeWorld(Core(), Free(20.0f), TiersOff()).rolled);
    CHECK(!DecideFreeWorld(Core(), Free(19.9f), TiersOff()).rolled);
    // Balanced: the level tier answers instead of the roll (a Neutral level 20 pays).
    CHECK(DecideFreeWorld(Core(), Free(20.0f), kConfig).allyTiered);
    CHECK(!DecideFreeWorld(Core(), Free(19.9f), kConfig).allyTiered);
    CHECK(!DecideFreeWorld(Core(), Free(19.9f), kConfig).rolled);
    CHECK(FreeWorldMayRoll(Core(), Relation(20.0f), kConfig));
    CHECK(!FreeWorldMayRoll(Core(), Relation(19.9f), kConfig));
    CHECK(!FreeWorldMayRoll(Core(), RelationFact(), kConfig));
    CoreFacts animal = AnimalCore(3.0f);
    CHECK(!FreeWorldMayRoll(animal, Relation(90.0f), kConfig));
}

// ------------------------------------------------ Balanced allies by level

CoreFacts AllyCore(int npcLevel, int leaderLevel, Temperament temperament)
{
    CoreFacts core = Core();
    core.npcLevel = npcLevel;
    core.playerLevel = leaderLevel;
    core.temperament = temperament;
    return core;
}

void AGoodLowLevelAllyStillRollsAndJoinsFree()
{
    const char* name = "balanced ally: a Good ally under level 20 joins free on today's roll";
    const CoreFacts core = AllyCore(12, 12, Temperament_Honorable);
    FreeExtra extra = Free(40.0f);
    // 35 + 10 (relation) + 10 (allied) - 20 (honourable) = 35
    extra.roll = KeyRolling(true, 35);
    Verdict v = DecideFreeWorld(core, extra, kConfig);
    CHECK(v.rolled);
    CHECK(v.chancePercent == 35);
    CHECK(v.willJoin);
    CHECK(v.currency == Currency_None);
    CHECK(v.reasonKey == Reason_AllyJoins);
    CHECK(v.allyTiered);
    CHECK(v.allyLevel == 12);
    CHECK(v.allyDiscountPercent == 0);
    CHECK(!v.temporary);

    extra.roll = KeyRolling(false, 45);
    v = DecideFreeWorld(core, extra, kConfig);
    CHECK(!v.willJoin);
    CHECK(v.reasonKey == Reason_AllyMaybe);
    CHECK(v.allyTiered);
}

void APayingAllyIsQuotedThePriceAsGatheredWithNoRoll()
{
    const char* name = "balanced ally: a paying ally gets a cats quote (discount already in the price), no roll";
    const CoreFacts core = AllyCore(34, 20, Temperament_Normal);
    FreeExtra extra = Free(40.0f);
    // character_facts puts 1 - discount in the faction multiplier for this ally.
    extra.price = PriceAtLevel(34);
    extra.price.factionMultiplier = 0.80f;
    extra.roll = KeyRolling(false, 95);
    const Verdict v = DecideFreeWorld(core, extra, kConfig);
    CHECK(!v.rolled);
    CHECK(v.willJoin);
    CHECK(v.currency == Currency_Cats);
    CHECK(v.reasonKey == Reason_CatsJoin);
    // Quoted exactly as gathered: the route does not discount a second time.
    CHECK(v.cats == FinalPriceInCats(extra.price, kConfig.prices));
    CHECK(v.allyTiered);
    CHECK(v.allyLevel == 34);
    CHECK(v.allyDiscountPercent == 20);
    CHECK(!v.temporary);
}

void APayingAllyTheGoodTooFromLevelTwenty()
{
    const char* name = "balanced ally: from level 20 a Good ally pays like everyone, a short purse is a CatsAsk";
    const CoreFacts core = AllyCore(20, 20, Temperament_WarmKind);
    FreeExtra extra = Free(30.0f);
    extra.purse = Purse(10);
    const Verdict v = DecideFreeWorld(core, extra, kConfig);
    CHECK(!v.willJoin);
    CHECK(v.currency == Currency_Cats);
    CHECK(v.reasonKey == Reason_CatsAsk);
    CHECK(v.purse == 10);
    CHECK(v.allyDiscountPercent == 20);
}

void APayingAllysDiscountFollowsTheTierCapAndTheScale()
{
    const char* name = "balanced ally: 50-79 caps at 10%; the scale moves the discount";
    FreeExtra extra = Free(60.0f);
    CHECK(DecideFreeWorld(AllyCore(60, 60, Temperament_Brave), extra, kConfig).allyDiscountPercent == 10);
    Config scaled;
    scaled.allyTiers.discountScale = 0.5f;
    extra = Free(28.0f);
    CHECK(DecideFreeWorld(AllyCore(34, 34, Temperament_Normal), extra, scaled).allyDiscountPercent == 14);
}

void AStrongGoodAllyComesForAFewDays()
{
    const char* name = "balanced ally: 80+ Good/Neutral close in level join as temporary members, days from the seed";
    const CoreFacts core = AllyCore(85, 80, Temperament_Honorable);
    FreeExtra extra = Free(40.0f);
    extra.roll = KeyRolling(false, 95);
    const Verdict v = DecideFreeWorld(core, extra, kConfig);
    CHECK(v.willJoin);
    CHECK(v.currency == Currency_None);
    CHECK(v.reasonKey == Reason_AllyTemporary);
    CHECK(!v.rolled);
    CHECK(v.temporary);
    CHECK(v.allyTiered);
    CHECK(v.allyLevel == 85);
    const int expected = TemporaryDaysFromSeed(
        DailySeed(extra.roll.key, extra.roll.dayIndex, kTempDaysSalt), kConfig.allyTiers);
    CHECK(v.temporaryDays == expected);
    CHECK(v.temporaryDays >= 3 && v.temporaryDays <= 10);
    // Same key, same day: the same number of days however often they are asked.
    CHECK(DecideFreeWorld(core, extra, kConfig).temporaryDays == v.temporaryDays);

    // No key to draw from (a peek): still a temporary member, for the minimum.
    extra.roll = RollFact();
    const Verdict peek = DecideFreeWorld(core, extra, kConfig);
    CHECK(peek.temporary);
    CHECK(peek.temporaryDays == kConfig.allyTiers.tempMemberMinDays);

    CHECK(DecideFreeWorld(AllyCore(120, 110, Temperament_Dumb), Free(40.0f), kConfig).reasonKey ==
          Reason_AllyTemporary);
}

void TheStrongAlliesWhoWillNotCome()
{
    const char* name = "balanced ally: too far above the leader, cruel at 80+, greedy at 100+ -> refused";
    Verdict v = DecideFreeWorld(AllyCore(85, 60, Temperament_Honorable), Free(40.0f), kConfig);
    CHECK(!v.willJoin);
    CHECK(v.currency == Currency_Refused);
    CHECK(v.reasonKey == Reason_AllyLeaderTooWeak);
    CHECK(v.allyTiered);
    CHECK(!v.temporary);

    v = DecideFreeWorld(AllyCore(85, 85, Temperament_ColdCruel), Free(40.0f), kConfig);
    CHECK(v.currency == Currency_Refused);
    CHECK(v.reasonKey == Reason_AllyCruel);

    v = DecideFreeWorld(AllyCore(120, 120, Temperament_Smart), Free(40.0f), kConfig);
    CHECK(v.currency == Currency_Refused);
    CHECK(v.reasonKey == Reason_AllyGreedy);

    // The greedy at 80-99 still pay.
    v = DecideFreeWorld(AllyCore(90, 20, Temperament_Traitorous), Free(40.0f), kConfig);
    CHECK(v.currency == Currency_Cats);
    CHECK(v.allyDiscountPercent == 10);
}

void CheatAndTiersOffKeepTheirAllies()
{
    const char* name = "balanced ally: Cheat and tiers switched off keep the single ally roll";
    const CoreFacts strong = AllyCore(85, 20, Temperament_ColdCruel);
    FreeExtra extra = Free(40.0f);
    extra.roll = KeyRolling(true, 5);

    Config cheat;
    cheat.preset = Preset_Cheat;
    Verdict v = DecideFreeWorld(strong, extra, cheat);
    CHECK(v.rolled);
    CHECK(!v.allyTiered);
    // Cheat never reaches the free world in the game (rules.cpp answers first),
    // and DecideCheat does not read the relation at all.
    CheatFacts facts;
    CHECK(DecideCheat(facts, cheat).reasonKey == DecideCheat(facts, Config()).reasonKey);

    const Config off = TiersOff();
    v = DecideFreeWorld(strong, extra, off);
    CHECK(v.rolled);
    CHECK(!v.allyTiered);
    CHECK(v.reasonKey == Reason_AllyJoins || v.reasonKey == Reason_AllyMaybe);

    AllyTierDecision d;
    CHECK(!DecideBalancedAllyTier(strong, Relation(40.0f), cheat, &d));
    CHECK(!DecideBalancedAllyTier(strong, Relation(40.0f), off, &d));
}

void OnlyAlliesHaveATier()
{
    const char* name = "balanced ally: no tier below friendlyRelation, for an unknown relation or an animal";
    AllyTierDecision d;
    const CoreFacts core = AllyCore(34, 34, Temperament_Normal);
    CHECK(DecideBalancedAllyTier(core, Relation(20.0f), kConfig, &d));
    CHECK(d.mode == AllyTierMode_Paid);
    CHECK(!DecideBalancedAllyTier(core, Relation(19.9f), kConfig, &d));
    CHECK(!DecideBalancedAllyTier(core, RelationFact(), kConfig, &d));
    CHECK(!DecideBalancedAllyTier(AnimalCore(3.0f), Relation(90.0f), kConfig, &d));
    // A non-ally keeps the ordinary bribe, untiered.
    CHECK(!DecideFreeWorld(core, Free(0.0f), kConfig).allyTiered);
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

void AnOffendedBribeCostsMore()
{
    const char* name = "free world: an NPC offended today quotes the marked-up bribe";
    FreeExtra extra = Free(0.0f);
    extra.haggle.offended = true;
    const Verdict v = DecideFreeWorld(Core(), extra, kConfig);
    CHECK(v.currency == Currency_Cats);
    CHECK(v.cats == OffendedAsk(FinalPriceInCats(extra.price, kConfig.prices), true, kConfig.prices));
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
    // The cared-for draw is seeded from the same key a click rolls with.
    extra.care = extra.roll;
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

void ACaredForPrisonerGetsTheBonusInTheRoll()
{
    const char* name = "cage: a healthy, fed prisoner rolls with the cared-for bonus, never an auto-join";
    const CoreFacts core = Prisoner();   // unhurt, fed, full health (CoreFacts defaults)
    CageExtra extra = FailingTheRoll(core);
    const Verdict v = DecideCage(core, extra, kConfig);
    CHECK(v.rolled);
    CHECK(v.reasonKey != Reason_Intimidated);
    const int bonus = CaredForBonus(true, true, extra.care.key, extra.care.dayIndex,
                                    kConfig.submission);
    CHECK(bonus >= kConfig.submission.caredForBonusMin && bonus <= kConfig.submission.caredForBonusMax);
    CHECK(v.chanceInputs.caredFor == bonus);
    CHECK(v.chancePercent == JoinChance(v.chanceInputs, kConfig.chance, kConfig.submission));
    ChanceInputs without = v.chanceInputs;
    without.caredFor = 0;
    CHECK(JoinChance(without, kConfig.chance, kConfig.submission) + bonus == v.chancePercent);

    // Asked again the same day: the same draw.
    CHECK(DecideCage(core, extra, kConfig).chanceInputs.caredFor == bonus);
}

void AHurtOrHungryPrisonerGetsNoCaredForBonus()
{
    const char* name = "cage: wounded, starving or below full health means no cared-for bonus";
    CoreFacts hurt = Prisoner();
    hurt.wounded = true;
    CHECK(DecideCage(hurt, FailingTheRoll(hurt), kConfig).chanceInputs.caredFor == 0);
    CoreFacts hungry = Prisoner();
    hungry.starving = true;
    CHECK(DecideCage(hungry, FailingTheRoll(hungry), kConfig).chanceInputs.caredFor == 0);
    CoreFacts scratched = Prisoner();
    scratched.health = 0.9f;
    CHECK(DecideCage(scratched, FailingTheRoll(scratched), kConfig).chanceInputs.caredFor == 0);
    // No key to seed from (the card's peek of an NPC without an id): no draw either.
    CHECK(DecideCage(Prisoner(), InCage(), kConfig).chanceInputs.caredFor == 0);
}

void ALongHeldHealthyPrisonerStillRolls()
{
    const char* name = "cage: time in the cage alone decides nothing (no easy wait, P4)";
    const CoreFacts core = Prisoner();
    const Verdict v = DecideCage(core, FailingTheRoll(core), kConfig);
    CHECK(v.rolled);
    CHECK(v.chancePercent > 0 && v.chancePercent < 100);
}

void IntimidationIsAStrongArgumentNotAVerdict()
{
    const char* name = "cage: being caged and outmatched rolls level + defeats + stress + intimidation";
    CoreFacts core = Prisoner();
    CageExtra extra = InCage();
    extra.roll = KeyRolling(true, 50);
    // 35 + 15 = 50 at even levels with nothing else.
    Verdict v = DecideCage(core, extra, kConfig);
    CHECK(v.rolled);
    CHECK(v.chancePercent == 50);
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

// prisoner-module-fixes F11 (audit): a hungry enslaved prisoner gets no DARLE
// COMIDA because freedom is chosen before hunger. That is the documented rule,
// not a bug (QA G3 "Liberar a un esclavo: se une"; route_cage.cpp: someone freed
// from slavers does not need convincing): starving or not, they join, so there is
// no refusal for food to answer. Pinned here so the precedence is not "fixed" by
// accident; the Slave archetype follows the same rule.
void AStarvingSlaveStillFollowsWithoutFood()
{
    const char* name = "cage: a starving slave joins on freedom, before any food remedy";
    CoreFacts core = Prisoner();
    core.enslaved = true;
    core.starving = true;
    Verdict v = DecideCage(core, FailingTheRoll(core), kConfig);
    CHECK(v.willJoin);
    CHECK(v.currency == Currency_Freedom);
    CHECK(v.reasonKey == Reason_FreedFollows);

    core.enslaved = false;
    core.archetype = Archetype_Slave;
    core.wounded = true;
    v = DecideCage(core, FailingTheRoll(core), kConfig);
    CHECK(v.willJoin);
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

CoreFacts CagedAnimal(float hunger)
{
    CoreFacts core = Prisoner();
    core.animal = true;
    core.hunger = hunger;
    core.starving = hunger < 1.0f;
    return core;
}

CageExtra CagedBeast(const AnimalExtra& animal)
{
    CageExtra extra = FailingTheRoll(Prisoner());
    extra.animal = animal;
    return extra;
}

void AnAnimalInTheCageIsTamedByTheSameBar()
{
    const char* name = "cage: a caged animal settles on the taming bar, hungry or full";
    Verdict v = DecideCage(CagedAnimal(0.0f), CagedBeast(Beast()), kConfig);
    CHECK(v.isAnimal);
    CHECK(v.wantsFood);
    CHECK(v.reasonKey == Reason_TameHungry);

    // Healthy, fed by the player and full: no "wary" branch any more, it joins.
    v = DecideCage(CagedAnimal(3.0f), CagedBeast(Beast()), kConfig);
    CHECK(v.willJoin);
    CHECK(v.currency == Currency_Care);
    CHECK(v.reasonKey == Reason_TameReady);
}

void ACagedUnfedAnimalIsNotTamedByTime()
{
    const char* name = "cage: an unfed animal is not tamed, and gets no cared-for bonus";
    CageExtra extra = CagedBeast(Unfed());
    const Verdict v = DecideCage(CagedAnimal(3.0f), extra, kConfig);
    CHECK(!v.willJoin);
    CHECK(v.isAnimal);
    CHECK(v.reasonKey == Reason_TameNotFed);
    CHECK(v.chanceInputs.caredFor == 0);
}

void AnimalsAreNeverIntimidated()
{
    const char* name = "cage: an animal is never intimidated into joining";
    // A key that joins at 1%: any roll the cage made would pass.
    CageExtra extra = CagedBeast(Beast());
    extra.roll = KeyRolling(true, 1);
    const Verdict v = DecideCage(CagedAnimal(1.5f), extra, kConfig);
    CHECK(!v.willJoin);
    CHECK(!v.rolled);
    CHECK(v.reasonKey == Reason_TameHungry);

    CageExtra unfed = CagedBeast(Unfed());
    unfed.roll = KeyRolling(true, 1);
    const Verdict full = DecideCage(CagedAnimal(3.0f), unfed, kConfig);
    CHECK(!full.willJoin);
    CHECK(full.reasonKey == Reason_TameNotFed);
}

void ACagedAnimalHasNoSaviourShortcut()
{
    const char* name = "cage: remembering the player as its saviour does not tame an unfed animal";
    CoreFacts core = CagedAnimal(3.0f);
    core.rememberedAsSaviour = true;
    const Verdict v = DecideCage(core, CagedBeast(Unfed()), kConfig);
    CHECK(!v.willJoin);
    CHECK(v.reasonKey == Reason_TameNotFed);
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

void AnOffendedPrisonerCostsMore()
{
    const char* name = "cage: a prisoner offended today quotes the marked-up price";
    CoreFacts core = Prisoner();
    CageExtra extra = FailingTheRoll(core);
    extra.haggle.offended = true;
    const Verdict v = DecideCage(core, extra, kConfig);
    CHECK(v.currency == Currency_Cats);
    CHECK(v.cats == OffendedAsk(FinalPriceInCats(extra.price, kConfig.prices), true, kConfig.prices));
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

    // Fed, unwounded, full health, in the cage: the cage route rolls (no shortcut, P4).
    CoreFacts patched = Core();
    patched.inMyCage = true;
    patched.health = 1.0f;
    CHECK(ChooseEntryRoute(MakeRouteFacts(patched, kConfig)) == Route_Cage);
    patched.health = 0.9f;
    CHECK(ChooseEntryRoute(MakeRouteFacts(patched, kConfig)) == Route_Cage);
}
} // namespace

// ------------------------------------------------- animal field yield (D3)

// A free animal beaten twice: Honorable and Military on paper, both ignored.
CoreFacts BeatenAnimal()
{
    CoreFacts core = Core();
    core.animal = true;
    core.defeats = 2;
    core.temperament = Temperament_Honorable;
    core.archetype = Archetype_Military;
    return core;
}

void AnAnimalYieldsAtAPlainBeatingChance()
{
    const char* name = "animal yield: the field-yield chance of a Normal, no loyalty penalty";
    // The same 69 a Normal bandit gets: 35 + 24 + 0 + 10.
    YieldExtra yield;
    yield.roll = KeyRolling(true, 69);
    const Verdict v = DecideAnimalFieldYield(BeatenAnimal(), yield, Unfed(), kConfig);
    CHECK(v.rolled);
    CHECK(v.chancePercent == 69);
    CHECK(v.chanceInputs.personality == Temperament_Normal);
    CHECK(v.chanceInputs.situation == kConfig.submission.fieldYieldBonus);
    CHECK(v.willJoin);
    CHECK(v.isAnimal);
    CHECK(v.currency == Currency_Submission);
    CHECK(v.reasonKey == Reason_AnimalYieldJoins);
}

void AnAnimalThatDoesNotYieldCanStillBeFed()
{
    const char* name = "animal yield: a failed roll keeps food as the other way in";
    CoreFacts core = BeatenAnimal();
    core.hunger = 1.5f;               // half a stomach: more stress, a higher chance
    const int chance = DecideAnimalFieldYield(core, YieldExtra(), Unfed(), kConfig).chancePercent;
    CHECK(chance > 69 && chance < 100);
    YieldExtra yield;
    yield.roll = KeyRolling(false, chance);
    const Verdict v = DecideAnimalFieldYield(core, yield, Unfed(), kConfig);
    CHECK(v.rolled);
    CHECK(v.chancePercent == chance);
    CHECK(!v.willJoin);
    CHECK(v.isAnimal);
    CHECK(v.reasonKey == Reason_AnimalYieldNotYet);
    // The feed button needs care with an appetite; the stomach bar rides along.
    CHECK(v.currency == Currency_Care);
    CHECK(v.wantsFood);
    CHECK(v.tameNeeded > 0.0f);
    CHECK(v.tameFullness > 0.499f && v.tameFullness < 0.501f);
}

void ATamedAnimalJoinsEvenWhenTheRollFails()
{
    const char* name = "animal yield: fed past the bar it follows you whatever the roll said";
    YieldExtra yield;
    yield.roll = KeyRolling(false, 79);
    const Verdict v = DecideAnimalFieldYield(BeatenAnimal(), yield, Beast(), kConfig);
    CHECK(v.willJoin);
    CHECK(v.reasonKey == Reason_TameReady);
}

void AnAnimalInAnotherCageIsNotYoursToBeat()
{
    const char* name = "animal yield: another faction's prisoner is not asked, beaten or not";
    YieldExtra yield;
    yield.roll = KeyRolling(true, 79);
    yield.access.inAnyCage = true;
    const Verdict v = DecideAnimalFieldYield(BeatenAnimal(), yield, Beast(), kConfig);
    CHECK(!v.willJoin);
    CHECK(!v.rolled);
    CHECK(v.reasonKey == Reason_InOthersCage);
}

void AnAnimalNeverHearsTheInsistenceLockout()
{
    const char* name = "animal yield: a stale pestering lockout never stops an animal";
    YieldExtra yield;
    yield.roll = KeyRolling(true, 79);
    yield.access.ignoringPlayer = true;
    const Verdict v = DecideAnimalFieldYield(BeatenAnimal(), yield, Unfed(), kConfig);
    CHECK(v.rolled);
    CHECK(v.reasonKey == Reason_AnimalYieldJoins);
}

void RunRouteDecisionsTests()
{
    CheatAHealedAnimalJoinsWithoutTheCage();
    CheatADefeatedAnimalJoinsWhenSpokenTo();
    CheatTheMealIsNamedFirst();
    CheatAnOwnedAnimalIsStillCageOnly();
    CheatAPersonIsNotMovedByACureOrABeating();
    AnAnimalYieldsAtAPlainBeatingChance();
    AnAnimalThatDoesNotYieldCanStillBeFed();
    ATamedAnimalJoinsEvenWhenTheRollFails();
    AnAnimalInAnotherCageIsNotYoursToBeat();
    AnAnimalNeverHearsTheInsistenceLockout();
    TheTamingBarTravelsOnTheVerdict();
    AnimalsSwitchedOffStopsAnAnimal();
    UnreadableTraitsFallBackToAPlainRecruit();
    AFakerAndACorpseAreNotRecruitable();
    TheGateChecksInTheShippedOrder();
    AnOrdinaryCharacterPassesTheGate();
    CheatWithoutACageJoinsAtOnce();
    CheatAsksForTheCage();
    CheatWaitsAMinuteThenJoins();
    CheatAnyCageWaitsAMinuteThenJoins();
    CheatAnyCageOffKeepsYourCageOnly();
    CheatAnyCageStillNeedsACage();
    CheatAnyCageAsksForAnyCage();
    CheatCageCountsRule();
    CheatAnyCageHoldClockRule();
    TheHoldClockCountsThisCage();
    CheatAFedAnimalJoinsWithoutTheCage();
    CheatAFedAnimalInTheCageDoesNotWait();
    CheatAnUnfedAnimalOutsideStillNeedsTheCage();
    CheatAnUnfedAnimalInTheCageWaitsTheMinute();
    CheatAnOwnedAnimalOutsideNeedsTheCageFedOrNot();
    CheatFoodMeansNothingToAPerson();
    CheatWithoutACageIsUnchangedForAnimals();
    OutColdAndOwingWaitsForThemToWake();
    OutColdAndDyingWantsCare();
    OutColdAndNothingElseJustWaits();
    DyingIsARescueRoute();
    ASavedLifeRollsAtTheSavedLifeChance();
    ARescuedOneWhoCannotFollowPaysItForward();
    HealingThemMoreArguesMore();
    WithoutAKeyARescueIsNotAGiven();
    RescuePeekAndDecideAgreeForAKeyedNpc();
    ARollNeedsTheKeyAndTheClock();
    FieldYieldRollsAtTheBeatingChance();
    FieldYieldNotYetNamesTheWayOut();
    FieldYieldReadsStressAndUniqueButNotTheGrudge();
    FieldYieldRefusesSomeoneInAnotherCage();
    FieldYieldRefusesUnderThePesteringLockout();
    FieldYieldLoyaltyIsAPenaltyNotASkip();
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
    AGoodLowLevelAllyStillRollsAndJoinsFree();
    APayingAllyIsQuotedThePriceAsGatheredWithNoRoll();
    APayingAllyTheGoodTooFromLevelTwenty();
    APayingAllysDiscountFollowsTheTierCapAndTheScale();
    AStrongGoodAllyComesForAFewDays();
    TheStrongAlliesWhoWillNotCome();
    CheatAndTiersOffKeepTheirAllies();
    OnlyAlliesHaveATier();
    TheGoodNaturedStopTalkingBelowTheirCutoff();
    NobodyTalksBelowTheSecondCutoff();
    TheCutoffsAreConfigurable();
    TheGoodNaturedNeverTakeCats();
    TheMilitaryServeTheirFaction();
    EveryoneElseHasAPrice();
    ACageRouteSwitchedOffIsDisabled();
    ACaredForPrisonerGetsTheBonusInTheRoll();
    AHurtOrHungryPrisonerGetsNoCaredForBonus();
    ALongHeldHealthyPrisonerStillRolls();
    IntimidationIsAStrongArgumentNotAVerdict();
    TheCageRollCountsTheGrudgeAndTheName();
    ASlaveIsFreedNotBought();
    AStarvingSlaveStillFollowsWithoutFood();
    TheHurtAndHungryWantCare();
    TheKindAreCaredForAndJoin();
    AnAnimalInTheCageIsTamedByTheSameBar();
    ACagedUnfedAnimalIsNotTamedByTime();
    AnimalsAreNeverIntimidated();
    ACagedAnimalHasNoSaviourShortcut();
    AFullUnfedWildAnimalRefuses();
    AFedFullAnimalJoins();
    FeedingAFullUnfedAnimalIsOffered();
    AnUnfedHungryAnimalNamesTheGap();
    ASoldierIsBrokenNotBought();
    AZealotHasNoPrice();
    ABanditKingHasAPrice();
    AnOrdinaryPrisonerIsQuotedCats();
    TheCurrencyTable();
    WhoTradesAmongTheNamed();
    RouteFactsAreBuiltFromTheCoreAndTheConfig();
    AnOffendedNpcQuotesTheMarkedUpAsk();
    AnOffendedBribeCostsMore();
    AnOffendedPrisonerCostsMore();
}
