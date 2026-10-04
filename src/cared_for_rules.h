#pragma once

// The Balanced cage's cared-for bonus, without the engine (v1-presets-close P4).
//
// A person in the player's cage who is unhurt, fed and at full health
// (IsFullyCaredFor, body_rules.h) used to join on the spot ("sano gracias a ti").
// That made the cage a nursing chore with a guaranteed payout, so it is a bonus on
// the ordinary roll now: being looked after argues for joining, it does not decide.
//
// The bonus is not a flat number, so the cared-for road is not too easy: it is a
// value in [submissionCaredForBonusMin, submissionCaredForBonusMax], drawn once per
// NPC per game day. Like every other roll in the mod it is seeded from the
// character and the in-game day (DailySeed under kCaredForBonusSalt), so clicking
// again or reloading never draws it afresh.

#include "char_key.h"
#include "config.h"

namespace totalrecruit
{
// Keeps the draw apart from the join and haggle rolls of the same NPC and day.
const unsigned int kCaredForBonusSalt = 0x43415245u;   // "CARE"

// The bonus a seed stands for: uniform in [minPct, maxPct]. A max below the min
// means the min (the duel_join_rules convention); negative ends read as 0.
int CaredForBonusFromSeed(unsigned int seed, int minPct, int maxPct);

// The day's bonus for one prisoner: 0 when not fully cared for, or with no key to
// seed from (no stable answer exists then); otherwise
// CaredForBonusFromSeed(DailySeed(key, dayIndex, kCaredForBonusSalt), min, max).
int CaredForBonus(bool caredFor, bool haveKey, const CharKey& key, int dayIndex,
                  const Submission& sub);
} // namespace totalrecruit
