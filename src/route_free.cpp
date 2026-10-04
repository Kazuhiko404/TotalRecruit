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

// Today's ally roll: level, relation and personality, plus what standing this good
// is worth on its own. Every ally used to get only this; Balanced with the tiers
// switched off still does, and so does a Good ally under the first cut level.
Verdict AllyRoll(const CoreFacts& core, const FreeExtra& extra, const Config& config, Verdict v)
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

    if (RollsToJoin(extra.roll.haveKey, extra.roll.key, extra.roll.dayIndex, chance))
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

ReasonKey AllyRefusalReason(AllyTierReason reason)
{
    switch (reason)
    {
    case AllyTierReason_Greedy:        return Reason_AllyGreedy;
    case AllyTierReason_Cruel:         return Reason_AllyCruel;
    case AllyTierReason_LeaderTooWeak:
    default:                           return Reason_AllyLeaderTooWeak;
    }
}

// How long a temporary member stays, drawn on the NPC's own daily seed so asking
// twice in a day gives the same answer. Without a key (a peek) there is nothing to
// draw from, and the shortest stay is said rather than an invented one.
int TemporaryDays(const RollFact& roll, const AllyTiers& tiers)
{
    if (!roll.haveKey)
    {
        return tiers.tempMemberMinDays;
    }
    return TemporaryDaysFromSeed(DailySeed(roll.key, roll.dayIndex, kTempDaysSalt), tiers);
}

// A Balanced ally: their level tier decides the deal (ally_tier_rules.h).
Verdict DecideTieredAlly(const CoreFacts& core, const FreeExtra& extra, const Config& config,
                         const AllyTierDecision& tier, Verdict v)
{
    v.allyTiered = true;
    v.allyLevel = core.npcLevel;

    switch (tier.mode)
    {
    case AllyTierMode_Free:
        return AllyRoll(core, extra, config, v);

    case AllyTierMode_Paid:
        // No roll: paying is the gate, as for any other price. The friendship
        // discount is already in extra.price (character_facts GatherFreeExtra puts
        // 1 - discount in its faction multiplier), so the quote -- and with it the
        // purse check, the offended markup and the haggle slider -- is taken as is.
        v.allyDiscountPercent = DiscountPercent(tier.discountFraction);
        ApplyCatsOffer(&v, extra.price, extra.purse, extra.haggle, config.prices);
        return v;

    case AllyTierMode_Temporary:
        // They come, for a while: temp_member.h takes the days once recruited.
        v.willJoin = true;
        v.currency = Currency_None;
        v.reasonKey = Reason_AllyTemporary;
        v.temporary = true;
        v.temporaryDays = TemporaryDays(extra.roll, config.allyTiers);
        return v;

    case AllyTierMode_Refuse:
    default:
        v.currency = Currency_Refused;
        v.reasonKey = AllyRefusalReason(tier.reason);
        return v;
    }
}
} // namespace

bool DecideAccessRefusal(const CoreFacts& core, const AccessFact& access, Verdict* out)
{
    if (access.inAnyCage)
    {
        Verdict v = BaseVerdict(core);
        v.reasonKey = Reason_InOthersCage;
        *out = v;
        return true;
    }

    // Insistence check: the player has pestered them once too often.
    if (access.ignoringPlayer)
    {
        Verdict v = BaseVerdict(core);
        v.currency = Currency_Refused;
        v.reasonKey = Reason_Insistence;
        *out = v;
        return true;
    }
    return false;
}

bool ServesStandingOrganisation(Archetype archetype)
{
    return archetype == Archetype_Military || archetype == Archetype_LawEnforcement;
}

bool DecideBalancedAllyTier(const CoreFacts& core, const RelationFact& relation,
                            const Config& config, AllyTierDecision* out)
{
    if (config.preset != Preset_Balanced || !config.allyTiers.enabled || core.animal ||
        !IsFriendly(relation, config))
    {
        return false;
    }
    *out = DecideAllyTier(core.npcLevel, core.playerLevel, GroupOf(core.temperament),
                          relation.value, config.allyTiers);
    return true;
}

bool FreeWorldMayRoll(const CoreFacts& core, const RelationFact& relation, const Config& config)
{
    return !core.animal && IsFriendly(relation, config);
}

// The free-world conversation: someone who is not in your cage.
Verdict DecideFreeWorld(const CoreFacts& core, const FreeExtra& extra, const Config& config)
{
    Verdict v = BaseVerdict(core);

    // Another cage, or pestered once too often (shared with field-yield).
    if (DecideAccessRefusal(core, extra.access, &v))
    {
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
    // so refusing outright reads as a bug, not as a rule. In Balanced the ally's
    // level tier decides the deal and every refusal names its reason; elsewhere it
    // is today's roll.
    if (IsFriendly(extra.relation, config))
    {
        AllyTierDecision tier;
        if (DecideBalancedAllyTier(core, extra.relation, config, &tier))
        {
            return DecideTieredAlly(core, extra, config, tier, v);
        }
        return AllyRoll(core, extra, config, v);
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
    if (ServesStandingOrganisation(core.archetype))
    {
        v.currency = Currency_Refused;
        v.reasonKey = Reason_MilitaryLoyalty;
        return v;
    }

    // Everyone else takes a bribe, at a price.
    ApplyCatsOffer(&v, extra.price, extra.purse, extra.haggle, config.prices);
    return v;
}
} // namespace totalrecruit
