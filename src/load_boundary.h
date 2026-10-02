#pragma once

// The load boundary (T7): when the player loads or imports a save, everything
// the mod remembers about the world that is going away is dropped BEFORE the
// engine starts building the new one.
//
// Why it is needed: every per-session table here (defeat counts, hail clocks,
// mercy flags, the hold clock, insistence records) is keyed by character and
// lives in plugin memory, not in the save. Loading a save replaces the world, so
// without a reset the previous save's prisoners leak into the new one -- harmless
// with handle keys (they mostly never matched again), wrong with persisted ids
// (a character present in both saves would inherit the other save's history).
// The raw Character* the mod holds (hover target, clicked target, conversation,
// open offer window) point into memory the load is about to free.
//
// Hooks SaveManager::loadGame and SaveManager::importGame, exactly where
// TotalSparring and Proving Grounds reset their identity. Deliberately NOT
// GameWorld::loadAllPlatoons: that runs INSIDE the load, after characters have
// been bound to the ids they read back from the save, so a reset there would
// throw those away.

namespace totalrecruit
{
namespace LoadBoundary
{
// Hooks both functions. False when either could not be placed; the identity
// registry and the session tables are then not reset at a load (the destructor
// hook still unbinds every destroyed character).
bool Install();

// The reset itself, in one place. Public so a later task can call it from
// another boundary (new game, return to the main menu).
void ResetSessionState();
} // namespace LoadBoundary
} // namespace totalrecruit
