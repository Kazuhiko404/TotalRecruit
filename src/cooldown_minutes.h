#pragma once

// PURE: the reject cooldown and the duel cooldown, kept to the game minute
// (priority-one B).
//
// Both used to be stored as the whole game hour they end on, so Cheat's
// one-minute wait (cheat_waits.h) ran until the next whole hour: up to about an
// hour instead of a minute. Now the stored value is the game MINUTE the cooldown
// ends on (game hours * 60, an int: a century of game time still fits).
//
// Rounding keeps Balanced as it was: a wait of an hour or more still ends on the
// whole game hour it rounds up to (the old ceil), and only a wait under an hour
// (Cheat's minute, or a short configured value) ends on the minute it rounds up
// to. 0 hours or less is no cooldown (0).
//
// Saves: the old rows ("total_recruit.reject_cooldown_until",
// "total_recruit.duel_cooldown_until") held whole hours and are still written, as
// the hour the minute rounds up to, so an older build reads a sane value. The
// exact minute rides a new row ("..._until_min", saved_ints.cpp) restored after
// the old one; a save from before this build has only the old row, which reads
// back as that hour's first minute (UntilMinuteFromSavedHour).
//
// No Kenshi / MyGUI include: tested in tests/rules_tests/cooldown_minutes_tests.cpp.

namespace totalrecruit
{
// The game minute a cooldown of `cooldownHours` started at `nowHours` ends on; 0
// when there is none. At least 1 when there is one.
int CooldownUntilMinute(double nowHours, float cooldownHours);

// Whether a stored end minute still runs at `nowHours`; the game hours left go to
// `hoursLeft` (0 when it does not run; may be NULL). 0 or less never runs.
bool CooldownActiveAt(int untilMinute, double nowHours, double* hoursLeft);

// A legacy save value (a whole end hour) as an end minute; 0 or less is none, and
// a huge value is clamped instead of wrapping.
int UntilMinuteFromSavedHour(int untilHour);

// The legacy row's value for an end minute: the hour it rounds up to; 0 for none.
int SavedHourFromUntilMinute(int untilMinute);
} // namespace totalrecruit
