#include "duel_join_rules.h"

#include "chance_rules.h"
#include "duel_challenge_rules.h"

namespace totalrecruit
{
namespace
{
// The cooldown pick's resolution: 1/1000 of the band, both ends included.
const unsigned int kCooldownSteps = 1000u;

float NotNegative(float value)
{
    return value > 0.0f ? value : 0.0f;
}
} // namespace

int DuelJoinChance(int wins, const DuelChallenges& config)
{
    const int counted = wins < 1 ? 1 : wins;
    // Wide enough for any clamped config; a hand-built one cannot overflow either.
    const long long chance =
        (long long)config.joinBaseChance + (long long)config.joinChancePerWin * (long long)(counted - 1);
    if (chance <= 0)
    {
        return 0;
    }
    return chance >= 100 ? 100 : (int)chance;
}

DuelJoinOdds DuelNextWinOdds(bool cheat, int winsSoFar, const DuelChallenges& config)
{
    DuelJoinOdds odds;
    if (cheat)
    {
        odds.sure = true;
        odds.chance = 100;
        return odds;
    }
    const int sofar = winsSoFar > 0 ? winsSoFar : 0;
    odds.chance = DuelJoinChance(sofar + 1, config);
    return odds;
}

int DuelJoinRollFromSeed(unsigned int seed)
{
    return (int)(seed % 100u);
}

void DuelWinCooldownBand(int npcLevel, const DuelChallenges& config, float* minHours,
                         float* maxHours)
{
    const float lo = NotNegative(config.winCooldownMinHours);
    const float top = npcLevel >= kDuelWinCooldownWideFromLevel ? config.winCooldownMaxHours
                                                                : config.winCooldownMaxHoursUnder100;
    const float hi = top < lo ? lo : top;
    if (minHours != 0)
    {
        *minHours = lo;
    }
    if (maxHours != 0)
    {
        *maxHours = hi;
    }
}

float DuelWinCooldownHoursFromSeed(unsigned int seed, int npcLevel, const DuelChallenges& config)
{
    float lo = 0.0f;
    float hi = 0.0f;
    DuelWinCooldownBand(npcLevel, config, &lo, &hi);
    const unsigned int step = seed % (kCooldownSteps + 1u);
    if (step == kCooldownSteps)
    {
        return hi;
    }
    return lo + (hi - lo) * ((float)step / (float)kCooldownSteps);
}

DuelWinDecision DecideDuelWin(const DuelWinFacts& facts, const DuelChallenges& config)
{
    DuelWinDecision d;
    if (facts.cheat)
    {
        d.joins = true;
        d.chance = 100;
        return d;
    }
    const int wins = facts.wins < 1 ? 1 : facts.wins;
    d.chance = DuelJoinChance(wins, config);
    d.roll = DuelJoinRollFromSeed(DailySeed(facts.npc, wins, kDuelJoinRollSalt));
    d.joins = d.roll < d.chance;
    if (d.joins)
    {
        return d;
    }
    d.cooldownHours = DuelWinCooldownHoursFromSeed(DailySeed(facts.npc, wins, kDuelWinCooldownSalt),
                                                   facts.npcLevel, config);
    d.cooldownUntilHour = DuelCooldownUntilHour(facts.nowHours, d.cooldownHours);
    return d;
}
} // namespace totalrecruit
