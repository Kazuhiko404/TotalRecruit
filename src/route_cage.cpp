#include "route_decisions.h"

namespace totalrecruit
{
Currency ChooseCurrency(const CoreFacts& core)
{
    if (core.animal)
    {
        return Currency_Care;
    }

    if (core.enslaved || core.archetype == Archetype_Slave)
    {
        return Currency_Freedom;
    }

    if (core.starving || core.wounded)
    {
        return Currency_Care;
    }

    // The honourable and the kind never accept a bribe in cats.
    if (core.temperament == Temperament_Honorable || core.temperament == Temperament_WarmKind)
    {
        return Currency_Care;
    }

    switch (core.archetype)
    {
    case Archetype_Bandit:
    case Archetype_Trader:
    case Archetype_Slaver:
    case Archetype_Civilian:
    case Archetype_Adventurer:
    case Archetype_Diplomat:
        return Currency_Cats;

    case Archetype_Military:
    case Archetype_LawEnforcement:
        return Currency_Submission;

    default:
        return (core.temperament == Temperament_Brave) ? Currency_Submission : Currency_Cats;
    }
}

// A named character is not one kind of problem. The bandit king and the Holy
// Nation's leader are both unique, and only one of them has a price.
//
// The split is what they believe in. Someone who already sells people, or who
// enjoys the cruelty, understands a transaction perfectly well -- money is a
// language they speak. A zealot does not: offering coin to someone who thinks you
// are unclean is the wrong tool entirely, and the only thing that moves them is
// being kept and broken.
//
// Both are recruitable. Neither is free.
bool UniqueTakesMoney(Temperament temperament, Archetype archetype)
{
    switch (temperament)
    {
    case Temperament_ColdCruel:
    case Temperament_Traitorous:
    case Temperament_Maniacal:
    case Temperament_Smart:
        return true;
    default:
        break;
    }

    // Failing personality, the trade they chose says it instead.
    return archetype == Archetype_Bandit || archetype == Archetype_Trader ||
           archetype == Archetype_Slaver;
}

namespace
{
// What a caged person says once the currency table has sent them to "care".
// Animals never get here: DecideCage hands them to DecideAnimal first.
Verdict CareVerdict(Verdict v, const CoreFacts& core)
{
    const bool cared = !core.starving && !core.wounded;

    if (cared)
    {
        v.willJoin = true;
        v.reasonKey = Reason_CaredFor;
    }
    else if (core.starving && core.wounded)
    {
        v.reasonKey = Reason_StarvingAndWounded;
    }
    else if (core.starving)
    {
        v.reasonKey = Reason_Starving;
    }
    else
    {
        v.reasonKey = Reason_Wounded;
    }
    return v;
}
} // namespace

// Route B -- recruit prisoners in your own cage.
Verdict DecideCage(const CoreFacts& core, const CageExtra& extra, const Config& config)
{
    Verdict v = BaseVerdict(core);

    if (!config.captivityEnabled)
    {
        v.reasonKey = Reason_CaptivityDisabled;
        return v;
    }

    // A caged animal is still an animal: it is tamed with the player's food and
    // nothing else, so it settles on the same stomach arithmetic as a loose one
    // (DecideAnimal) before any of the cage's human arguments -- the easy wait,
    // the intimidation roll, the saviour shortcut -- can answer for it. Deciding
    // this twice is how two rules that are meant to be one drift apart. The
    // animal route answers for itself: it does not carry what the cage route set.
    if (core.animal)
    {
        return DecideAnimal(core, extra.animal, config);
    }

    v.viaCaptivity = true;

    // Easy mode: time alone breaks anyone. Whoever they are and whatever they would
    // normally charge, long enough in a cage is its own argument -- and the player
    // only has to wait, not work out what this particular NPC wants.
    if (config.prices.easyHoursCaged > 0.0f && extra.hoursHeld >= config.prices.easyHoursCaged)
    {
        v.willJoin = true;
        v.currency = Currency_Submission;
        v.reasonKey = Reason_EasyWait;
        return v;
    }

    // Overwhelming strength used to be an automatic yes below a gap of -40, which
    // handed the player anyone they had outgrown -- including the honourable
    // characters the mod goes out of its way to make expensive. It is a strong
    // argument now, not a verdict: the edge bonus is already in JoinChance, and
    // this only adds the weight of being caged and outmatched.
    {
        ChanceInputs in;
        in.npcLevel     = core.npcLevel;
        in.playerLevel  = core.playerLevel;
        in.relation     = 0.0f;
        in.haveRelation = false;
        in.personality  = core.temperament;
        // Defeats are what it took to put them in here, and they stop accumulating
        // once the door shuts -- there is nobody left to beat in a fight. Inside,
        // the argument is the stress bar: hunger nobody answered, wounds nobody
        // treated, sleep nobody allowed. Both count, but the cage is where the
        // second one does the work.
        in.defeats      = core.defeats;
        in.stress       = StressFromState(core.hunger, core.worstDamage, core.restedState,
                                          config.submission);
        in.grudge       = core.grudge;
        in.unique       = core.unique;
        in.situation    = config.chance.intimidationBonus;

        const int chance = JoinChance(in, config.chance, config.submission);
        v.rolled = true;
        v.chanceInputs = in;
        v.chancePercent = chance;

        if (RollsToJoin(extra.roll.haveKey, extra.roll.key, extra.roll.dayIndex, chance))
        {
            v.willJoin = true;
            v.currency = Currency_Submission;
            v.reasonKey = Reason_Intimidated;
            return v;
        }
    }

    v.currency = ChooseCurrency(core);

    // A named character overrides the ordinary table, because for them the currency
    // IS the story. Someone starving or bleeding still gets care first -- a leader
    // in that state is a person before they are a symbol.
    if (core.unique && !core.animal && !core.starving && !core.wounded)
    {
        v.currency = UniqueTakesMoney(core.temperament, core.archetype) ? Currency_Cats
                                                                         : Currency_Submission;
    }

    switch (v.currency)
    {
    case Currency_Freedom:
        // Someone you freed from slavers does not need convincing. The cheapest
        // route in the mod, and the most Kenshi.
        v.willJoin = true;
        v.reasonKey = Reason_FreedFollows;
        break;

    case Currency_Care:
        v = CareVerdict(v, core);
        break;


    case Currency_Submission:
    {
        // Being in your cage already proves you beat them -- you cannot cage
        // someone you did not knock down. What the proud still owe is being worn
        // down, and that is read from their condition rather than a clock: hunger
        // and wounds are both things the player can hasten or relieve.
        //
        // (Audit note: ChooseCurrency sends the starving and the wounded to care
        // before anything else, so no input reaches the first branch below. It is
        // kept as it shipped rather than removed in a refactor.)
        const bool wornDown = core.starving || core.wounded;

        if (wornDown)
        {
            v.willJoin = true;
            v.reasonKey = Reason_BrokenNoPride;
        }
        else if (core.unique)
        {
            // No price is coming for this one. Their objection is what they
            // believe, and belief does not have an exchange rate -- only time in a
            // cage and the body's own arguments.
            v.reasonKey = Reason_UniqueZealot;
        }
        else
        {
            v.reasonKey = Reason_Defiant;
        }
        break;
    }

    case Currency_Cats:
    default:
        ApplyCatsOffer(&v, extra.price, extra.purse, extra.haggle, config.prices);
        break;
    }

    return v;
}
} // namespace totalrecruit
