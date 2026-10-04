#pragma once

// What the readings of a body mean, without the engine: wounded, starving, cared for.
//
// These were inline in Evaluate, between the engine reads and the first route.
// They are the bridge from what the engine says to what the rules ask, and every
// one of them has a trap the notes warn about (health reads 0 for anyone out
// cold, hunger counts fullness on 0..3), so each is its own small function.

namespace totalrecruit
{
// Health only means something while they are conscious: getOverallHealthRating
// reads 0 for anyone knocked out, so testing it on a KO'd character would call
// them gravely wounded however intact they are. Out cold, "wounded" is "dying".
bool IsWounded(bool unconscious, bool dying, float health, float woundedThreshold);

// The engine's own isReallyHungry decides by default; `hungerThreshold` is an
// opt-in extra trigger for players who raise it deliberately, as a 0..1 fraction
// of a full stomach (0.4 is "starving below 40% full"; 0 is off). The raw
// reading counts fullness and runs 0..3, so it is normalised before comparing.
bool IsStarving(bool engineSaysStarving, float rawHunger, float hungerThreshold);

// Fed, unwounded and at full health: what the Balanced cage's cared-for bonus
// (cared_for_rules.h) asks for.
bool IsFullyCaredFor(bool wounded, bool starving, float health);

// R4.4 -- how much of them the player put back together, 0..1. Health is only
// readable while they are conscious, so someone still out cold is credited by
// blood, which stays honest either way.
float RestoredFraction(bool unconscious, float bloodFraction, float health);
} // namespace totalrecruit
