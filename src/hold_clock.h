#pragma once

// How long a prisoner has been held, without the engine.
//
// The engine does not record when captivity began, so the mod watches: every
// sighting says whether the character is in the player's cage right now, and
// the clock starts the first time it is seen caged and is thrown away the
// moment it is seen free. That is what makes the wait mean "time in THIS
// cage" -- it used to start at the first Evaluate, which is whenever the player
// happened to click, and was never dropped, so a prisoner released and caged
// again joined instantly on the strength of the previous stay.
//
// It lives only in memory, as before: a reload restarts the clock rather than
// freeing someone the player never actually held.

#include "char_key.h"

#include <map>

namespace totalrecruit
{
class HoldClock
{
public:
    // One sighting at game hour `nowHours`. Caged starts the clock if it is not
    // already running; not caged forgets it.
    void Observe(const CharKey& who, bool caged, double nowHours);

    // Game hours since the clock started; 0 for anyone not being held. Asking
    // never starts the clock -- only a sighting does.
    float HoursHeld(const CharKey& who, double nowHours) const;

    bool IsTracked(const CharKey& who) const;
    int TrackedCount() const;
    void Clear();

private:
    std::map<CharKey, double> since_;
};
} // namespace totalrecruit
