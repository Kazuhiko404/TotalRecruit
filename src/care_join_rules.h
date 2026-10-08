#pragma once

// PURE: when care alone makes a caged prisoner join (balanced-tuning T6).
//
// The honourable and the warm/kind take no cats, so when the Balanced cage's daily
// roll fails the currency table (route_cage.cpp ChooseCurrency) sends them to care.
// Care used to make them join on the spot: healthy and fed was a yes, whatever the
// roll said, which skipped the days the roll is meant to take. Now care is a yes
// only once the prisoner has been kept that way for a while: the care streak
// (care_streak_rules.h CareStreakDays, kept days in a row before today) has to reach
// careJoinStreakDays. Below it they stay, and the reason says how many more
// cared-for days are missing (Reason_CaredForNotYet).
//
// careJoinStreakDays 0 is the old behaviour: care joins as soon as they are cared for.
// Cheat never reaches this (route_cheat.cpp answers first).
//
// No Kenshi include: tested in tests/rules_tests/care_join_rules_tests.cpp.

#include "config.h"

namespace totalrecruit
{
// The config range (config.cpp LoadConfig clamps to it; the rule clamps too).
extern const int kCareJoinStreakDaysMin;   // 0 (care joins at once)
extern const int kCareJoinStreakDaysMax;   // 30

// The streak care needs before it joins: careJoinStreakDays clamped to
// [kCareJoinStreakDaysMin, kCareJoinStreakDaysMax].
int CareJoinStreakDaysNeeded(const Submission& sub);

// The cared-for days still missing with `streakDays` of streak (negative reads as 0):
// 0 when care joins now, otherwise CareJoinStreakDaysNeeded - streakDays.
int CareJoinDaysMissing(int streakDays, const Submission& sub);
} // namespace totalrecruit
