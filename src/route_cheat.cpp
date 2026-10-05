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
} // namespace

// R1.1 / R1.2 -- Cheat keeps the gesture and drops the price.
//
// It used to return here for anyone, anywhere, which left nothing of the ritual:
// no cage, no wait, no moment where the prisoner decides. The cage is required now
// and the wait is a minute of game time, because in Cheat time is a gesture rather
// than a cost. Nothing else is asked -- no cats, no food, no submission, no level,
// no standing. The one other gesture is an animal's meal: fed by the player, an
// animal that belongs to nobody joins on the spot, no cage needed.
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

    if (!facts.inMyCage)
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
