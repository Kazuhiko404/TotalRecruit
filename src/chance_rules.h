#pragma once

// The join-chance engine, without the engine.
//
// Every route that asks "given how they feel about us, how the levels compare
// and who they are, will they come?" does it through JoinChance. Each input is
// its own small function so a wrong number traces to one term, and none of them
// read the game: levels, relation, stress and the config structs arrive as
// arguments. That is what lets tests/rules_tests compile this file on its own.

#include "config.h"
#include "char_key.h"
#include "temperament.h"

namespace totalrecruit
{
struct ChanceInputs
{
    int npcLevel;
    int playerLevel;
    float relation;
    bool haveRelation;
    Temperament personality;
    // Route bonus: how much this particular situation argues for them coming.
    // A saved life is worth more than a conversation.
    int situation;
    // How worn down they are right now, 0..1. See StressFromState.
    float stress;
    // How many times the player has put them down. Weighted by temperament,
    // because the same beating does not mean the same thing to a coward and to
    // someone who intends to die on their feet.
    int defeats;
    // They remember the player killing one of theirs. Only reaches here for
    // temperaments that can be talked round at all -- the rest never get this
    // far.
    bool grudge;
    // A named character. There is only one of them and no second attempt, so
    // they cost more work than anyone else at the same level.
    bool unique;

    ChanceInputs()
        : npcLevel(1), playerLevel(1), relation(0.0f), haveRelation(false),
          personality(Temperament_Normal), situation(0), stress(0.0f),
          defeats(0), grudge(false), unique(false)
    {
    }
};

// Level gap as a weight, never a gate. Above the player it costs, below the
// player it pays, and the bonus is capped.
int ChanceFromLevelGap(int npcLevel, int playerLevel, const Chance& chance);

// Faction standing as a percentage shift: a quarter of the relation.
int ChanceFromRelation(float relation, bool haveRelation);

// Temperament: not who is easiest to bully, who is easiest to convince.
int ChanceFromPersonality(Temperament personality);

// R3.5 -- how much a temperament divides what a beating is worth.
float ResolveMultiplier(Temperament personality, const Submission& sub);

// What being worn down argues for, divided by resolve.
int ChanceFromStress(float stress, Temperament personality, const Submission& sub);

// R3.2 -- how much having been beaten argues for yielding.
int ChanceFromDefeats(int defeats, Temperament personality, const Submission& sub);

// Who cannot be bought off after the player killed one of theirs: the
// honourable, the brave and the kind, when the config says the grudge is
// absolute.
bool GrudgeIsUnforgivable(Temperament personality, const Chance& chance);

// The engine's hunger field counts fullness on 0..3 (3 is a full stomach).
// This is the honest 0..1, where 1 is sated and 0 is starving.
float HungerToFullness01(float rawHunger);

// How worn down a prisoner is, 0..1, derived from present state only: an empty
// stomach, the worst wound, and missing rest. Feed them and it falls.
float StressFromState(float rawHunger, float worstDamage, float restedState,
                      const Submission& sub);

// Sum of every term, clamped to [floor, ceiling]. Neither end is ever certain.
int JoinChance(const ChanceInputs& in, const Chance& chance, const Submission& sub);

// The stable roll: the same (index, serial) and percent always give the same
// answer, so a refusal is never an invitation to ask again. This form is the
// handle fallback: the engine handle holds inside one process but is reassigned
// when the game is relaunched, so it does NOT travel with the save. The CharKey
// form below is the one the mod rolls with.
bool StableRoll(unsigned int index, unsigned int serial, int percent);

// The same roll seeded from a CharKey. A key that carries a persisted id is
// seeded from the id, so an NPC keeps their luck across a game relaunch; a
// handle-only key (the fallback) is seeded exactly as above.
bool StableRoll(const CharKey& key, int percent);
} // namespace totalrecruit
