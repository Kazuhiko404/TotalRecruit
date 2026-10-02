#include "route_decisions.h"
#include "body_rules.h"

namespace totalrecruit
{
// Nobody agrees to anything while they are out cold. A deal struck with an
// unconscious body is not a deal, and it undercuts the rescue route, whose whole
// point is that the offer arrives when they wake and find out who saved them.
Verdict DecideUnconscious(const CoreFacts& core, const Config& config)
{
    Verdict v = BaseVerdict(core);

    if (config.rescueEnabled && core.rememberedAsSaviour)
    {
        v.viaRescue = true;
        v.reasonKey = Reason_UnconsciousOwesYou;
    }
    else if (core.dying)
    {
        v.viaRescue = true;
        v.currency = Currency_Care;
        v.reasonKey = Reason_UnconsciousDying;
    }
    else
    {
        v.reasonKey = Reason_UnconsciousWait;
    }
    return v;
}

// Still dying and not yet saved: the route is open, but the price is care.
Verdict DecideDying(const CoreFacts& core)
{
    Verdict v = BaseVerdict(core);
    v.viaRescue = true;
    v.currency = Currency_Care;
    v.reasonKey = Reason_Dying;
    return v;
}

// Easy mode, the healing half: someone patched all the way back up owes the player
// whether or not the engine happened to credit the first aid. The applyFirstAid
// hook only fires while the player's own medic is working, so a prisoner healed by
// a bed or by time would otherwise slip through.
Verdict DecideEasyHealthy(const CoreFacts& core)
{
    Verdict v = BaseVerdict(core);
    v.willJoin = true;
    v.viaRescue = true;
    v.currency = Currency_Care;
    v.reasonKey = Reason_EasyHealthy;
    return v;
}

// Route A -- rescue. Someone you pulled out of the dirt owes you nothing else.
//
// The debt is real, but it is not a contract. Whether they can follow you depends
// on who they are and what they would be walking into -- and a saved life weighs
// more than any other opening, hence the largest situation bonus in the mod.
Verdict DecideRescue(const CoreFacts& core, const RescueExtra& extra, const Config& config)
{
    Verdict v = BaseVerdict(core);
    v.viaRescue = true;

    ChanceInputs in;
    in.npcLevel     = core.npcLevel;
    in.playerLevel  = core.playerLevel;
    in.relation     = extra.relation.value;
    in.haveRelation = extra.relation.known;
    in.personality  = core.temperament;
    in.defeats      = core.defeats;
    // R4.4 -- how much of them the player actually put back together. Bandaging
    // someone and walking off is not the same favour as leaving them whole, and
    // this is the same shape as feeding an animal: the fraction restored is the
    // fraction of the argument earned.
    const float restored = RestoredFraction(core.unconscious, core.bloodFraction, core.health);
    in.situation = config.rescueBaseBonus + (int)(restored * config.rescueHealWeight);

    const int chance = JoinChance(in, config.chance, config.submission);
    v.rolled = true;
    v.chanceInputs = in;
    v.chancePercent = chance;

    if (RollsToJoin(extra.roll.haveKey, extra.roll.key, chance))
    {
        v.willJoin = true;
        v.currency = Currency_None;
        v.reasonKey = Reason_RescueJoins;
        return v;
    }

    // They cannot come, so they pay it forward with what they have. This is the
    // NPC paying the player, never the reverse: charging someone for having saved
    // them would invert the one route in this mod where the player is not
    // exercising power.
    //
    // LT_MY_LIFESAVER persists in the save, so the debt outlives the refusal: meet
    // them again later and the roll is the same, but a changed level or standing
    // can turn it.
    v.currency = Currency_Refused;
    v.owesGift = true;
    v.reasonKey = Reason_RescueRepays;
    return v;
}
} // namespace totalrecruit
