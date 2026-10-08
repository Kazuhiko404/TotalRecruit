#pragma once

// PURE: the Balanced cage's care streak (balanced-tuning T3).
//
// The cared-for bonus (cared_for_rules.h) is a draw of the day, so one day of care
// was worth as much as a week of it, and breaking a prisoner (full stress) was
// worth far more than looking after them. The streak makes care a sustained
// argument: every game day in a row on which the prisoner was seen unhurt, fed and
// at full health (IsFullyCaredFor, body_rules.h) adds caredForStreakPerDay to the
// bonus, up to caredForStreakMax.
//
// What counts as a day (CareStreakDays):
//   - a day is kept when every sighting of the prisoner that day found them cared
//     for; one sighting that did not resets the streak to 0 on the spot;
//   - the streak counts kept days in a row that ENDED before today: today is still
//     running, so it counts from tomorrow on (the first cared-for day adds nothing);
//   - a game day with no sighting at all breaks the chain: nobody can say it was
//     kept, so it errs towards making the player wait (the hold clock's rule).
// The game day is chance_rules.h DayIndexFromHours, the day DailySeed rolls on, so
// nothing here is re-rollable with clicks: the streak only moves with the game clock.
//
// Kept per prisoner by CareStreakLedger and saved with the character (three
// saved_ints.h rows, care_streak.h); an old save without them reads as no streak.
//
// No Kenshi include: tested in tests/rules_tests/care_streak_rules_tests.cpp.

#include "char_key.h"
#include "config.h"

#include <map>

namespace totalrecruit
{
// The config ranges (config.cpp LoadConfig clamps to them; the rules clamp too).
extern const int kCareStreakPerDayMin;   // 0 (no streak)
extern const int kCareStreakPerDayMax;   // 10
extern const int kCareStreakMaxMin;      // 0
extern const int kCareStreakMaxMax;      // 40

// One prisoner's care record. dayIndex -1: never seen.
struct CareStreak
{
    int dayIndex;   // the game day of the latest sighting
    int streak;     // kept days in a row that ended before dayIndex
    bool dayKept;   // every sighting on dayIndex found them cared for

    CareStreak() : dayIndex(-1), streak(0), dayKept(false) {}
};

// `stored` after one more sighting on `today`, cared for or not. A day before the
// stored one (a clock that went back) starts the record afresh.
CareStreak ObserveCare(const CareStreak& stored, int today, bool caredFor);

// The kept days in a row before `today` (0 when the chain is broken, or today was
// already seen not cared for).
int CareStreakDays(const CareStreak& stored, int today);

// What `streakDays` add to the cared-for bonus: caredForStreakPerDay per day
// (clamped to [kCareStreakPerDayMin, Max]), capped at caredForStreakMax (clamped
// to [kCareStreakMaxMin, Max]). 0 for 0 days or fewer.
int CareStreakBonus(int streakDays, const Submission& sub);

// The care record per prisoner. Cleared at load begin and refilled from each
// character's save state.
class CareStreakLedger
{
public:
    void Observe(const CharKey& key, int today, bool caredFor);
    // CareStreakDays of the prisoner's record (0 for someone never seen).
    int StreakDays(const CharKey& key, int today) const;

    // The save rows: the day index + 1 (0 = no record), the streak, 1 when the day
    // was kept; all 0 for someone not in the ledger.
    int SavedDay(const CharKey& key) const;
    int SavedStreak(const CharKey& key) const;
    int SavedKept(const CharKey& key) const;

    // A loading save state puts the rows back, in any order; 0 or less restores
    // nothing. A streak without its day row reads as no streak.
    void RestoreDay(const CharKey& key, int saved);
    void RestoreStreak(const CharKey& key, int saved);
    void RestoreKept(const CharKey& key, int saved);

    void Clear();
    int Size() const;

private:
    typedef std::map<CharKey, CareStreak> Entries;
    Entries entries_;
};
} // namespace totalrecruit
