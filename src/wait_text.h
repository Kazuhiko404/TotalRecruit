#pragma once

// The player-facing text of a wait, read from messages.json (priority-one B).
// wait_format.h decides how a wait reads (minutes under an hour, whole hours from
// an hour on); this picks the matching line: the usual key ({hours}) or its
// "<key>Minutes" twin ({minutes}, filled here). The caller still runs Fill for
// {name} and the rest, passing `*hoursOut` as the hours (0 for a minute line,
// which has no {hours}).

#include "wait_format.h"

#include <string>

class Character;

namespace totalrecruit
{
// The line for a wait of `hoursLeft`: Say(hoursKey) with the hours to fill, or
// Say(hoursKey + "Minutes") with {minutes} already filled. A list picks the same
// entry for the same `who`, as Say does.
std::string SayWait(const std::string& hoursKey, const std::string& hoursFallback,
                    const std::string& minutesFallback, Character* who, float hoursLeft,
                    int* hoursOut);

// The wait suffix templates of the cage buttons (cage.button.restHours /
// cage.button.restMinutes), defaulting to WaitTexts' built-in ones.
WaitTexts RestWaitTexts();
} // namespace totalrecruit
