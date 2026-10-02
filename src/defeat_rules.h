#pragma once

// How many defeats a character counts as having suffered, without the engine.
//
// The count is made of two readings that cover each other's blind spot: the
// precise session counter (lost on reload) and the floor the save keeps in the
// engine's per-character memory (survives, but can only say "at least once").
// A reload costs precision, never progress -- which only holds if the floor is
// large enough to keep every route the count unlocked.

namespace totalrecruit
{
// What a reload leaves behind for a character carrying the persistent
// "the player beat my squad once" memory tag, and 0 for one without it.
//
// The configured floor is never allowed below defeatsToBreak: the memory tag is
// written at the first knockout, so a floor of 1 under a threshold of 2 turned
// every reload into a step back from "ready to yield" to "barely touched"
// (REQUISITOS R3.3: the NPC never goes back to intact once defeated).
int PersistedDefeatFloor(bool markedInSave, int configuredFloor, int defeatsToBreak);

// The count the rules read: the precise session count, but never below the floor.
int EffectiveDefeatCount(int sessionCount, int persistedFloor);
} // namespace totalrecruit
