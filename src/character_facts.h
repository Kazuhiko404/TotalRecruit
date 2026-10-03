#pragma once

// Reads the engine and answers, in plain values, what the rules ask about an NPC.
//
// This is the only place the decision path touches the game. Evaluate gathers
// the facts here (entry_facts.h), the route rules (route_*.cpp) decide from them,
// and nothing in between reads the engine again -- so a level is read once, not
// once per route, and a rule can be tested without a game.
//
// Engine pointers can already be freed, and reading one access-violates rather
// than returning null, so every dereference is guarded. Each guard keeps a
// POD-only frame: C2712 counts every C++ object in the whole function, not just
// the ones inside the __try block.

#include "entry_facts.h"

class Character;

namespace totalrecruit
{
// Personality and archetype come from StateBroadcastData, which the engine
// populates for every Character -- including ones added by mods that have never
// heard of this plugin. That is what makes universal coverage possible.
struct Traits
{
    int npcType;
    int personality;
    // The same personality as the engine-free enum the chance rules take.
    Temperament temperament;
    int slaveState;
    bool escapedPrisoner;

    Traits()
        : npcType(0), personality(0), temperament(Temperament_Normal),
          slaveState(0), escapedPrisoner(false)
    {
    }
};

bool SafeReadTraits(Character* character, Traits* out);

// What every route may look at, read once.
void GatherCoreFacts(Character* character, CoreFacts* out);

// A decision may leave marks (it starts a prisoner's hold clock, it mints the id a roll
// is seeded from). A peek is the same read with none of that, for the readout: it
// looks at a clock that is already running and supplies no roll key at all, so the
// roll cannot pass and nobody gets an id for being looked at.
enum GatherMode
{
    Gather_Decide,
    Gather_Peek
};

// What one route needs beyond the core, read only when that route answers.
CheatFacts GatherCheatFacts(Character* character, GatherMode mode = Gather_Decide);
RescueExtra GatherRescueExtra(Character* character, GatherMode mode = Gather_Decide);
YieldExtra GatherYieldExtra(Character* character, GatherMode mode = Gather_Decide);
FreeExtra GatherFreeExtra(Character* character, const CoreFacts& core,
                          GatherMode mode = Gather_Decide);
CageExtra GatherCageExtra(Character* character, const CoreFacts& core,
                          GatherMode mode = Gather_Decide);

// The haggling speaker's weight: the best of Strength, MeleeAttack and Toughness of
// the player character doing the talking, truncated to a whole point. False when the
// stats cannot be read; the leverage math itself is HaggleLeverage (haggle_rules.h).
bool SafeSpeakerBestStat(Character* speaker, int* out);

// Everything a haggle over this NPC's price reads from the engine (entry_facts.h
// HaggleTerms). Mints the roll id like a decision does: only called for a
// cats offer the player is looking at, never for a peek. roll.haveKey false (no
// id or no clock) means there is no stable seed and so no haggle.
HaggleTerms ReadHaggleTerms(Character* npc);

// Why a character was judged out cold, in the log. Written whenever a verdict
// turns on medical state, because those are the calls that have been wrong before
// and the headers cannot say which flag lied.
void LogUnconsciousCondition(Character* character);
} // namespace totalrecruit
