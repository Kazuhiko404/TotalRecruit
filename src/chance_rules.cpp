#include "chance_rules.h"
#include "entry_facts.h"

#include <cmath>

namespace totalrecruit
{
namespace
{
// The span is 0..3. KEP writes hunger = 3.0f to repair a broken medical state
// and treats hunger < 0 as invalid, and a fed pack beast measured 2.99 in game.
const float kHungerFull  = 3.0f;
const float kHungerEmpty = 0.0f;

float Clamp01(float v)
{
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}
} // namespace

// The base is deliberately well below the ceiling. An earlier version started
// at 100 for anyone level-equal, which meant every other term landed on an
// already-full number and vanished into the clamp. A middling start is what
// gives the rest of the engine room to matter.
int ChanceFromLevelGap(int npcLevel, int playerLevel, const Chance& c)
{
    const int gap = npcLevel - playerLevel;

    if (gap > 0)
    {
        const int chance = c.levelBase - (int)(gap * c.levelGapPenalty);
        return chance < c.levelGapFloor ? c.levelGapFloor : chance;
    }

    const int bonus = (int)((-gap) * c.levelEdgeBonus);
    return c.levelBase + (bonus > c.levelEdgeCap ? c.levelEdgeCap : bonus);
}

int ChanceFromRelation(float relation, bool haveRelation, const Chance& chance)
{
    if (!haveRelation)
    {
        return 0;
    }
    // chance.relationWeight of the relation: +100 adds 25 points at the default
    // quarter, -100 removes 25.
    return (int)(relation * chance.relationWeight);
}

int ChanceFromPersonality(Temperament personality, const Chance& chance)
{
    if ((int)personality < 0 || (int)personality >= (int)Temperament_Count)
    {
        return 0;
    }
    return chance.personality[personality];
}

float ResolveMultiplier(Temperament personality, const Submission& sub)
{
    switch (personality)
    {
    case Temperament_Fearful:
    case Temperament_Dumb:
        return sub.fearfulResolve;
    case Temperament_Brave:
        return sub.braveResolve;
    case Temperament_Honorable:
        return sub.honourableResolve;
    default:
        return 1.0f;
    }
}

int ChanceFromStress(float stress, Temperament personality, const Submission& sub)
{
    if (stress <= 0.0f)
    {
        return 0;
    }
    const float resolve = ResolveMultiplier(personality, sub);
    const float raw = (stress * sub.maxStressChance) /
                      (resolve > 0.01f ? resolve : 1.0f);
    const int value = (int)raw;
    return value > sub.maxStressChance ? sub.maxStressChance : value;
}

// Divided by resolve rather than multiplied: a high resolve means each defeat
// counts for less.
int ChanceFromDefeats(int defeats, Temperament personality, const Submission& sub)
{
    if (defeats <= 0 || !sub.countDefeats)
    {
        return 0;
    }

    const float resolve = ResolveMultiplier(personality, sub);
    const float raw = (float)(defeats * sub.chancePerDefeat) /
                      (resolve > 0.01f ? resolve : 1.0f);

    const int value = (int)raw;
    return value > sub.maxDefeatChance ? sub.maxDefeatChance : value;
}

// Loyalty is the whole of what an honourable or a brave NPC is; forgiving this
// for cats or for a beating would cost them the only thing the mod says about
// them. Everyone else remembers it as a grievance, which is a weight rather
// than a wall.
bool GrudgeIsUnforgivable(Temperament personality, const Chance& chance)
{
    if (!chance.grudgeIsAbsolute)
    {
        return false;
    }
    return personality == Temperament_Honorable ||
           personality == Temperament_Brave ||
           personality == Temperament_WarmKind;
}

float HungerToFullness01(float rawHunger)
{
    return Clamp01((rawHunger - kHungerEmpty) / (kHungerFull - kHungerEmpty));
}

StressTerms StressTermsFromState(float rawHunger, float worstDamage, float restedState,
                                 const Submission& sub)
{
    // Hunger counts fullness, so an empty stomach is the top of this term.
    const float starved = 1.0f - HungerToFullness01(rawHunger);
    // worstDamage is the state of whichever part is in the worst shape.
    const float hurt = Clamp01(worstDamage);
    // restedState counts rest, so exhaustion is its inverse.
    const float tired = Clamp01(1.0f - restedState);

    StressTerms terms;
    terms.hunger = starved * sub.stressFromHunger;
    terms.wounds = hurt * sub.stressFromWounds;
    terms.tired  = tired * sub.stressFromExhaustion;
    return terms;
}

float StressFromState(float rawHunger, float worstDamage, float restedState,
                      const Submission& sub)
{
    const StressTerms t = StressTermsFromState(rawHunger, worstDamage, restedState, sub);
    return Clamp01(t.hunger + t.wounds + t.tired);
}

int JoinChance(const ChanceInputs& in, const Chance& c, const Submission& sub)
{
    int chance = ChanceFromLevelGap(in.npcLevel, in.playerLevel, c)
               + ChanceFromRelation(in.relation, in.haveRelation, c)
               + ChanceFromPersonality(in.personality, c)
               + ChanceFromDefeats(in.defeats, in.personality, sub)
               + ChanceFromStress(in.stress, in.personality, sub)
               + in.situation
               - (in.grudge ? c.grudgePenalty : 0)
               - (in.unique ? c.uniquePenalty : 0);

    // A floor above zero keeps a hopeless case merely unlikely -- which is what
    // makes relation and personality worth improving -- and a ceiling below 100
    // keeps the strongest player from collecting anyone they point at.
    if (chance < c.floor)   chance = c.floor;
    if (chance > c.ceiling) chance = c.ceiling;
    return chance;
}

bool StableRoll(unsigned int index, unsigned int serial, int percent)
{
    if (percent >= 100) return true;
    if (percent <= 0)   return false;

    // Knuth's multiplicative hash on both halves, so neighbouring slots do not
    // land in neighbouring buckets.
    const unsigned int seed = (index * 2654435761u) ^ (serial * 40503u);
    return (int)((seed * 2654435761u) % 100u) < percent;
}

namespace
{
// Murmur3's 32-bit finaliser: every input bit reaches every output bit.
unsigned int Fmix32(unsigned int h)
{
    h ^= h >> 16;
    h *= 0x85ebca6bu;
    h ^= h >> 13;
    h *= 0xc2b2ae35u;
    h ^= h >> 16;
    return h;
}

// FNV-1a over the id's bytes, then Murmur3's finaliser so the low bits (the
// ones "% 100" reads) depend on every byte: ids that differ in one character
// must not land in neighbouring buckets.
unsigned int HashId(const std::string& id)
{
    unsigned int h = 2166136261u;
    for (std::string::size_type i = 0; i < id.size(); ++i)
    {
        h ^= (unsigned char)id[i];
        h *= 16777619u;
    }
    return Fmix32(h);
}

// The key's own hash, before any day or salt: the id when there is one, the
// handle otherwise.
unsigned int HashKey(const CharKey& key)
{
    if (key.HasId())
    {
        return HashId(key.id);
    }
    return Fmix32((key.index * 2654435761u) ^ (key.serial * 40503u));
}
} // namespace

int DayIndexFromHours(double gameHours)
{
    return (int)std::floor(gameHours / 24.0);
}

// The day and the salt go through their own avalanche before they meet the key,
// and the result through another, so "day + 1" flips about half the bits of the
// seed instead of nudging the bucket by one.
unsigned int DailySeed(const CharKey& key, int dayIndex, unsigned int salt)
{
    const unsigned int day = Fmix32((unsigned int)dayIndex * 0x9E3779B9u ^ salt);
    return Fmix32(HashKey(key) ^ day);
}

bool DailyRoll(const CharKey& key, int dayIndex, unsigned int salt, int percent)
{
    if (percent >= 100) return true;
    if (percent <= 0)   return false;
    return (int)(DailySeed(key, dayIndex, salt) % 100u) < percent;
}

bool RollsToJoin(bool haveKey, const CharKey& key, int dayIndex, int percent)
{
    if (percent >= 100) return true;
    if (percent <= 0)   return false;
    if (!haveKey)       return false;
    return DailyRoll(key, dayIndex, kJoinRollSalt, percent);
}

RollFact RollFactFrom(bool haveKey, const CharKey& key, bool haveClock, double gameHours)
{
    RollFact fact;
    fact.haveKey = haveKey && haveClock;
    if (haveKey)
    {
        fact.key = key;
    }
    fact.dayIndex = DayIndexFromHours(haveClock ? gameHours : 0.0);
    return fact;
}

bool StableRoll(const CharKey& key, int percent)
{
    if (!key.HasId())
    {
        return StableRoll(key.index, key.serial, percent);
    }

    if (percent >= 100) return true;
    if (percent <= 0)   return false;
    return (int)(HashId(key.id) % 100u) < percent;
}
} // namespace totalrecruit
