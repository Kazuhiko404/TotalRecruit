#include "route_decisions.h"
#include "relation_rules.h"

namespace totalrecruit
{
namespace
{
bool IsGoodNatured(Temperament temperament)
{
    return temperament == Temperament_Honorable || temperament == Temperament_WarmKind;
}

bool IsFriendly(const RelationFact& relation, const Config& config)
{
    return relation.known && relation.value >= config.friendlyRelation;
}
} // namespace

bool FreeWorldMayRoll(const CoreFacts& core, const RelationFact& relation, const Config& config)
{
    return !core.animal && IsFriendly(relation, config);
}

// The free-world conversation: someone who is not in your cage.
Verdict DecideFreeWorld(const CoreFacts& core, const FreeExtra& extra, const Config& config)
{
    Verdict v = BaseVerdict(core);

    if (extra.inAnyCage)
    {
        v.reasonKey = Reason_InOthersCage;
        return v;
    }

    // Insistence check: the player has pestered them once too often.
    if (extra.ignoringPlayer)
    {
        v.currency = Currency_Refused;
        v.reasonKey = Reason_Insistence;
        return v;
    }

    // Animals leave the human path here. Everything below this line -- faction
    // standing, bribes, honour, military loyalty -- is a human argument, and
    // letting a beast fall through to it is what produced refusals like "his honour
    // forbids accepting bribes" aimed at a pack bull. Taming is its own negotiation
    // and it is settled in food.
    if (core.animal)
    {
        return DecideAnimal(core, extra.animal, config);
    }

    // Faction relation, read once and used for everything below: who will talk, who
    // will take money, and how they open. It is still never a flat gate for people
    // who like you -- only for the ones who do not.
    v.band = extra.relation.known ? RelationBand(extra.relation.value) : "neutral";
    const bool goodNatured = IsGoodNatured(core.temperament);

    // Allies. The player already knows they are allies -- the cursor goes yellow --
    // so refusing outright reads as a bug, not as a rule.
    if (IsFriendly(extra.relation, config))
    {
        ChanceInputs in;
        in.npcLevel     = core.npcLevel;
        in.playerLevel  = core.playerLevel;
        in.relation     = extra.relation.value;
        in.haveRelation = extra.relation.known;
        in.personality  = core.temperament;
        // Standing this good is itself an argument.
        in.situation    = config.chance.alliedBonus;

        const int chance = JoinChance(in, config.chance, config.submission);
        v.rolled = true;
        v.chanceInputs = in;
        v.chancePercent = chance;

        if (RollsToJoin(extra.roll.haveKey, extra.roll.key, chance))
        {
            v.willJoin = true;
            v.currency = Currency_None;
            v.reasonKey = Reason_AllyJoins;
            return v;
        }

        // Not a refusal to be argued with -- a "not you, not yet", which is what a
        // level gap actually means between allies.
        v.currency = Currency_Refused;
        v.reasonKey = Reason_AllyMaybe;
        return v;
    }

    // Below relationRefuseGoodNatured an honourable or kind NPC has nothing to
    // discuss. Their objection is who you are, and no amount of money answers that.
    if (extra.relation.known && extra.relation.value < config.relationRefuseGoodNatured &&
        goodNatured)
    {
        v.currency = Currency_Refused;
        v.reasonKey = Reason_FactionHonour;
        return v;
    }

    // Below relationRefuseAll even the greedy stop listening. Money works because
    // the seller expects to live to spend it; past open war that stops holding.
    if (extra.relation.known && extra.relation.value < config.relationRefuseAll)
    {
        v.currency = Currency_Refused;
        v.reasonKey = Reason_FactionGreedy;
        return v;
    }

    // A free NPC bargains. The level gap used to end the conversation here, above a
    // fixed 15: it was the one refusal in the mod that named no way out. It feeds
    // JoinChance like everything else now, so a stronger NPC is harder to win over
    // rather than sealed off.

    // Honourable or kind personalities never accept cats.
    if (goodNatured)
    {
        if (core.starving)
        {
            v.currency = Currency_Care;
            v.reasonKey = Reason_HonourStarving;
        }
        else
        {
            v.currency = Currency_Refused;
            v.reasonKey = Reason_HonourRefusesBribe;
        }
        return v;
    }

    // Nobody who serves a standing organisation can be bought off in the open. A
    // reason is only worth showing if it names the way out; two exist here, so both
    // are offered rather than only the one the mod happens to prefer.
    if (core.archetype == Archetype_Military || core.archetype == Archetype_LawEnforcement)
    {
        v.currency = Currency_Refused;
        v.reasonKey = Reason_MilitaryLoyalty;
        return v;
    }

    // Everyone else takes a bribe, at a price.
    ApplyCatsOffer(&v, extra.price, extra.purse, config.prices);
    return v;
}
} // namespace totalrecruit
