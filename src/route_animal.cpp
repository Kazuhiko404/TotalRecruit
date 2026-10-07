#include "route_decisions.h"
#include "animal_rules.h"
#include "animal_beating_rules.h"

namespace totalrecruit
{
// A loose animal is tamed, not negotiated with. Kept apart from the human
// bargaining path because every term there -- faction standing, bribes, honour,
// military loyalty -- is meaningless to a beast, and letting an animal fall
// through to it produced refusals like "his honour forbids accepting bribes"
// aimed at a pack bull.
//
// The whole cost is food FROM THE PLAYER: a wild animal sits at a full stomach on
// its own, so fullness alone handed one to anyone who spoke to it (Cabra and Goat
// Alpha read "CAN JOIN" never having been fed). It joins once the player has fed
// it at least once (fedByPlayer, feeding_ledger.h) and its stomach has reached the
// bar. Maturity and level set the bar (animal_rules.cpp); the player's own level
// discounts it, which in Kenshi terms is the animal judging whether you can keep
// feeding it.
namespace
{
// The stomach bar this animal has to reach: maturity and level, the player's edge,
// and its temperament.
float TamingBar(const CoreFacts& core, const AnimalExtra& extra, const Config& config)
{
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
    return needed;
}

// The taming verdict against the bar `needed`.
Verdict DecideAgainstBar(const CoreFacts& core, const AnimalExtra& extra, float needed)
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

    const bool fedByPlayer = extra.fedByPlayer > 0;
    const bool pastTheBar = fullness >= needed;
    // Food is offered until both hold: a full animal that never ate from the
    // player's hand still takes the first meal.
    v.wantsFood = !fedByPlayer || !pastTheBar;
    // The bar this verdict is judged against, for the readout: the same two numbers
    // the comparison below uses.
    v.tameFullness = fullness;
    v.tameNeeded = needed;

    if (core.wounded)
    {
        v.reasonKey = Reason_TameWounded;
        return v;
    }

    if (pastTheBar)
    {
        if (!fedByPlayer)
        {
            v.reasonKey = Reason_TameNotFed;
            return v;
        }
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
} // namespace

Verdict DecideAnimal(const CoreFacts& core, const AnimalExtra& extra, const Config& config)
{
    return DecideAgainstBar(core, extra, TamingBar(core, extra, config));
}

// priority-one D: the blows count in the cage. The beating stress lowers the bar
// (animal_beating_rules.h); the first meal and the stomach still decide.
Verdict DecideCagedAnimal(const CoreFacts& core, const AnimalExtra& extra, const Config& config)
{
    const float needed = ApplyAnimalBeating(TamingBar(core, extra, config), core.beatingStress,
                                            core.npcLevel, config.animals);
    return DecideAgainstBar(core, extra, needed);
}
} // namespace totalrecruit
