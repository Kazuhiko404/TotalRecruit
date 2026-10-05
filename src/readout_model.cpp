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

// The four parts of the bar, in whole percentage points that add up to the bar.
// Each term is scaled the same way the clamp scales the sum (when a config makes
// the terms add past 1.0 the bar still tops out at 100), rounded to the nearest
// point, and what rounding gained or lost is settled on the largest part (the
// first of equals, in hunger, wounds, tired, beaten order).
void SplitStress(const ReadoutInput& in, const Submission& sub, int pct, ReadoutView* out)
{
    const StressTerms t = StressTermsFromState(in.core.hunger, in.core.worstDamage,
                                               in.core.restedState, in.core.beatingStress, sub);
    const float terms[4] = { t.hunger, t.wounds, t.tired, t.beaten };
    const float sum = terms[0] + terms[1] + terms[2] + terms[3];
    if (sum <= 0.0f || pct <= 0)
    {
        out->hungerPts = out->woundsPts = out->tiredPts = out->beatenPts = 0;
        return;
    }
    const float k = (float)pct / (sum * 100.0f);
    int parts[4] = { 0, 0, 0, 0 };
    int total = 0;
    int biggest = 0;
    for (int i = 0; i < 4; ++i)
    {
        parts[i] = (int)(terms[i] * 100.0f * k + 0.5f);
        total += parts[i];
        if (terms[i] > terms[biggest]) biggest = i;
    }
    parts[biggest] += pct - total;
    if (parts[biggest] < 0) parts[biggest] = 0;
    out->hungerPts = parts[0];
    out->woundsPts = parts[1];
    out->tiredPts = parts[2];
    out->beatenPts = parts[3];
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
    return key == Reason_FieldYieldNotYet || key == Reason_AllyMaybe ||
           key == Reason_AnimalYieldNotYet;
}

// A free animal in the open can be beaten into yielding (animal-duel-submission D3),
// so its defeats count like a person's there; someone else's animal, or one in the
// player's cage, never yields in the field and shows none.
bool AnimalDefeatsMatter(const ReadoutInput& in, const Config& config)
{
    return in.haveCore && in.core.animal && !in.core.animalOwned && !in.core.inMyCage &&
           in.route != Route_Cage && config.surrenderEnabled &&
           (in.route == Route_FieldYield || in.core.defeats > 0);
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

// "Duelos: 2-1": wins in `food`, losses in `need`, draws (when any) in `chance`. With
// the next win's odds (duel_join_rules.h) the same line says what a win does: a sure
// join keeps the draws in `chance`; a percent takes `chance` and the draws move to
// `purse` (readout_catalog.h).
ReadoutLineView DuelRecordLine(const ReadoutInput& in)
{
    ReadoutLineView line;
    line.food = in.duelWins > 0 ? in.duelWins : 0;
    line.need = in.duelLosses > 0 ? in.duelLosses : 0;
    const int draws = in.duelDraws > 0 ? in.duelDraws : 0;
    line.chance = draws;
    if (!in.duelJoinKnown)
    {
        line.id = draws > 0 ? kLineDuelRecordWithDraws : kLineDuelRecord;
        return line;
    }
    if (in.duelJoinSure)
    {
        line.id = draws > 0 ? kLineDuelRecordDrawsSure : kLineDuelRecordSure;
        return line;
    }
    line.purse = draws;
    line.chance = in.duelJoinChance > 0 ? in.duelJoinChance : 0;
    line.id = draws > 0 ? kLineDuelRecordDrawsChance : kLineDuelRecordChance;
    return line;
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
                                                   in.core.restedState, in.core.beatingStress,
                                                   config.submission);
        v.stressPct = Percent(stress);
        SplitStress(in, config.submission, v.stressPct, &v);
    }

    if (in.verdict.rolled)
    {
        v.showChance = true;
        v.chancePct = in.verdict.chancePercent;
        // The Balanced cage's cared-for bonus already inside that chance
        // (cared_for_rules.h): "Lo cuidaste +12%" under the bar.
        v.caredForPct = in.verdict.chanceInputs.caredFor > 0 ? in.verdict.chanceInputs.caredFor : 0;
    }

    const bool personDefeats = person && (stressRoute || in.core.defeats > 0);
    if (config.submission.countDefeats && (personDefeats || AnimalDefeatsMatter(in, config)))
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

    // A challengeable ally (duel_challenge_rules.h): the head-to-head duel record,
    // when the second line is still free.
    if (in.duelRecordKnown && !v.secondary.Present())
    {
        v.secondary = DuelRecordLine(in);
    }
    return v;
}
} // namespace totalrecruit
