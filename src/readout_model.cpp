#include "readout_model.h"
#include "readout_catalog.h"
#include "chance_rules.h"

namespace totalrecruit
{
namespace
{
int Percent(float fraction)
{
    if (fraction < 0.0f) fraction = 0.0f;
    if (fraction > 1.0f) fraction = 1.0f;
    return (int)(fraction * 100.0f + 0.5f);
}

// The verdicts that are not about a person to talk to.
bool IsHiddenReason(ReasonKey key)
{
    return key == Reason_None || key == Reason_Nobody || key == Reason_AnimalsDisabled ||
           key == Reason_Pretending || key == Reason_Dead;
}

ReadoutRoute NameRoute(const ReadoutInput& in)
{
    if (in.mercyPending)
    {
        return ReadoutRoute_Mercy;
    }
    if (in.haveCore && in.core.animal)
    {
        return ReadoutRoute_Animal;
    }
    switch (in.route)
    {
    case Route_Cage:
        return ReadoutRoute_Captivity;
    case Route_Rescue:
    case Route_Dying:
    case Route_Unconscious:
    case Route_EasyHealthy:
        return ReadoutRoute_Rescue;
    case Route_FieldYield:
        return ReadoutRoute_FieldYield;
    case Route_Surrendered:
        return ReadoutRoute_Surrender;
    case Route_GrudgeRefusal:
    case Route_FreeWorld:
    default:
        return ReadoutRoute_Free;
    }
}

// The three parts of the bar, in whole percentage points that add up to the bar.
// Each term is scaled the same way the clamp scales the sum (when a config makes
// the terms add past 1.0 the bar still tops out at 100), floored, and the points
// lost to flooring go to the largest part.
void SplitStress(const ReadoutInput& in, const Submission& sub, int pct, ReadoutView* out)
{
    const StressTerms t = StressTermsFromState(in.core.hunger, in.core.worstDamage,
                                               in.core.restedState, sub);
    const float sum = t.hunger + t.wounds + t.tired;
    if (sum <= 0.0f || pct <= 0)
    {
        out->hungerPts = out->woundsPts = out->tiredPts = 0;
        return;
    }
    const float k = (float)pct / (sum * 100.0f);
    int parts[3] = { (int)(t.hunger * 100.0f * k), (int)(t.wounds * 100.0f * k),
                     (int)(t.tired * 100.0f * k) };
    const int leftover = pct - (parts[0] + parts[1] + parts[2]);
    int biggest = 0;
    if (t.wounds > t.hunger && t.wounds >= t.tired) biggest = 1;
    if (t.tired > t.hunger && t.tired > t.wounds) biggest = 2;
    parts[biggest] += leftover;
    out->hungerPts = parts[0];
    out->woundsPts = parts[1];
    out->tiredPts = parts[2];
}

ReadoutLineView LineFor(int id, const ReadoutInput& in)
{
    ReadoutLineView line;
    line.id = id;
    line.cats = in.verdict.cats;
    line.purse = in.verdict.purse;
    if (in.verdict.willJoin)
    {
        line.tone = ReadoutTone_Good;
    }
    else if (in.verdict.currency == Currency_Refused)
    {
        line.tone = ReadoutTone_Refused;
    }
    return line;
}

// The "not yet" a roll gives when it fails. Without the roll key it did not fail:
// RollsToJoin has no stable answer and says no, so the click (keyed) may still
// say yes. Other rolled routes fall through to a real way in (a price) instead.
bool IsLostRoll(ReasonKey key)
{
    return key == Reason_FieldYieldNotYet || key == Reason_AllyMaybe;
}

// The main line, told truthfully for what the card could and could not know.
ReadoutLineView PrimaryLine(const ReadoutInput& in)
{
    const ReasonKey key = in.verdict.reasonKey;
    if (in.verdict.rolled && !in.rollKeyed && !in.verdict.willJoin && IsLostRoll(key))
    {
        // The odds, with no promise either way.
        ReadoutLineView odds;
        odds.id = kLineRollOpen;
        odds.chance = in.verdict.chancePercent;
        return odds;
    }
    if (key == Reason_FieldYieldNotYet && !(in.haveCore && in.core.inMyCage))
    {
        // Starving them is only on offer for someone in the player's own cage.
        return LineFor(kLineFieldYieldBeatAgain, in);
    }
    if (in.verdict.allyTiered && in.verdict.currency == Currency_Cats)
    {
        // A Balanced ally's price is a friend's price: the card says so.
        if (key == Reason_CatsJoin) return LineFor(kLineAllyPaidJoin, in);
        if (key == Reason_CatsAsk)  return LineFor(kLineAllyPaidAsk, in);
    }
    return LineFor((int)key, in);
}


// A temporary member's or a helper's card: nothing to decide any more, only the days
// they have left.
ReadoutView DaysLeftReadout(int lineId, int daysLeft)
{
    ReadoutView v;
    v.visible = true;
    v.route = ReadoutRoute_Free;
    v.primary.id = lineId;
    v.primary.need = daysLeft;
    v.primary.tone = ReadoutTone_Good;
    return v;
}
} // namespace

ReadoutView BuildReadout(const ReadoutInput& in, const Config& config)
{
    if (in.tempMember)
    {
        return DaysLeftReadout(kLineTempMemberDaysLeft, in.tempDaysLeft);
    }
    // A free helper's card (helper.h): the same shape, their own line.
    if (in.helper)
    {
        return DaysLeftReadout(kLineHelperDaysLeft, in.helperDaysLeft);
    }
    ReadoutView v;
    if (IsHiddenReason(in.verdict.reasonKey))
    {
        return v;
    }
    v.visible = true;
    v.route = NameRoute(in);

    const bool person = in.haveCore && !in.core.animal;
    const bool stressRoute = in.route == Route_Cage || in.route == Route_FieldYield;

    // Stress: only where the roll reads it, and the bar IS the stress the roll was
    // made with when there was one.
    if (person && stressRoute && !in.mercyPending)
    {
        v.showStress = true;
        const float stress = in.verdict.rolled
                                 ? in.verdict.chanceInputs.stress
                                 : StressFromState(in.core.hunger, in.core.worstDamage,
                                                   in.core.restedState, config.submission);
        v.stressPct = Percent(stress);
        SplitStress(in, config.submission, v.stressPct, &v);
    }

    if (in.verdict.rolled)
    {
        v.showChance = true;
        v.chancePct = in.verdict.chancePercent;
    }

    if (person && config.submission.countDefeats &&
        (stressRoute || in.core.defeats > 0))
    {
        v.showDefeats = true;
        v.defeats = in.core.defeats;
        v.defeatsNeeded = config.submission.defeatsToBreak;
    }

    if (in.haveCore && in.core.animal && in.verdict.tameNeeded >= 0.0f)
    {
        v.showFood = true;
        v.foodPct = Percent(in.verdict.tameFullness);
        v.foodNeededPct = Percent(in.verdict.tameNeeded);
    }

    v.primary = PrimaryLine(in);
    v.primary.food = v.foodPct;
    v.primary.need = v.foodNeededPct;

    // The easy wait is a way in whatever the price table says, so while it still
    // runs it is worth a line. Two reasons name no number of their own (the proud
    // and the zealot); for them the wait is the most useful thing to say.
    const float easy = config.prices.easyHoursCaged;
    const bool waitRuns = in.route == Route_Cage && person && config.captivityEnabled &&
                          easy > 0.0f && in.holdKnown && in.hoursHeld < easy &&
                          !in.verdict.willJoin && !in.mercyPending;
    if (waitRuns)
    {
        ReadoutLineView wait;
        wait.id = kLineWaitCage;
        wait.hours = easy - in.hoursHeld;
        if (in.verdict.reasonKey == Reason_Defiant ||
            in.verdict.reasonKey == Reason_UniqueZealot)
        {
            v.primary = wait;
        }
        else
        {
            v.secondary = wait;
        }
    }
    return v;
}
} // namespace totalrecruit
