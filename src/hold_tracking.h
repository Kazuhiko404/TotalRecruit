#pragma once

class Character;

namespace totalrecruit
{
// How long each captive has been held in the player's cage, on the game clock.
//
// The engine does not record when captivity began, so the mod watches: the
// periodic sweep (hail.cpp, through NoteCageState) starts a prisoner's clock the
// first time it sees them in the player's cage and drops it when it sees them
// free. A prisoner taken before the plugin ever saw them is counted from the first
// sighting, which errs towards making the player wait -- the safe direction. It
// lives only in memory: a reload restarts the clock rather than freeing someone the
// player never actually held. The timing rules themselves are engine-free
// (hold_clock.cpp); this is the engine-facing half.

// Looks at where this character is right now and keeps the hold clock honest:
// caged starts it (once), free drops it, so the easy-mode wait counts time in THIS
// cage. Returns whether they are in the player's cage. Called by the periodic
// sweep for everyone in range.
bool NoteCageState(Character* character);

// Hours held, in in-game hours. Only called for a prisoner already known to be in
// the player's cage, so the sighting it makes is a caged one: that is the fallback
// for a prisoner the periodic sweep has not reached yet (out of range, or caged
// this very moment), and it never restarts a clock the sweep already started.
float HoursHeld(Character* character);

// Forgets every hold clock (new session, plugin start and stop).
void ResetHoldClock();
} // namespace totalrecruit
