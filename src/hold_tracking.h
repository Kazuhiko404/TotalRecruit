#pragma once

class Character;

#include <string>

namespace totalrecruit
{
// How long each captive has been held in the player's cage, on the game clock.
//
// The engine does not record when captivity began, so the mod watches: the
// periodic sweep (hail.cpp, through NoteCageState) starts a prisoner's clock the
// first time it sees them in the player's cage and drops it when it sees them
// free. A prisoner taken before the plugin ever saw them is counted from the first
// sighting, which errs towards making the player wait -- the safe direction. The start
// rides in the character save (saved_ints row total_recruit.hold_start_hour), so a
// reload keeps a caged prisoner's hours (v1-presets-close P3). The timing rules themselves are engine-free
// (hold_clock.cpp); this is the engine-facing half.

// Looks at where this character is right now and keeps the hold clock honest:
// caged starts it (once), free drops it, so Cheat's wait counts time in THIS
// cage. In Cheat with cheatAnyCage on (HoldClockCountsAnyCage) any cage, shackles
// or kidnap counts as caged for the clock (cheat-any-cage). Returns whether they
// are in the player's cage -- never the wider any-cage answer. Called by the
// periodic sweep for everyone in range.
bool NoteCageState(Character* character);

// Hours held, in in-game hours. Only called for a prisoner already known to be in
// a cage the clock counts (the player's, or in Cheat with cheatAnyCage any cage,
// CheatCageSatisfied), so the sighting it makes is a caged one: that is the fallback
// for a prisoner the periodic sweep has not reached yet (out of range, or caged
// this very moment), and it never restarts a clock the sweep already started.
float HoursHeld(Character* character);

// What a bystander look at the hold clock may do: read it. Never starts a clock, never
// drops one, never mints an id. False when no clock is running for this character
// (the periodic sweep has not seen them in the cage yet), so the caller says nothing
// rather than "0 hours".
bool PeekHoursHeld(Character* character, float* out);

// The readout card's look (prisoner-hold-time): PeekHoursHeld, but only while they
// sit in a cage the clock counts right now (route_decisions.h HoldClockCounts: the
// player's cage, or in Cheat with cheatAnyCage any cage, shackles or kidnap). A clock
// the sweep has not dropped yet for someone already out reads as false. Never starts
// a clock, never mints an id.
bool PeekCountedHoursHeld(Character* character, float* out);

// The save row (saved_ints.h): the hold start of the character carrying
// `persistedId`, in tenths of a game hour, 0 when not held; and putting it back
// from a loading save state. Thread-safe (the serialise hook).
int SavedHoldStart(const std::string& persistedId);
void RestoreSavedHoldStart(const std::string& persistedId, int tenths);

// Forgets every hold clock (new session, plugin start and stop).
void ResetHoldClock();
} // namespace totalrecruit
