#pragma once

// PURE: resignation in the Balanced cage (balanced-tuning T4).
//
// Since the easy wait was removed (v1-presets-close P4) time in the cage counted
// for nothing on its own: a prisoner kept in the same state rolled the same chance
// on day 1 and on day 30. Resignation gives time back a voice, slowly and capped:
// every FULL game day held adds resignationPerDay, up to resignationMax, and the
// sum is divided by the resolve multiplier (chance_rules.h ResolveMultiplier) like
// stress and defeats -- the proud resign slower. It is one more term of the roll
// (ChanceInputs::resignation), never a verdict.
//
// The days come from the hold clock (hold_tracking.h), whose start is saved with
// the character (total_recruit.hold_start_hour), so a reload keeps them. No clock
// (a prisoner the sweep has not seen yet) reads as 0 hours.
//
// No Kenshi include: tested in tests/rules_tests/resignation_rules_tests.cpp.

#include "config.h"
#include "temperament.h"

namespace totalrecruit
{
// The config ranges (config.cpp LoadConfig clamps to them; the rules clamp too).
extern const int kResignationPerDayMin;   // 0 (off)
extern const int kResignationPerDayMax;   // 5
extern const int kResignationMaxMin;      // 0
extern const int kResignationMaxMax;      // 30

// Whole game days in `hoursHeld` (24 h each), rounded down; 0 for 0 or less.
int FullDaysHeld(float hoursHeld);

// The resignation term: min(FullDaysHeld x resignationPerDay, resignationMax) /
// resolve, truncated, and never above resignationMax (a low resolve cannot push
// it past the cap). Per day and cap are clamped to their ranges.
int ResignationChance(float hoursHeld, Temperament personality, const Submission& sub);
} // namespace totalrecruit
