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
struct RollFact;   // entry_facts.h

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

// Faction standing as a percentage shift: chance.relationWeight of the relation
// (a quarter by default).
int ChanceFromRelation(float relation, bool haveRelation, const Chance& chance);

// Temperament: not who is easiest to bully, who is easiest to convince. The
// offsets are chance.personality[] (Chance's constructor holds the defaults).
int ChanceFromPersonality(Temperament personality, const Chance& chance);

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

// The four terms StressFromState adds up, each already weighted and not yet clamped,
// so a readout can say what the bar is made of. Their sum, clamped to 0..1, IS
// StressFromState; the two cannot drift because StressFromState is built from this.
// `beaten` is the beating stress (beating_stress_rules.h): already 0..1 and weighed
// by its own gains, so it enters as it is (clamped).
struct StressTerms
{
    float hunger;
    float wounds;
    float tired;
    float beaten;
};
StressTerms StressTermsFromState(float rawHunger, float worstDamage, float restedState,
                                 float beatingStress, const Submission& sub);

// How worn down a prisoner is, 0..1: an empty stomach, the worst wound and missing
// rest (present state: feed them and it falls), plus what Golpear and Noquear left
// (the beating stress, fading on its own clock).
float StressFromState(float rawHunger, float worstDamage, float restedState,
                      float beatingStress, const Submission& sub);

// Sum of every term, clamped to [floor, ceiling]. Neither end is ever certain.
int JoinChance(const ChanceInputs& in, const Chance& chance, const Submission& sub);

// The stable roll: the same (index, serial) and percent always give the same
// answer, so a refusal is never an invitation to ask again. This form is the
// handle fallback: the engine handle holds inside one process but is reassigned
// when the game is relaunched, so it does NOT travel with the save. The CharKey
// form below is the one the mod rolls with.
bool StableRoll(unsigned int index, unsigned int serial, int percent);

// The same roll seeded from a CharKey, for a whole lifetime. A key that carries a
// persisted id is seeded from the id, so an NPC keeps their luck across a game
// relaunch; a handle-only key (the fallback) is seeded exactly as above. The
// verdict no longer rolls with this: it rolls once per in-game day (DailyRoll).
bool StableRoll(const CharKey& key, int percent);

// Whole in-game days since the clock started: floor(gameHours / 24). The day is
// what a daily roll is re-seeded by, so "come back tomorrow" means something.
int DayIndexFromHours(double gameHours);

// Salts that keep two daily draws about the same NPC on the same day apart: the
// join roll and the haggle roll must not be the same coin.
const unsigned int kJoinRollSalt   = 0x4A4F494Eu;   // "JOIN"
const unsigned int kHaggleRollSalt = 0x48414747u;   // "HAGG"

// The 32-bit seed of one daily draw: the key's own hash, mixed with the day and
// the salt through a full avalanche, so consecutive days (and neighbouring ids)
// land in unrelated buckets rather than drifting by one.
unsigned int DailySeed(const CharKey& key, int dayIndex, unsigned int salt);

// A draw that holds for the whole of one in-game day and is drawn afresh the
// next: DailySeed % 100 < percent. 100 and above always passes, 0 and below never
// does. Monotonic in the percent within a day.
bool DailyRoll(const CharKey& key, int dayIndex, unsigned int salt, int percent);

// A chance the player cannot re-roll by clicking. PLAN.md settled that this mod
// has no dice, and the reason was save-scumming: a percentage rolled per click is
// really "click until it works". The roll is seeded from the character and the
// in-game day (DailyRoll under kJoinRollSalt), so they see a real chance that
// stays decided for the rest of the day and is asked again the next one. 100 and
// above always joins, 0 and below never does, and with no key to seed from there
// is no stable answer to give -- inventing one that changes per call is worse
// than refusing. The day arrives as an argument (RollFact.dayIndex, read on the
// engine side from the same game clock as the hold clock); this stays pure.
bool RollsToJoin(bool haveKey, const CharKey& key, int dayIndex, int percent);

// The roll a route draws on, from what the engine side read: the character's key
// (`haveKey` false when it could not be read) and the game clock (`haveClock`
// false when it could not be read). Both or no key: without either there is no
// stable seed. Which key is the caller's choice -- the click mints one
// (SafeCharKey), the card's peek only finds the existing one (SafeFindCharKey) --
// so for an NPC that already has an id both draw the same roll and agree.
RollFact RollFactFrom(bool haveKey, const CharKey& key, bool haveClock, double gameHours);
} // namespace totalrecruit
