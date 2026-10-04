#include "route_decisions.h"

namespace totalrecruit
{
// The one honest no -- but only for those who have something to keep faith with.
// ChooseEntryRoute only sends the loyal here (honourable, brave or kind), and only
// outside the player's cage. To them, refusing the person who killed a comrade IS
// the thing that defines them, and no amount of beating should buy it off; the
// reason names no way out because there honestly is none. A turncoat or a
// cold-blooded one never had comrades, only colleagues: they remember, it costs
// the player, and it can still be outweighed (see ChanceInputs::grudge).
Verdict DecideGrudgeRefusal(const CoreFacts& core)
{
    Verdict v = BaseVerdict(core);
    v.currency = Currency_Refused;
    v.reasonKey = Reason_GrudgeNever;
    return v;
}

// Route C -- submission, and it does not go through a cage. This is what beating
// someone repeatedly is supposed to buy.
//
// Being knocked out is not a bar here -- it is the evidence. They lost to you,
// repeatedly, and the wounds are still on them. The offer is made when they come
// round, which is exactly how surrender reads in fiction.
//
// It speaks before the free-world route, so it asks the same access question
// first: another faction's prisoner and someone who has stopped listening are
// not the player's to talk to, beaten or not. Loyalty to a standing organisation
// does not bar the route -- a beating is the one argument such a person hears --
// but it weighs on the roll (fieldYieldLoyaltyPenalty).
Verdict DecideFieldYield(const CoreFacts& core, const YieldExtra& extra, const Config& config)
{
    Verdict v = BaseVerdict(core);
    if (DecideAccessRefusal(core, extra.access, &v))
    {
        return v;
    }

    ChanceInputs in;
    in.npcLevel     = core.npcLevel;
    in.playerLevel  = core.playerLevel;
    in.relation     = 0.0f;
    in.haveRelation = false;
    in.personality  = core.temperament;
    in.defeats      = core.defeats;
    in.stress       = StressFromState(core.hunger, core.worstDamage, core.restedState,
                                      core.beatingStress, config.submission);
    in.unique       = core.unique;
    in.situation    = config.submission.fieldYieldBonus -
                      (ServesStandingOrganisation(core.archetype)
                           ? config.submission.fieldYieldLoyaltyPenalty : 0);

    const int chance = JoinChance(in, config.chance, config.submission);
    v.rolled = true;
    v.chanceInputs = in;
    v.chancePercent = chance;

    v.currency = Currency_Submission;
    if (RollsToJoin(extra.roll.haveKey, extra.roll.key, extra.roll.dayIndex, chance))
    {
        v.willJoin = true;
        v.reasonKey = Reason_FieldYieldJoins;
        return v;
    }

    // Not yet, and the reason names what would change it.
    v.reasonKey = Reason_FieldYieldNotYet;
    return v;
}

// Route C -- surrender. Someone you beat, still on their feet but with the fight
// gone, can yield rather than be cut down.
Verdict DecideSurrendered(const CoreFacts& core)
{
    Verdict v = BaseVerdict(core);
    v.willJoin = true;
    v.currency = Currency_Submission;
    v.reasonKey = Reason_Surrendered;
    return v;
}
} // namespace totalrecruit
