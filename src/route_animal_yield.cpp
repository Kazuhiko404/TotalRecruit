#include "route_decisions.h"

namespace totalrecruit
{
// animal-duel-submission D3 -- "para Balanced ... el mismo 2/2 que ya tenemos con NPC,
// aplicar la misma logica". A free animal the player's squad has beaten
// defeatsToBreak times in the open yields the way a person does in the field: the
// same JoinChance, the same field-yield bonus, the same daily seed. Two terms do not
// apply to a beast: it has no temperament here (it counts as Normal) and it serves no
// standing organisation (no loyalty penalty).
//
// The person's field yield answers alone; an animal's does not close the taming door.
// Before D3 a beaten animal was simply tamed with food, and two knockouts must not
// take that away: when the roll fails the taming route answers, so a fed animal past
// the bar still follows, and an unfed one keeps the feed button and its stomach bar
// under the "not yet" line.
Verdict DecideAnimalFieldYield(const CoreFacts& core, const YieldExtra& yield,
                               const AnimalExtra& animal, const Config& config)
{
    Verdict v = BaseVerdict(core);
    v.isAnimal = true;
    // Another faction's prisoner is not the player's to ask. The insistence lockout
    // never applies to an animal (DecideAccessRefusal skips it).
    if (DecideAccessRefusal(core, yield.access, &v))
    {
        return v;
    }

    ChanceInputs in;
    in.npcLevel     = core.npcLevel;
    in.playerLevel  = core.playerLevel;
    in.relation     = 0.0f;
    in.haveRelation = false;
    in.personality  = Temperament_Normal;
    in.defeats      = core.defeats;
    in.stress       = StressFromState(core.hunger, core.worstDamage, core.restedState,
                                      core.beatingStress, config.submission);
    in.unique       = core.unique;
    in.situation    = config.submission.fieldYieldBonus;

    const int chance = JoinChance(in, config.chance, config.submission);
    v.rolled = true;
    v.chanceInputs = in;
    v.chancePercent = chance;

    if (RollsToJoin(yield.roll.haveKey, yield.roll.key, yield.roll.dayIndex, chance))
    {
        v.willJoin = true;
        v.currency = Currency_Submission;
        v.reasonKey = Reason_AnimalYieldJoins;
        return v;
    }

    // Not yet -- but food still tames it.
    const Verdict tame = DecideAnimal(core, animal, config);
    if (tame.willJoin)
    {
        return tame;
    }
    v.currency = tame.currency;
    v.wantsFood = tame.wantsFood;
    v.tameFullness = tame.tameFullness;
    v.tameNeeded = tame.tameNeeded;
    v.reasonKey = Reason_AnimalYieldNotYet;
    return v;
}
} // namespace totalrecruit
