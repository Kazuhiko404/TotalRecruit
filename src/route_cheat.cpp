#include "route_decisions.h"
#include "cheat_waits.h"

namespace totalrecruit
{
// R1.1 / R1.2 -- Cheat keeps the gesture and drops the price.
//
// It used to return here for anyone, anywhere, which left nothing of the ritual:
// no cage, no wait, no moment where the prisoner decides. The cage is required now
// and the wait is a minute of game time, because in Cheat time is a gesture rather
// than a cost. Nothing else is asked -- no cats, no food, no submission, no level,
// no standing.
Verdict DecideCheat(const CheatFacts& facts, const Config& config)
{
    Verdict v;

    if (!config.cheatNeedsCage)
    {
        v.willJoin = true;
        v.reasonKey = Reason_CheatJoinsFreely;
        return v;
    }

    if (!facts.inMyCage)
    {
        v.currency = Currency_Refused;
        v.reasonKey = Reason_CheatNeedsCage;
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
