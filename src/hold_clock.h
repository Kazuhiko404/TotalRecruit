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
// The start rides in the character save (v1-presets-close P3): saved_ints writes
// SavedStartTenths next to the persisted id and the load hook puts it back with
// RestoreStartTenths after load_boundary cleared the clock. The next sighting then
// decides as always: still caged keeps the hours, free drops them, and a start
// later than the game clock (an older save) restarts the hold.

#include "char_key.h"

#include <map>

namespace totalrecruit
{
// The save form of a hold start: tenths of a game hour, rounded, at least 1 (0
// means "not held" in the save) and at most INT_MAX.
int HoldStartToSavedTenths(double hours);
double HoldStartFromSavedTenths(int tenths);   // 0.0 for 0 or less

class HoldClock
{
public:
    // One sighting at game hour `nowHours`. Caged starts the clock if it is not
    // already running; not caged forgets it.
    void Observe(const CharKey& who, bool caged, double nowHours);

    // Game hours since the clock started; 0 for anyone not being held. Asking
    // never starts the clock -- only a sighting does.
    float HoursHeld(const CharKey& who, double nowHours) const;

    // The save row: the start in tenths of a game hour (HoldStartToSavedTenths),
    // 0 for anyone not being held.
    int SavedStartTenths(const CharKey& who) const;

    // A loading save state puts the start back. 0 or less restores nothing.
    void RestoreStartTenths(const CharKey& who, int tenths);

    bool IsTracked(const CharKey& who) const;
    int TrackedCount() const;
    void Clear();

private:
    std::map<CharKey, double> since_;
};
} // namespace totalrecruit
