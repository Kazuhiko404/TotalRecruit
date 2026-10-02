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

// What one route needs beyond the core, read only when that route answers.
CheatFacts GatherCheatFacts(Character* character);
RescueExtra GatherRescueExtra(Character* character);
YieldExtra GatherYieldExtra(Character* character);
FreeExtra GatherFreeExtra(Character* character, const CoreFacts& core);
CageExtra GatherCageExtra(Character* character, const CoreFacts& core);

// Why a character was judged out cold, in the log. Written whenever a verdict
// turns on medical state, because those are the calls that have been wrong before
// and the headers cannot say which flag lied.
void LogUnconsciousCondition(Character* character);
} // namespace totalrecruit
