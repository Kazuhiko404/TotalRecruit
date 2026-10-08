#pragma once

// PURE: the Balanced cage's daily limits on the beating stress (balanced-tuning
// T2). Without them Golpear + Noquear filled a normal prisoner's bar in about six
// game hours; with them one game day of beating moves the bar by at most the cap.
//
// Order, per blow or knockout that landed (DailyBeatingGain):
//   1. diminish: the n-th gain of the prisoner's game day is weighed by
//      beatingDiminishFactor^(n-1) -- applied to the raw gain, BEFORE the per-blow
//      clamp (BeatingStressGainScaled), so every blow still adds
//      beatingStressMin..beatingStressMax: the floor keeps a late blow worth
//      something and the max still bounds a fresh knockout;
//   2. cap: what is left of beatingDailyStressCap for that game day
//      (CapBeatingGain), AFTER the per-blow clamp -- the cap is the only thing that
//      takes a blow below the per-blow floor, down to 0.
// Every landed blow counts towards n, including one the cap zeroed.
//
// The game day is chance_rules.h DayIndexFromHours, the day DailySeed rolls on.
// The record of a day is kept per prisoner by BeatingDayLedger and saved with the
// character (three saved_ints.h rows, beating_stress.h); a record of another day,
// or none at all (an old save), reads as a fresh day.
//
// Balanced only: Cheat keeps the plain per-blow gain (beating_stress.cpp).
//
// No Kenshi include: tested in tests/rules_tests/beating_daily_rules_tests.cpp.

#include "beating_stress_rules.h"
#include "char_key.h"
#include "config.h"
#include "temperament.h"

#include <map>

namespace totalrecruit
{
// The config ranges (config.cpp LoadConfig clamps to them; the rules clamp too).
extern const float kBeatingDiminishFactorMin;   // 0.3
extern const float kBeatingDiminishFactorMax;   // 1.0 (no diminishing)
extern const float kBeatingDailyStressCapMin;   // 0.1
extern const float kBeatingDailyStressCapMax;   // 1.0

// One prisoner's beatings on one game day. dayIndex -1: no record.
struct BeatingDay
{
    int dayIndex;
    int blows;      // landed blows and knockouts so far that day
    float gained;   // bar gained so far that day, 0..1

    BeatingDay() : dayIndex(-1), blows(0), gained(0.0f) {}
};

// `stored` when it is a record of `today`; otherwise a fresh day `today`.
BeatingDay BeatingDayFor(const BeatingDay& stored, int today);

// factor^blowsSoFar, factor clamped to [kBeatingDiminishFactorMin, Max]. A count
// of 0 or less is the first blow (1).
float BeatingDiminishMultiplier(int blowsSoFar, float factor);

// `gain` (0 or less adds nothing) cut to what is left of `dailyCap` (clamped to
// [kBeatingDailyStressCapMin, Max]) after `gainedToday`; never below 0.
float CapBeatingGain(float gain, float gainedToday, float dailyCap);

// What this blow or knockout adds to the bar given `today` so far (see the order
// above). `today` must already be BeatingDayFor the current day.
float DailyBeatingGain(BeatingBlow blow, int playerLevel, int npcLevel,
                       Temperament personality, const BeatingDay& today, const Submission& sub);

// `today` after one more landed blow that added `added` (0 or less adds nothing
// to `gained`, the blow still counts).
BeatingDay RecordBeating(const BeatingDay& today, float added);

// The day record per prisoner, keyed like the beating stress bar. Cleared at load
// begin and refilled from each character's save state.
class BeatingDayLedger
{
public:
    // The prisoner's record for `today` (a fresh day when there is none).
    BeatingDay Today(const CharKey& key, int today) const;
    void Record(const CharKey& key, const BeatingDay& day);

    // The save rows: the day index + 1 (0 = no record), the blows, the gained bar
    // in thousandths; all 0 for someone not in the ledger.
    int SavedDay(const CharKey& key) const;
    int SavedBlows(const CharKey& key) const;
    int SavedGained(const CharKey& key) const;

    // A loading save state puts the rows back, in any order; 0 or less restores
    // nothing. Counts without their day row read as a fresh day.
    void RestoreDay(const CharKey& key, int saved);
    void RestoreBlows(const CharKey& key, int saved);
    void RestoreGained(const CharKey& key, int saved);

    void Clear();
    int Size() const;

private:
    typedef std::map<CharKey, BeatingDay> Entries;
    Entries entries_;
};
} // namespace totalrecruit
