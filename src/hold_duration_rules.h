#pragma once

// How the readout card writes how long a prisoner has been held, without the engine.
//
// The hours come from the hold clock (hold_tracking.h PeekHoursHeld), on the game
// clock. Time held is rounded DOWN to the whole minute: "2 h 15 min" means at least
// that long, never a promise the clock has not reached. A zero part is left out
// ("2 h", "1 d"), and the smallest unit shrinks as the hold grows:
//   under an hour  "{m} min"
//   under a day    "{h} h {m} min"
//   a day or more  "{d} d {h} h"

#include <string>

namespace totalrecruit
{
// 0.5 -> "30 min"; 2.25 -> "2 h 15 min"; 2.0 -> "2 h"; 27.0 -> "1 d 3 h"; 24.5 -> "1 d".
// Nothing (or a negative number) is "0 min".
std::string FormatHoldDuration(float hours);
} // namespace totalrecruit
