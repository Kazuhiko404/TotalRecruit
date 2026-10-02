#include "route_decisions.h"
#include "animal_rules.h"

namespace totalrecruit
{
// A loose animal is tamed, not negotiated with. Kept apart from the human
// bargaining path because every term there -- faction standing, bribes, honour,
// military loyalty -- is meaningless to a beast, and letting an animal fall
// through to it produced refusals like "his honour forbids accepting bribes"
// aimed at a pack bull.
//
// The whole cost is food, and how full the animal already is decides whether that
// cost has been paid. Maturity and level set the bar (animal_rules.cpp); the
// player's own level discounts it, which in Kenshi terms is the animal judging
// whether you can keep feeding it.
Verdict DecideAnimal(const CoreFacts& core, const AnimalExtra& extra, const Config& config)
{
    Verdict v;
    v.isAnimal = true;
    v.currency = Currency_Care;

    // An animal that already belongs to someone is not tamed, it is stolen. Loose
    // wildlife sits in bookkeeping factions the engine marks as not real, while an
    // owned beast carries a real faction of people. So food buys nothing here; the
    // cage does.
    if (!core.inMyCage && extra.belongsToSomeone)
    {
        v.currency = Currency_Refused;
        v.reasonKey = Reason_AnimalOwned;
        return v;
    }

    const float fullness = HungerToFullness01(core.hunger);
    float needed = TamingFullnessNeeded(extra.maturity, core.npcLevel, core.playerLevel,
                                        config.animals);

    // R5.4 -- animals have a temperament, and it varies: two bonedogs of the same
    // breed measured PT_HONORABLE and PT_TRAITOROUS. So it counts here as it does
    // everywhere else, expressed as how much convincing the creature takes rather
    // than as a separate rule.
    if (config.animals.personalityMatters)
    {
        const float resolve = ResolveMultiplier(core.temperament, config.submission);
        needed = ApplyAnimalTemperament(needed, resolve, config.animals);
    }

    v.wantsFood = (fullness < needed);

    if (core.wounded)
    {
        v.reasonKey = Reason_TameWounded;
        return v;
    }

    if (fullness >= needed)
    {
        v.willJoin = true;
        v.reasonKey = Reason_TameReady;
        return v;
    }

    // The gap is what the player acts on, so it is the number the line carries.
    // Percent, not money: the window reads it as the price.
    v.cats = MissingStomachPercent(needed, fullness);
    v.reasonKey = Reason_TameHungry;
    return v;
}
} // namespace totalrecruit
