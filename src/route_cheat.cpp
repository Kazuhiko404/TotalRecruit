#include "route_decisions.h"
#include "cheat_waits.h"

namespace totalrecruit
{
namespace
{
// An animal that ate from the player's hand and belongs to nobody: in Cheat the
// meal is the gesture (animal-insistence-cheat-feed A2). One that belongs to
// someone keeps Cheat's only way in for it, the cage.
bool FreeAnimal(const CheatFacts& facts)
{
    return facts.animal && (facts.inMyCage || !facts.belongsToSomeone);
}

// The other two gestures a free animal answers in Cheat (animal-duel-submission D4):
// the player healed it, or the player's squad beat it. The meal is named first, then
// the cure, then the beating. Reason_None when none of them happened.
ReasonKey AnimalGestureReason(const CheatFacts& facts)
{
    if (facts.healedByPlayer)
    {
        return Reason_CheatAnimalHealed;
    }
    if (facts.defeatedByPlayer)
    {
        return Reason_CheatAnimalDefeated;
    }
    return Reason_None;
}
} // namespace

// "Para cheat mode sacar la limitacion de solo reclutar de tu celda": with
// cheatAnyCage on, another faction's cage, shackles or a kidnap is a cage too.
bool CheatCageSatisfied(bool inMyCage, bool inAnyCage, const Config& config)
{
    return inMyCage || (config.cheatAnyCage && inAnyCage);
}

bool HoldClockCountsAnyCage(const Config& config)
{
    return config.preset == Preset_Cheat && config.cheatNeedsCage && config.cheatAnyCage;
}

bool HoldClockCounts(bool inMyCage, bool inAnyCage, const Config& config)
{
    return inMyCage || (inAnyCage && HoldClockCountsAnyCage(config));
}

// R1.1 / R1.2 -- Cheat keeps the gesture and drops the price.
//
// It used to return here for anyone, anywhere, which left nothing of the ritual:
// no cage, no wait, no moment where the prisoner decides. The cage is required now
// and the wait is a minute of game time, because in Cheat time is a gesture rather
// than a cost. Nothing else is asked -- no cats, no food, no submission, no level,
// no standing. The one other gesture is for an animal that belongs to nobody: a
// meal from the player, a cure from the player, or a knockout by the player's squad,
// and it joins on the spot, no cage needed.
Verdict DecideCheat(const CheatFacts& facts, const Config& config)
{
    Verdict v;

    if (!config.cheatNeedsCage)
    {
        v.willJoin = true;
        v.reasonKey = Reason_CheatJoinsFreely;
        return v;
    }

    // An animal is tamed with food, in Cheat as in Balanced, but without the bar:
    // one meal from the player's hand and it follows, caged or not, with no wait
    // and no wound, hunger or insistence to argue about.
    if (FreeAnimal(facts) && facts.fedByPlayer)
    {
        v.willJoin = true;
        v.isAnimal = true;
        v.currency = Currency_Care;
        v.reasonKey = Reason_TameReady;
        return v;
    }

    // "Lo mismo con curar", "si lo vence mi NPC deberia dejarme hablar para que se
    // una": a cure from the player's hand or a knockout by the player's squad is the
    // same gesture, caged or not, with no wait.
    const ReasonKey gesture = FreeAnimal(facts) ? AnimalGestureReason(facts) : Reason_None;
    if (gesture != Reason_None)
    {
        v.willJoin = true;
        v.isAnimal = true;
        v.currency = Currency_None;
        v.reasonKey = gesture;
        return v;
    }

    if (!CheatCageSatisfied(facts.inMyCage, facts.inAnyCage, config))
    {
        v.currency = Currency_Refused;
        v.reasonKey = Reason_CheatNeedsCage;
        if (FreeAnimal(facts))
        {
            // Still "put it in a cage", but food is the other way in: care with an
            // appetite is what puts the feed button in the popup (RemedyFor).
            v.isAnimal = true;
            v.currency = Currency_Care;
            v.wantsFood = true;
        }
        return v;
    }

    // One game minute (cheat_waits.h), or cheatHoursCaged when that is shorter.
    if (facts.hoursHeld < CageWaitHours(config))
    {
        v.currency = Currency_Refused;
        v.reasonKey = Reason_CheatWaiting;
        return v;
    }

    v.willJoin = true;
    v.currency = Currency_None;
    v.reasonKey = Reason_CheatJoins;
    return v;
}
} // namespace totalrecruit
