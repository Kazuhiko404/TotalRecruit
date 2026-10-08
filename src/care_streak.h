#pragma once

class Character;

#include <string>

#include "char_key.h"

namespace totalrecruit
{
// The engine-facing half of the Balanced cage's care streak (balanced-tuning T3).
// The rule (what a kept day is, how the streak grows and resets, what it adds to
// the cared-for bonus) is care_streak_rules.h; this keeps one CareStreakLedger
// behind a lock (the serialise hook reads it off the game thread) and feeds it
// sightings:
//   - the periodic sweep (hail.cpp) for every prisoner in the player's cage, so the
//     streak follows the game clock while the player is around;
//   - a decision on a caged prisoner (GatherCageExtra), so a click never reads a
//     streak that missed the sighting it is looking at.
// A peek (the readout card) only reads. Balanced only: Cheat never asks.

// The sweep's look at someone it just saw in the player's cage: today's sighting,
// cared for (IsFullyCaredFor) or not. Outside Balanced it does nothing. The
// prisoner already has an id (the hold clock minted it).
void NoteCareState(Character* character);

// The streak of `key` on `dayIndex` (CareStreakDays). With `observe` (a decision)
// the sighting is recorded first, `caredFor` or not; without it, a plain read.
int CareStreakDaysFor(const CharKey& key, int dayIndex, bool observe, bool caredFor);

// The save rows (saved_ints.h): total_recruit.care_streak_day (game day + 1, 0 = no
// record), care_streak_days, care_streak_kept (1/0). Thread-safe.
int SavedCareStreakDay(const std::string& persistedId);
void RestoreSavedCareStreakDay(const std::string& persistedId, int value);
int SavedCareStreakDays(const std::string& persistedId);
void RestoreSavedCareStreakDays(const std::string& persistedId, int value);
int SavedCareStreakKept(const std::string& persistedId);
void RestoreSavedCareStreakKept(const std::string& persistedId, int value);

// Forgets every record (load begin, plugin start and stop).
void ForgetAllCareStreaks();
} // namespace totalrecruit
