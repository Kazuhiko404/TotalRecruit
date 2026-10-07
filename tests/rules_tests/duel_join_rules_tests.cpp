// Tests for src/duel_join_rules.cpp: a won duel is a join chance in Balanced and a sure
// join in Cheat; the roll and the win cooldown are seeded by the NPC and the win count.

#include <cstdio>

#include "test_support.h"
#include "chance_rules.h"
#include "duel_join_rules.h"

using namespace totalrecruit;

namespace
{
const DuelChallenges kConfig = DuelChallenges();

DuelWinFacts Win(const char* id, int wins, int level)
{
    DuelWinFacts f;
    f.cheat = false;
    f.npc = CharKey::FromId(id);
    f.wins = wins;
    f.npcLevel = level;
    f.nowHours = 100.0;
    return f;
}

void TheChanceGrowsWithEveryWin()
{
    const char* name = "chance: 25, 45, 65, 85, 100 by default, capped at 100";
    CHECK(DuelJoinChance(1, kConfig) == 25);
    CHECK(DuelJoinChance(2, kConfig) == 45);
    CHECK(DuelJoinChance(3, kConfig) == 65);
    CHECK(DuelJoinChance(4, kConfig) == 85);
    CHECK(DuelJoinChance(5, kConfig) == 100);
    CHECK(DuelJoinChance(12, kConfig) == 100);
}

void FewerThanOneWinReadsAsTheFirst()
{
    const char* name = "chance: 0 or negative wins read as the first win";
    CHECK(DuelJoinChance(0, kConfig) == 25);
    CHECK(DuelJoinChance(-3, kConfig) == 25);
}

void TheChanceFollowsTheConfig()
{
    const char* name = "chance: base and per-win come from the config, never below 0";
    DuelChallenges c;
    c.joinBaseChance = 10;
    c.joinChancePerWin = 30;
    CHECK(DuelJoinChance(1, c) == 10);
    CHECK(DuelJoinChance(3, c) == 70);
    CHECK(DuelJoinChance(4, c) == 100);
    c.joinBaseChance = -5;
    c.joinChancePerWin = 0;
    CHECK(DuelJoinChance(2, c) == 0);
}

void TheNextWinOddsCountTheNextWin()
{
    const char* name = "odds: Balanced shows the chance of the next win, Cheat is sure";
    const DuelJoinOdds first = DuelNextWinOdds(false, 0, kConfig);
    CHECK(!first.sure && first.chance == 25);
    const DuelJoinOdds third = DuelNextWinOdds(false, 2, kConfig);
    CHECK(!third.sure && third.chance == 65);
    const DuelJoinOdds cheat = DuelNextWinOdds(true, 2, kConfig);
    CHECK(cheat.sure && cheat.chance == 100);
}

void TheRollIsAPercent()
{
    const char* name = "roll: seed % 100";
    CHECK(DuelJoinRollFromSeed(0u) == 0);
    CHECK(DuelJoinRollFromSeed(99u) == 99);
    CHECK(DuelJoinRollFromSeed(1234u) == 34);
    CHECK(DuelJoinRollFromSeed(0xFFFFFFFFu) == (int)(0xFFFFFFFFu % 100u));
}

void TheBandDependsOnTheLevel()
{
    const char* name = "band: 80-99 waits 24..72 h, 100+ waits 24..120 h";
    float lo = -1.0f;
    float hi = -1.0f;
    DuelWinCooldownBand(85, kConfig, &lo, &hi);
    CHECK(lo == 24.0f && hi == 72.0f);
    DuelWinCooldownBand(99, kConfig, &lo, &hi);
    CHECK(hi == 72.0f);
    DuelWinCooldownBand(100, kConfig, &lo, &hi);
    CHECK(lo == 24.0f && hi == 120.0f);
}

void ABandBelowItsMinReadsAsTheMin()
{
    const char* name = "band: a max below the min is the min; negatives read as 0";
    DuelChallenges c;
    c.winCooldownMinHours = 30.0f;
    c.winCooldownMaxHoursUnder100 = 10.0f;
    float lo = -1.0f;
    float hi = -1.0f;
    DuelWinCooldownBand(85, c, &lo, &hi);
    CHECK(lo == 30.0f && hi == 30.0f);
    CHECK(DuelWinCooldownHoursFromSeed(12345u, 85, c) == 30.0f);
    c.winCooldownMinHours = -4.0f;
    c.winCooldownMaxHoursUnder100 = -1.0f;
    DuelWinCooldownBand(85, c, &lo, &hi);
    CHECK(lo == 0.0f && hi == 0.0f);
}

void ThePickStaysInTheBandAndCoversIt()
{
    const char* name = "pick: every seed lands in [min, max]; the ends are reachable";
    bool low = false;
    bool high = false;
    bool inside = true;
    for (unsigned int seed = 0; seed < 3000u; ++seed)
    {
        const float h = DuelWinCooldownHoursFromSeed(seed, 85, kConfig);
        if (h < 24.0f || h > 72.0f) inside = false;
        if (h < 30.0f) low = true;
        if (h > 66.0f) high = true;
    }
    CHECK(inside);
    CHECK(low && high);
    CHECK(DuelWinCooldownHoursFromSeed(0u, 85, kConfig) == 24.0f);
    CHECK(DuelWinCooldownHoursFromSeed(1000u, 85, kConfig) == 72.0f);
    CHECK(DuelWinCooldownHoursFromSeed(1000u, 120, kConfig) == 120.0f);
}

void CheatAlwaysJoins()
{
    const char* name = "decide: Cheat joins on any win, no roll, no cooldown";
    DuelWinFacts f = Win("id:cheat", 1, 85);
    f.cheat = true;
    DuelChallenges c;
    c.joinBaseChance = 0;
    c.joinChancePerWin = 0;
    const DuelWinDecision d = DecideDuelWin(f, c);
    CHECK(d.joins);
    CHECK(d.chance == 100);
    CHECK(d.cooldownUntilMinute == 0);
}

void TheMinimumWinsDefaultToOne()
{
    const char* name = "min wins: both presets default to 1 (today's behaviour); fewer reads as 1";
    CHECK(kConfig.minWinsCheat == 1);
    CHECK(kConfig.minWinsBalanced == 1);
    CHECK(DuelMinWins(true, kConfig) == 1);
    CHECK(DuelMinWins(false, kConfig) == 1);
    DuelChallenges c;
    c.minWinsCheat = 3;
    c.minWinsBalanced = 0;
    CHECK(DuelMinWins(true, c) == 3);
    CHECK(DuelMinWins(false, c) == 1);
    // Default: the first win decides as before.
    CHECK(DecideDuelWin(Win("id:x", 1, 85), kConfig).winsStillNeeded == 0);
    CHECK(DuelNextWinOdds(false, 0, kConfig).winsNeeded == 0);
}

void AWinUnderTheMinimumNeverJoins()
{
    const char* name = "min wins: a win under the minimum stays with no roll and no cooldown, Cheat too";
    DuelChallenges c;
    c.minWinsCheat = 3;
    c.minWinsBalanced = 2;
    c.joinBaseChance = 100;

    DuelWinFacts cheat = Win("id:cheat", 1, 85);
    cheat.cheat = true;
    DuelWinDecision d = DecideDuelWin(cheat, c);
    CHECK(!d.joins);
    CHECK(d.chance == 0);
    CHECK(d.roll == 0);
    CHECK(d.winsStillNeeded == 2);
    CHECK(d.cooldownUntilMinute == 0);
    cheat.wins = 2;
    CHECK(DecideDuelWin(cheat, c).winsStillNeeded == 1);
    cheat.wins = 3;
    d = DecideDuelWin(cheat, c);
    CHECK(d.joins);
    CHECK(d.winsStillNeeded == 0);

    DuelWinFacts balanced = Win("id:bal", 1, 85);
    d = DecideDuelWin(balanced, c);
    CHECK(!d.joins);
    CHECK(d.winsStillNeeded == 1);
    CHECK(d.cooldownUntilMinute == 0);
    balanced.wins = 2;
    d = DecideDuelWin(balanced, c);
    CHECK(d.winsStillNeeded == 0);
    CHECK(d.chance == 100);            // the curve still counts from the first win
    CHECK(d.joins);

    DuelChallenges curve;
    curve.minWinsBalanced = 3;
    CHECK(DecideDuelWin(Win("id:c", 3, 85), curve).chance == 65);
}

void TheNextWinOddsSayHowManyWinsAreMissing()
{
    const char* name = "min wins: the card odds count the wins still missing, the next one included";
    DuelChallenges c;
    c.minWinsCheat = 3;
    c.minWinsBalanced = 4;
    DuelJoinOdds o = DuelNextWinOdds(true, 0, c);
    CHECK(!o.sure);
    CHECK(o.chance == 0);
    CHECK(o.winsNeeded == 3);
    o = DuelNextWinOdds(true, 1, c);
    CHECK(o.winsNeeded == 2);
    o = DuelNextWinOdds(true, 2, c);      // the next win is the third: sure
    CHECK(o.sure);
    CHECK(o.winsNeeded == 0);
    o = DuelNextWinOdds(false, 1, c);
    CHECK(o.winsNeeded == 3);
    CHECK(o.chance == 0);
    o = DuelNextWinOdds(false, 3, c);     // the fourth win rolls the curve's fourth step
    CHECK(o.winsNeeded == 0);
    CHECK(o.chance == 85);
}

void BalancedRollsTheSeededChance()
{
    const char* name = "decide: Balanced joins exactly when the seeded roll is under the chance";
    for (int i = 0; i < 40; ++i)
    {
        char id[32];
        _snprintf_s(id, sizeof(id), _TRUNCATE, "id:npc-%d", i);
        for (int wins = 1; wins <= 5; ++wins)
        {
            const DuelWinFacts f = Win(id, wins, 90);
            const DuelWinDecision d = DecideDuelWin(f, kConfig);
            const int roll = (int)(DailySeed(f.npc, wins, kDuelJoinRollSalt) % 100u);
            CHECK(d.chance == DuelJoinChance(wins, kConfig));
            CHECK(d.roll == roll);
            CHECK(d.joins == (roll < d.chance));
        }
    }
}

void TheSameWinNeverRerolls()
{
    const char* name = "decide: the same NPC and win count give the same answer and cooldown";
    const DuelWinFacts f = Win("id:reload", 2, 104);
    const DuelWinDecision a = DecideDuelWin(f, kConfig);
    const DuelWinDecision b = DecideDuelWin(f, kConfig);
    CHECK(a.joins == b.joins && a.roll == b.roll && a.cooldownHours == b.cooldownHours);
}

void AWinThatStaysStartsTheWinCooldown()
{
    const char* name = "decide: a win that does not join waits a seeded time in the level's band";
    DuelChallenges never;
    never.joinBaseChance = 0;
    never.joinChancePerWin = 0;
    const DuelWinFacts under = Win("id:stay", 1, 85);
    const DuelWinDecision d = DecideDuelWin(under, never);
    CHECK(!d.joins);
    CHECK(d.chance == 0);
    CHECK(d.cooldownHours >= 24.0f && d.cooldownHours <= 72.0f);
    const unsigned int seed = DailySeed(under.npc, 1, kDuelWinCooldownSalt);
    CHECK(d.cooldownHours == DuelWinCooldownHoursFromSeed(seed, 85, never));
    CHECK(d.cooldownUntilMinute >= (100 + 24) * 60 && d.cooldownUntilMinute <= (100 + 73) * 60);

    const DuelWinFacts wide = Win("id:stay", 1, 130);
    const DuelWinDecision w = DecideDuelWin(wide, never);
    CHECK(w.cooldownHours >= 24.0f && w.cooldownHours <= 120.0f);
}

void AJoinHasNoCooldown()
{
    const char* name = "decide: a win that joins leaves no cooldown";
    DuelChallenges always;
    always.joinBaseChance = 100;
    const DuelWinDecision d = DecideDuelWin(Win("id:join", 1, 85), always);
    CHECK(d.joins);
    CHECK(d.cooldownUntilMinute == 0);
    CHECK(d.cooldownHours == 0.0f);
}

void ATestHourCooldownWorks()
{
    const char* name = "decide: a 1 h test band ends one hour later";
    DuelChallenges c;
    c.joinBaseChance = 0;
    c.joinChancePerWin = 0;
    c.winCooldownMinHours = 1.0f;
    c.winCooldownMaxHoursUnder100 = 1.0f;
    c.winCooldownMaxHours = 1.0f;
    const DuelWinDecision d = DecideDuelWin(Win("id:test", 3, 85), c);
    CHECK(d.cooldownHours == 1.0f);
    CHECK(d.cooldownUntilMinute == 101 * 60);
}
} // namespace

void RunDuelJoinRulesTests()
{
    TheChanceGrowsWithEveryWin();
    FewerThanOneWinReadsAsTheFirst();
    TheChanceFollowsTheConfig();
    TheNextWinOddsCountTheNextWin();
    TheRollIsAPercent();
    TheBandDependsOnTheLevel();
    ABandBelowItsMinReadsAsTheMin();
    ThePickStaysInTheBandAndCoversIt();
    CheatAlwaysJoins();
    TheMinimumWinsDefaultToOne();
    AWinUnderTheMinimumNeverJoins();
    TheNextWinOddsSayHowManyWinsAreMissing();
    BalancedRollsTheSeededChance();
    TheSameWinNeverRerolls();
    AWinThatStaysStartsTheWinCooldown();
    AJoinHasNoCooldown();
    ATestHourCooldownWorks();
}
