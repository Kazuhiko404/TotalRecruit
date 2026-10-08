// Characterization tests for src/chance_rules.cpp: they pin the numbers the
// plugin shipped with (config defaults), so a later change to any term shows up
// as a failing test instead of a quiet shift in game balance.

#include "test_support.h"
#include "chance_rules.h"

#include <cstdio>

using namespace totalrecruit;

namespace
{
const Chance kChance = Chance();
const Submission kSub = Submission();

// ---------------------------------------------------------------- level gap

void LevelGapEqualStartsAtBase()
{
    const char* name = "level gap: equal levels start at levelBase";
    CHECK(ChanceFromLevelGap(20, 20, kChance) == 35);
}

void LevelGapStrongerNpcCostsPerLevel()
{
    const char* name = "level gap: each level the NPC has over the player costs 1.5";
    CHECK(ChanceFromLevelGap(30, 20, kChance) == 20);
    CHECK(ChanceFromLevelGap(21, 20, kChance) == 34);   // 35 - (int)1.5 = 34
}

void LevelGapNeverBelowFloor()
{
    const char* name = "level gap: a hopeless gap bottoms out at levelGapFloor";
    CHECK(ChanceFromLevelGap(200, 1, kChance) == 2);
}

void LevelGapWeakerNpcPaysUpToCap()
{
    const char* name = "level gap: out-levelling pays per level, capped";
    CHECK(ChanceFromLevelGap(20, 30, kChance) == 50);
    CHECK(ChanceFromLevelGap(1, 200, kChance) == 75);   // 35 + cap 40
}

// ----------------------------------------------------------------- relation

void RelationIsAQuarterOrNothing()
{
    const char* name = "relation: a quarter of the value, zero when unknown";
    CHECK(ChanceFromRelation(100.0f, true, kChance) == 25);
    CHECK(ChanceFromRelation(-100.0f, true, kChance) == -25);
    CHECK(ChanceFromRelation(40.0f, false, kChance) == 0);
}

// -------------------------------------------------------------- personality

void PersonalityTable()
{
    const char* name = "personality: the shipped offsets";
    CHECK(ChanceFromPersonality(Temperament_Fearful, kChance) == 20);
    CHECK(ChanceFromPersonality(Temperament_Traitorous, kChance) == 15);
    CHECK(ChanceFromPersonality(Temperament_Dumb, kChance) == 10);
    CHECK(ChanceFromPersonality(Temperament_Normal, kChance) == 0);
    CHECK(ChanceFromPersonality(Temperament_WarmKind, kChance) == 0);
    CHECK(ChanceFromPersonality(Temperament_ColdCruel, kChance) == 0);
    CHECK(ChanceFromPersonality(Temperament_Smart, kChance) == -5);
    CHECK(ChanceFromPersonality(Temperament_Brave, kChance) == -15);
    CHECK(ChanceFromPersonality(Temperament_Honorable, kChance) == -20);
    CHECK(ChanceFromPersonality(Temperament_Maniacal, kChance) == -25);
}

// ------------------------------------------------------------------ defeats

void DefeatsScaleByResolveAndCap()
{
    const char* name = "defeats: 12 per defeat divided by resolve, capped at 45";
    CHECK(ChanceFromDefeats(0, Temperament_Normal, kSub) == 0);
    CHECK(ChanceFromDefeats(2, Temperament_Normal, kSub) == 24);
    CHECK(ChanceFromDefeats(2, Temperament_Honorable, kSub) == 13);   // 24 / 1.8
    CHECK(ChanceFromDefeats(2, Temperament_Fearful, kSub) == 40);     // 24 / 0.6
    CHECK(ChanceFromDefeats(10, Temperament_Normal, kSub) == 45);
}

void DefeatsIgnoredWhenCountingIsOff()
{
    const char* name = "defeats: countDefeats off makes the term zero";
    Submission sub;
    sub.countDefeats = false;
    CHECK(ChanceFromDefeats(5, Temperament_Normal, sub) == 0);
}

// ------------------------------------------------------------------- stress

void StressScalesByResolveAndCap()
{
    const char* name = "stress: full stress argues for maxStressChance, divided by resolve";
    CHECK(ChanceFromStress(0.0f, Temperament_Normal, kSub) == 0);
    CHECK(ChanceFromStress(1.0f, Temperament_Normal, kSub) == 30);
    CHECK(ChanceFromStress(0.5f, Temperament_Normal, kSub) == 15);
    CHECK(ChanceFromStress(1.0f, Temperament_Brave, kSub) == 18);     // 30 / 1.6
    CHECK(ChanceFromStress(1.0f, Temperament_Fearful, kSub) == 30);   // capped
}

void HungerRunsZeroToThree()
{
    const char* name = "hunger: raw 0..3 maps to fullness 0..1 and clamps";
    CHECK(HungerToFullness01(3.0f) == 1.0f);
    CHECK(HungerToFullness01(0.0f) == 0.0f);
    CHECK(HungerToFullness01(1.5f) == 0.5f);
    CHECK(HungerToFullness01(9.0f) == 1.0f);
    CHECK(HungerToFullness01(-1.0f) == 0.0f);
}

void StressFromStateWeights()
{
    const char* name = "stress from state: hunger 0.5, wounds 0.35, exhaustion 0.15, beatings as they are";
    // Fed, unhurt, fully rested, never beaten: nothing wears them down.
    CHECK(StressFromState(3.0f, 0.0f, 1.0f, 0.0f, kSub) == 0.0f);
    // Starving alone is half the bar.
    CHECK(StressFromState(0.0f, 0.0f, 1.0f, 0.0f, kSub) == 0.5f);
    // Everything wrong at once is the whole bar, and no more.
    CHECK(StressFromState(0.0f, 1.0f, 0.0f, 0.0f, kSub) == 1.0f);
    CHECK(StressFromState(0.0f, 5.0f, -3.0f, 0.0f, kSub) == 1.0f);
    // The beating stress is the fourth term, already 0..1 (beating_stress_rules.h).
    CHECK(StressFromState(3.0f, 0.0f, 1.0f, 0.3f, kSub) == 0.3f);
    CHECK(StressFromState(0.0f, 0.0f, 1.0f, 0.25f, kSub) == 0.75f);
    CHECK(StressFromState(0.0f, 0.0f, 1.0f, 0.9f, kSub) == 1.0f);
    CHECK(StressFromState(3.0f, 0.0f, 1.0f, -2.0f, kSub) == 0.0f);
    CHECK(StressFromState(3.0f, 0.0f, 1.0f, 7.0f, kSub) == 1.0f);
}

// ----------------------------------------------------------------- grudge

void GrudgeIsAbsoluteOnlyForTheLoyal()
{
    const char* name = "grudge: honourable, brave and kind never forgive";
    CHECK(GrudgeIsUnforgivable(Temperament_Honorable, kChance));
    CHECK(GrudgeIsUnforgivable(Temperament_Brave, kChance));
    CHECK(GrudgeIsUnforgivable(Temperament_WarmKind, kChance));
    CHECK(!GrudgeIsUnforgivable(Temperament_Normal, kChance));
    CHECK(!GrudgeIsUnforgivable(Temperament_Traitorous, kChance));
    CHECK(!GrudgeIsUnforgivable(Temperament_ColdCruel, kChance));
}

void GrudgeAbsoluteCanBeSwitchedOff()
{
    const char* name = "grudge: grudgeIsAbsolute off makes it a weight for everyone";
    Chance chance;
    chance.grudgeIsAbsolute = false;
    CHECK(!GrudgeIsUnforgivable(Temperament_Honorable, chance));
}

// ------------------------------------------------------------- JoinChance

void JoinChanceSumsTheTerms()
{
    const char* name = "join chance: level + relation + personality + defeats + stress + situation";
    ChanceInputs in;
    in.npcLevel = 20;
    in.playerLevel = 20;                    // 35
    in.relation = 40.0f;
    in.haveRelation = true;                 // +10
    in.personality = Temperament_Normal;    // +0
    in.defeats = 1;                         // +12
    in.stress = 0.0f;
    in.situation = 5;
    CHECK(JoinChance(in, kChance, kSub) == 62);   // 35+10+0+12+5
}

void JoinChanceSubtractsGrudgeAndUnique()
{
    const char* name = "join chance: grudge -20 and unique -25";
    ChanceInputs in;
    in.npcLevel = 20;
    in.playerLevel = 20;
    CHECK(JoinChance(in, kChance, kSub) == 35);
    in.grudge = true;
    CHECK(JoinChance(in, kChance, kSub) == 15);
    in.unique = true;
    CHECK(JoinChance(in, kChance, kSub) == 5);   // 0 clamped to the floor
}

void JoinChanceIsNeverCertain()
{
    const char* name = "join chance: clamped to [5, 95]";
    ChanceInputs hopeless;
    hopeless.npcLevel = 300;
    hopeless.playerLevel = 1;
    hopeless.personality = Temperament_Maniacal;
    hopeless.grudge = true;
    hopeless.unique = true;
    CHECK(JoinChance(hopeless, kChance, kSub) == 5);

    ChanceInputs sure;
    sure.npcLevel = 1;
    sure.playerLevel = 300;
    sure.personality = Temperament_Fearful;
    sure.situation = 100;
    CHECK(JoinChance(sure, kChance, kSub) == 95);
}

// ------------------------------------------------------------- StableRoll

void StableRollHonoursTheExtremes()
{
    const char* name = "stable roll: 100+ always, 0- never";
    CHECK(StableRoll(7, 3, 100));
    CHECK(StableRoll(7, 3, 250));
    CHECK(!StableRoll(7, 3, 0));
    CHECK(!StableRoll(7, 3, -4));
}

void StableRollNeverRerolls()
{
    const char* name = "stable roll: the same handle and percent always answer the same";
    for (unsigned int i = 1; i < 200; ++i)
    {
        const bool first = StableRoll(i, i * 3u, 37);
        CHECK(StableRoll(i, i * 3u, 37) == first);
        CHECK(StableRoll(i, i * 3u, 37) == first);
    }
}

void StableRollIsMonotonicInThePercent()
{
    const char* name = "stable roll: a handle that passes at p passes at every higher p";
    for (unsigned int i = 1; i < 300; ++i)
    {
        bool passed = false;
        for (int p = 1; p < 100; ++p)
        {
            const bool ok = StableRoll(i, 11u, p);
            if (passed)
            {
                CHECK(ok);
            }
            passed = passed || ok;
        }
    }
}

void StableRollApproximatesThePercent()
{
    const char* name = "stable roll: about 50% of handles pass at 50%";
    int passes = 0;
    const int total = 2000;
    for (int i = 1; i <= total; ++i)
    {
        if (StableRoll((unsigned int)i, 5u, 50))
        {
            ++passes;
        }
    }
    CHECK(passes > total * 40 / 100);
    CHECK(passes < total * 60 / 100);
}

// ------------------------------------------- StableRoll seeded from an id
// A persisted id seeds the roll, so an NPC keeps their luck across a relaunch.

CharKey IdKey(const char* format, int i)
{
    char id[96];
    _snprintf_s(id, sizeof(id), _TRUNCATE, format, i);
    return CharKey::FromId(id);
}

void IdRollNeverRerolls()
{
    const char* name = "stable roll (id): the same id and percent always answer the same";
    for (int i = 0; i < 200; ++i)
    {
        const CharKey key = IdKey("0a1b2c3d-%04d-4e5f-8a9b-c0d1e2f3a4b5", i);
        const bool first = StableRoll(key, 37);
        CHECK(StableRoll(key, 37) == first);
        // A rebuilt key, as after a relaunch.
        CHECK(StableRoll(IdKey("0a1b2c3d-%04d-4e5f-8a9b-c0d1e2f3a4b5", i), 37) == first);
    }
}

void IdRollHonoursTheExtremes()
{
    const char* name = "stable roll (id): 100+ always, 0- never";
    const CharKey key = CharKey::FromId("aaaa-bbbb");
    CHECK(StableRoll(key, 100));
    CHECK(StableRoll(key, 300));
    CHECK(!StableRoll(key, 0));
    CHECK(!StableRoll(key, -2));
}

void IdRollIsMonotonicInThePercent()
{
    const char* name = "stable roll (id): an id that passes at p passes at every higher p";
    for (int i = 0; i < 300; ++i)
    {
        const CharKey key = IdKey("id-%d", i);
        bool passed = false;
        for (int p = 1; p < 100; ++p)
        {
            const bool ok = StableRoll(key, p);
            if (passed)
            {
                CHECK(ok);
            }
            passed = passed || ok;
        }
    }
}

void IdRollApproximatesThePercent()
{
    const char* name = "stable roll (id): about 50% of ids pass at 50%, and 25% at 25%";
    int half = 0;
    int quarter = 0;
    const int total = 4000;
    for (int i = 0; i < total; ++i)
    {
        const CharKey key = IdKey("npc-%d-4000-8000-000000000000", i);
        if (StableRoll(key, 50)) ++half;
        if (StableRoll(key, 25)) ++quarter;
    }
    CHECK(half > total * 45 / 100 && half < total * 55 / 100);
    CHECK(quarter > total * 20 / 100 && quarter < total * 30 / 100);
}

void IdRollSpreadsNeighbouringIds()
{
    const char* name = "stable roll (id): ids that differ in one character do not roll alike";
    int agree = 0;
    const int total = 1000;
    for (int i = 0; i < total; ++i)
    {
        if (StableRoll(IdKey("npc-%d", i), 50) == StableRoll(IdKey("npc-%d", i + 1), 50)) ++agree;
    }
    CHECK(agree > total * 35 / 100 && agree < total * 65 / 100);   // independent coins agree about half the time
}

void HandleKeyRollsAsBefore()
{
    const char* name = "stable roll (key): a handle-only key rolls exactly as the (index, serial) form";
    for (unsigned int i = 1; i < 200; ++i)
    {
        CHECK(StableRoll(CharKey(i, i * 3u), 37) == StableRoll(i, i * 3u, 37));
        CHECK(StableRoll(CharKey(i, 11u), 60) == StableRoll(i, 11u, 60));
    }
}

// --- T8: the relation weight and the personality table moved into the config.

void RelationWeightIsConfigurable()
{
    const char* name = "relation: the weight comes from the config";
    Chance chance;
    chance.relationWeight = 0.5f;
    CHECK(ChanceFromRelation(100.0f, true, chance) == 50);
    CHECK(ChanceFromRelation(-40.0f, true, chance) == -20);
    chance.relationWeight = 0.0f;
    CHECK(ChanceFromRelation(100.0f, true, chance) == 0);
    CHECK(ChanceFromRelation(100.0f, false, chance) == 0);
}

void PersonalityTableIsConfigurable()
{
    const char* name = "personality: every offset comes from the config table";
    Chance chance;
    chance.personality[Temperament_Fearful]    = 1;
    chance.personality[Temperament_Traitorous] = 2;
    chance.personality[Temperament_Dumb]       = 3;
    chance.personality[Temperament_Normal]     = 4;
    chance.personality[Temperament_Smart]      = 5;
    chance.personality[Temperament_Brave]      = 6;
    chance.personality[Temperament_Honorable]  = 7;
    chance.personality[Temperament_Maniacal]   = 8;
    chance.personality[Temperament_ColdCruel]  = 9;
    chance.personality[Temperament_WarmKind]   = 10;
    CHECK(ChanceFromPersonality(Temperament_Fearful, chance) == 1);
    CHECK(ChanceFromPersonality(Temperament_Traitorous, chance) == 2);
    CHECK(ChanceFromPersonality(Temperament_Dumb, chance) == 3);
    CHECK(ChanceFromPersonality(Temperament_Normal, chance) == 4);
    CHECK(ChanceFromPersonality(Temperament_Smart, chance) == 5);
    CHECK(ChanceFromPersonality(Temperament_Brave, chance) == 6);
    CHECK(ChanceFromPersonality(Temperament_Honorable, chance) == 7);
    CHECK(ChanceFromPersonality(Temperament_Maniacal, chance) == 8);
    CHECK(ChanceFromPersonality(Temperament_ColdCruel, chance) == 9);
    CHECK(ChanceFromPersonality(Temperament_WarmKind, chance) == 10);
}

void PersonalityDefaultsAreTheShippedTable()
{
    const char* name = "personality: the constructor holds the shipped offsets, 0 for the rest";
    const Chance chance;
    CHECK(chance.personality[Temperament_Fearful] == 20);
    CHECK(chance.personality[Temperament_Maniacal] == -25);
    CHECK(chance.personality[Temperament_Normal] == 0);
    CHECK(chance.personality[Temperament_ColdCruel] == 0);
    CHECK(chance.personality[Temperament_WarmKind] == 0);
    CHECK(chance.relationWeight == 0.25f);
    CHECK(chance.alliedBonus == 10);
}

void JoinChanceUsesTheConfiguredTerms()
{
    const char* name = "join chance: a configured weight and table reach the total";
    ChanceInputs in;
    in.npcLevel = in.playerLevel = 20;     // +35
    in.relation = 80.0f;
    in.haveRelation = true;
    in.personality = Temperament_Brave;
    Chance chance;
    CHECK(JoinChance(in, chance, kSub) == 35 + 20 - 15);
    chance.relationWeight = 0.5f;          // +40 instead of +20
    chance.personality[Temperament_Brave] = 0;
    CHECK(JoinChance(in, chance, kSub) == 35 + 40);
}

// --- T8: the verdict roll, moved out of rules.cpp (StableChance) unchanged.

void TheVerdictRollHonoursTheExtremesWithoutAKey()
{
    const char* name = "verdict roll: 100 always joins, 0 never does, even with no key";
    const CharKey none;
    CHECK(RollsToJoin(false, none, 3, 100));
    CHECK(RollsToJoin(false, none, 3, 150));
    CHECK(!RollsToJoin(false, none, 3, 0));
    CHECK(!RollsToJoin(false, none, 3, -5));
}

void TheVerdictRollRefusesWhenThereIsNoKey()
{
    const char* name = "verdict roll: with no key to seed from the answer is no";
    const CharKey none;
    for (int percent = 1; percent < 100; ++percent)
    {
        CHECK(!RollsToJoin(false, none, 3, percent));
    }
}

void TheVerdictRollIsTheDailyJoinRoll()
{
    const char* name = "verdict roll: with a key it is the daily roll under the join salt";
    for (unsigned int i = 1; i < 200; ++i)
    {
        const CharKey key(i, i * 7u);
        for (int percent = 1; percent < 100; percent += 11)
        {
            CHECK(RollsToJoin(true, key, 12, percent) == DailyRoll(key, 12, kJoinRollSalt, percent));
            CHECK(RollsToJoin(true, IdKey("v-%d", (int)i), 12, percent) ==
                  DailyRoll(IdKey("v-%d", (int)i), 12, kJoinRollSalt, percent));
        }
    }
}

// --- haggle-and-daily-roll T1: the roll re-seeds once per in-game day.

void DayIndexIsWholeGameDays()
{
    const char* name = "day index: floor(game hours / 24)";
    CHECK(DayIndexFromHours(0.0) == 0);
    CHECK(DayIndexFromHours(23.99) == 0);
    CHECK(DayIndexFromHours(24.0) == 1);
    CHECK(DayIndexFromHours(47.5) == 1);
    CHECK(DayIndexFromHours(24.0 * 400 + 3.0) == 400);
    // An older save or a clock that has not started: never a positive day.
    CHECK(DayIndexFromHours(-1.0) == -1);
}

void DailyRollNeverRerollsWithinADay()
{
    const char* name = "daily roll: the same id and day always answer the same";
    for (int i = 0; i < 200; ++i)
    {
        for (int day = 0; day < 5; ++day)
        {
            const bool first = RollsToJoin(true, IdKey("same-%d", i), day, 37);
            CHECK(RollsToJoin(true, IdKey("same-%d", i), day, 37) == first);
            CHECK(RollsToJoin(true, IdKey("same-%d", i), day, 37) == first);
        }
    }
}

void DailyRollHonoursTheExtremes()
{
    const char* name = "daily roll: 0% never, 100% always, on every day";
    for (int day = 0; day < 60; ++day)
    {
        CHECK(!RollsToJoin(true, IdKey("edge-%d", day), day, 0));
        CHECK(RollsToJoin(true, IdKey("edge-%d", day), day, 100));
        CHECK(!DailyRoll(CharKey(5u, 9u), day, kJoinRollSalt, 0));
        CHECK(DailyRoll(CharKey(5u, 9u), day, kJoinRollSalt, 100));
    }
}

void DailyRollPassesAboutHalfTheDaysAtFifty()
{
    const char* name = "daily roll: a 50% NPC passes on roughly half of 30 days";
    int total = 0;
    for (int i = 0; i < 100; ++i)
    {
        int passes = 0;
        for (int day = 0; day < 30; ++day)
        {
            if (RollsToJoin(true, IdKey("npc-%d-4000-8000", i), day, 50)) ++passes;
        }
        // Deterministic for these ids; a fair coin over 30 days lands well inside this.
        CHECK(passes >= 5 && passes <= 25);
        total += passes;
    }
    CHECK(total > 3000 * 45 / 100 && total < 3000 * 55 / 100);
}

void DailyRollHandleKeyAlsoReseeds()
{
    const char* name = "daily roll: a handle-only key re-seeds per day as well";
    int passes = 0;
    for (int day = 0; day < 200; ++day)
    {
        if (RollsToJoin(true, CharKey(42u, 7u), day, 50)) ++passes;
    }
    CHECK(passes > 200 * 35 / 100 && passes < 200 * 65 / 100);
}

void DailyRollDifferentIdsAreIndependent()
{
    const char* name = "daily roll: different ids on the same day do not roll alike";
    int agree = 0;
    const int total = 1000;
    for (int i = 0; i < total; ++i)
    {
        if (RollsToJoin(true, IdKey("npc-%d", i), 9, 50) ==
            RollsToJoin(true, IdKey("npc-%d", i + 1), 9, 50)) ++agree;
    }
    CHECK(agree > total * 40 / 100 && agree < total * 60 / 100);
}

void DailyRollConsecutiveDaysAreIndependent()
{
    const char* name = "daily roll: the same id on consecutive days does not roll alike";
    int agree = 0;
    const int total = 1000;
    for (int i = 0; i < total; ++i)
    {
        const CharKey key = IdKey("walker-%d", i);
        if (RollsToJoin(true, key, i, 50) == RollsToJoin(true, key, i + 1, 50)) ++agree;
    }
    CHECK(agree > total * 40 / 100 && agree < total * 60 / 100);
}

void DailyRollSaltSeparatesDraws()
{
    const char* name = "daily roll: a different salt is a different draw on the same day";
    int agree = 0;
    const int total = 1000;
    for (int i = 0; i < total; ++i)
    {
        const CharKey key = IdKey("salted-%d", i);
        if (DailyRoll(key, 4, kJoinRollSalt, 50) == DailyRoll(key, 4, kJoinRollSalt + 1u, 50)) ++agree;
    }
    CHECK(agree > total * 40 / 100 && agree < total * 60 / 100);
}

void DailyRollIsMonotonicInThePercent()
{
    const char* name = "daily roll: an id that passes at p on a day passes at every higher p";
    for (int i = 0; i < 200; ++i)
    {
        bool passed = false;
        for (int p = 1; p < 100; ++p)
        {
            const bool ok = RollsToJoin(true, IdKey("mono-%d", i), i % 7, p);
            if (passed)
            {
                CHECK(ok);
            }
            passed = passed || ok;
        }
    }
}
} // namespace

void RunChanceRulesTests()
{
    LevelGapEqualStartsAtBase();
    LevelGapStrongerNpcCostsPerLevel();
    LevelGapNeverBelowFloor();
    LevelGapWeakerNpcPaysUpToCap();
    RelationIsAQuarterOrNothing();
    PersonalityTable();
    DefeatsScaleByResolveAndCap();
    DefeatsIgnoredWhenCountingIsOff();
    StressScalesByResolveAndCap();
    HungerRunsZeroToThree();
    StressFromStateWeights();
    GrudgeIsAbsoluteOnlyForTheLoyal();
    GrudgeAbsoluteCanBeSwitchedOff();
    JoinChanceSumsTheTerms();
    JoinChanceSubtractsGrudgeAndUnique();
    JoinChanceIsNeverCertain();
    StableRollHonoursTheExtremes();
    StableRollNeverRerolls();
    StableRollIsMonotonicInThePercent();
    StableRollApproximatesThePercent();
    IdRollNeverRerolls();
    IdRollHonoursTheExtremes();
    IdRollIsMonotonicInThePercent();
    IdRollApproximatesThePercent();
    IdRollSpreadsNeighbouringIds();
    HandleKeyRollsAsBefore();
    RelationWeightIsConfigurable();
    PersonalityTableIsConfigurable();
    PersonalityDefaultsAreTheShippedTable();
    JoinChanceUsesTheConfiguredTerms();
    TheVerdictRollHonoursTheExtremesWithoutAKey();
    TheVerdictRollRefusesWhenThereIsNoKey();
    TheVerdictRollIsTheDailyJoinRoll();
    DayIndexIsWholeGameDays();
    DailyRollNeverRerollsWithinADay();
    DailyRollHonoursTheExtremes();
    DailyRollPassesAboutHalfTheDaysAtFifty();
    DailyRollHandleKeyAlsoReseeds();
    DailyRollDifferentIdsAreIndependent();
    DailyRollConsecutiveDaysAreIndependent();
    DailyRollSaltSeparatesDraws();
    DailyRollIsMonotonicInThePercent();
}
