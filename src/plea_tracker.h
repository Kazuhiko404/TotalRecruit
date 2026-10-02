#pragma once

// Whose plea for mercy is on screen, without the engine.
//
// One at a time: a brawl can cripple several people in the same second, and a
// stack of windows would be worse than none. So "someone is pleading" blocks
// every later plea -- which is exactly why it has to be cleared on every way the
// window can end. It was kept as a raw Character*, which was never cleared on
// accept and could match a stranger at a freed address.

#include "char_key.h"

namespace totalrecruit
{
class PleaTracker
{
public:
    PleaTracker() : active_(false) {}

    // Starts a plea. False (and nothing changes) while another is pending.
    bool Begin(const CharKey& who);

    // True only for the exact character: a reused slot has another serial.
    bool IsPending(const CharKey& who) const;

    // Whether any plea is pending, whoever it belongs to.
    bool HasPending() const;

    // Ends the plea if it is this character's. Returns whether it ended one.
    bool ClearIf(const CharKey& who);

    // Ends whatever plea is pending: the window closed by some path that has
    // no character to name.
    void Clear();

private:
    bool active_;
    CharKey who_;
};
} // namespace totalrecruit
