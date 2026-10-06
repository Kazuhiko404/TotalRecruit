#pragma once

// What a won duel means, without the engine.
//
// User design (2026-10-04): in Balanced, beating an 80+ NPC in a TotalSparring duel
// does not recruit for sure any more; it is a chance that grows with every head-to-head
// win. In Cheat a win still recruits, and Cheat offers the duel too (duel_challenge_rules.h
// CanOfferDuel, `cheatPreset`).
//
//   chance   = min(100, duelJoinBaseChance + duelJoinChancePerWin * (wins - 1))
//              wins = TotalSparring's head-to-head wins of the winner against this NPC,
//              the just-won bout included (25, 45, 65, 85, 100 by default)
//   roll     = DailySeed(npc, wins, kDuelJoinRollSalt) % 100; joins when roll < chance.
//              Seeded by the NPC and the win count: a reload (same wins) never rerolls,
//              and the next win is a fresh draw.
//   stays    = a win cooldown, uniform in [duelWinCooldownMinHours, max] with max =
//              duelWinCooldownMaxHoursUnder100 below level 100 and duelWinCooldownMaxHours
//              from 100, drawn from DailySeed(npc, wins, kDuelWinCooldownSalt). The end
//              hour goes into the duel cooldown book (duel_challenge_rules.h), the same
//              saved row a lost duel uses.
//
//   minimum  = duelMinWinsCheat / duelMinWinsBalanced (duel-any-source U4, 1 by default):
//              a win with fewer head-to-head wins never joins, rolls nothing and starts
//              no win cooldown (Cheat too); it says how many wins are still missing.
//              From the minimum on, Cheat joins and Balanced rolls the curve above,
//              still counted from the first win (minimum 3: the third win rolls 65%).
//
// tests/rules_tests/duel_join_rules_tests.cpp.

#include "char_key.h"
#include "config.h"

namespace totalrecruit
{
// Salts that keep the join roll and the cooldown pick apart from each other and from
// every other DailySeed draw (chance_rules.h).
const unsigned int kDuelJoinRollSalt = 0x444A4F49u;      // "DJOI"
const unsigned int kDuelWinCooldownSalt = 0x4457434Fu;   // "DWCO"
// From this level up a win that does not join waits the longer band.
const int kDuelWinCooldownWideFromLevel = 100;

// The join chance of the `wins`-th win (the new one counted), 0..100. Fewer than one
// win reads as the first.
int DuelJoinChance(int wins, const DuelChallenges& config);

// The head-to-head wins this preset needs before a win can recruit; at least 1.
int DuelMinWins(bool cheat, const DuelChallenges& config);

// What the next win would do, for the popup row and the card line: Cheat is sure,
// Balanced is DuelJoinChance(winsSoFar + 1). When the next win is still under the
// minimum, neither: chance 0 and `winsNeeded` the wins still missing, the next one
// included (minimum 3 with none so far: 3).
struct DuelJoinOdds
{
    bool sure;
    int chance;
    int winsNeeded;

    DuelJoinOdds() : sure(false), chance(0), winsNeeded(0) {}
};
DuelJoinOdds DuelNextWinOdds(bool cheat, int winsSoFar, const DuelChallenges& config);

// The roll 0..99 drawn from a seed.
int DuelJoinRollFromSeed(unsigned int seed);

// The win cooldown band for this level: [min, max], max never below min, both >= 0.
void DuelWinCooldownBand(int npcLevel, const DuelChallenges& config, float* minHours,
                         float* maxHours);

// A uniform pick in the band from a seed (1/1000 of the band resolution).
float DuelWinCooldownHoursFromSeed(unsigned int seed, int npcLevel, const DuelChallenges& config);

struct DuelWinFacts
{
    // Preset_Cheat: a win always joins.
    bool cheat;
    CharKey npc;
    // Head-to-head wins against this NPC, the just-won bout included.
    int wins;
    int npcLevel;
    double nowHours;

    DuelWinFacts() : cheat(false), npc(), wins(1), npcLevel(0), nowHours(0.0) {}
};

struct DuelWinDecision
{
    bool joins;
    // The chance that was rolled against (100 in Cheat) and the roll (0 in Cheat).
    int chance;
    int roll;
    // Stays: the drawn cooldown and the game hour it ends (DuelCooldownUntilHour).
    float cooldownHours;
    int cooldownUntilHour;
    // Under the minimum wins: how many more wins it still needs (0 otherwise). Such a
    // win has no roll and no cooldown.
    int winsStillNeeded;

    DuelWinDecision()
        : joins(false), chance(0), roll(0), cooldownHours(0.0f), cooldownUntilHour(0), winsStillNeeded(0)
    {
    }
};

DuelWinDecision DecideDuelWin(const DuelWinFacts& facts, const DuelChallenges& config);
} // namespace totalrecruit
