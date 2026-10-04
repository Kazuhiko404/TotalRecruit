#include "offer_view.h"
#include "duel_challenge_rules.h"
#include "readout_catalog.h"
#include "readout_format.h"

#include <cstdio>

namespace totalrecruit
{
namespace
{
void ReplaceAll(std::string* text, const char* key, const std::string& value)
{
    const std::string needle(key);
    size_t at = 0;
    while ((at = text->find(needle, at)) != std::string::npos)
    {
        text->replace(at, needle.size(), value);
        at += value.size();
    }
}

// {name} here, {cats} with the dot between thousands (readout_format).
std::string Fill(const std::string& text, const std::string& name, int cats)
{
    std::string out = text;
    ReplaceAll(&out, "{name}", name);
    ReplaceAll(&out, "{cats}", FormatCats(cats));
    return out;
}

// The verdict's reason is written to follow a name ("pide 9570 cats ..."); on its
// own line it starts with a capital. ASCII only: an accented first letter stays.
std::string Sentence(const std::string& text)
{
    std::string out = text;
    if (!out.empty() && out[0] >= 'a' && out[0] <= 'z')
    {
        out[0] = (char)(out[0] - 'a' + 'A');
    }
    return out;
}

std::string Percent(int value)
{
    char buffer[16] = { 0 };
    _snprintf_s(buffer, sizeof(buffer), _TRUNCATE, "%d%%", value);
    return std::string(buffer);
}

std::string PercentNumber(int value)
{
    char buffer[16] = { 0 };
    _snprintf_s(buffer, sizeof(buffer), _TRUNCATE, "%d", value);
    return std::string(buffer);
}

std::string RouteTag(ReadoutRoute route, const OfferTexts& t)
{
    switch (route)
    {
    case ReadoutRoute_Captivity: return t.routeCaptivity;
    case ReadoutRoute_Rescue:    return t.routeRescue;
    case ReadoutRoute_Surrender: return t.routeSurrender;
    case ReadoutRoute_Mercy:     return t.routeMercy;
    case ReadoutRoute_Animal:    return t.routeAnimal;
    // OfferTexts carries no slot for it yet (offer_view.h / npc_speech.cpp were
    // outside T5's edit surface): the shared label table's text, which is also
    // what messages.json ships for readout.route.fieldYield.
    case ReadoutRoute_FieldYield: return t.routeFieldYield;
    case ReadoutRoute_Free:
    default:                     return t.routeFree;
    }
}

// The narrator's line for a yes, by what they want (was npc_speech DescribeOffer).
std::string JoinBody(const Verdict& v, const std::string& name, const OfferTexts& t)
{
    switch (v.currency)
    {
    case Currency_Cats:       return Fill(t.bodyCats, name, v.cats);
    case Currency_Freedom:    return Fill(t.bodyFreedom, name, 0);
    case Currency_Submission: return Fill(t.bodySubmission, name, 0);
    case Currency_Care:       return Fill(t.bodyCare, name, 0);
    case Currency_None:
    default:
        return Fill(v.viaRescue ? t.bodyRescue : t.bodyJoins, name, 0);
    }
}

// A Balanced ally's template ({level}, {pct}, {days} on top of {name} and {cats}).
std::string AllyFill(const std::string& text, const std::string& name, const Verdict& v)
{
    std::string out = Fill(text, name, v.cats);
    ReplaceAll(&out, "{level}", PercentNumber(v.allyLevel));
    ReplaceAll(&out, "{pct}", PercentNumber(v.allyDiscountPercent));
    ReplaceAll(&out, "{days}", PercentNumber(v.temporaryDays));
    return out;
}

// The body line a Balanced ally's tier gives (ally_tier_rules.h): the friend's
// price, the free friend, the temporary member. Empty when the tier has nothing
// of its own to say (not tiered, or a refusal, which reads like any other).
std::string AllyBody(const Verdict& v, const std::string& name, const OfferTexts& t)
{
    if (!v.allyTiered)
    {
        return std::string();
    }
    if (v.currency == Currency_Cats)
    {
        return AllyFill(v.allyDiscountPercent > 0 ? t.allyPaidBody : t.allyPaidFullBody, name, v);
    }
    if (v.willJoin && v.temporary)
    {
        return AllyFill(t.allyTemporaryBody, name, v);
    }
    if (v.willJoin)
    {
        return AllyFill(t.allyFreeBody, name, v);
    }
    return std::string();
}

// "-14%" beside a tiered ally's price; nothing for any other price.
std::string PriceTag(const Verdict& v, const OfferTexts& t)
{
    if (!v.allyTiered || v.allyDiscountPercent <= 0)
    {
        return std::string();
    }
    std::string tag = t.discountTag;
    ReplaceAll(&tag, "{pct}", PercentNumber(v.allyDiscountPercent));
    return tag;
}

// A cats price the player is being asked for: a yes with a price, or the "not
// yet" that is only the purse.
bool IsPriceOffer(const Verdict& v)
{
    return v.currency == Currency_Cats && v.cats > 0 &&
           (v.willJoin || v.reasonKey == Reason_CatsAsk);
}

std::string RemedyLabel(OfferRemedy remedy, const OfferTexts& t)
{
    switch (remedy)
    {
    case OfferRemedy_Feed:     return t.feed;
    case OfferRemedy_FeedMore: return t.feedMore;
    case OfferRemedy_None:
    default:                   return std::string();
    }
}

// The haggle step of BuildOfferView (ui-mockup/js/haggle.js applyHaggle): the
// slider, the line under it and the amber button, for a cats price. False when
// the slider has no range (a floor at the ask), which leaves the plain window.
bool ApplyHaggle(const OfferInput& in, const OfferTexts& t, OfferView* view)
{
    const int ask = in.verdict.cats;
    const HaggleBounds bounds = HaggleSliderBounds(ask, in.haggle.prices);
    if (bounds.minOffer >= bounds.maxOffer)
    {
        return false;
    }
    const int offer = in.haggle.offer > 0 ? ClampOffer(in.haggle.offer, bounds)
                                          : bounds.maxOffer;

    view->showSlider = true;
    view->haggleBounds = bounds;
    view->sliderSteps = HaggleStepCount(bounds);
    view->sliderStep = StepOfOffer(bounds, offer);
    view->offerCats = offer;
    view->haggleLabel = t.haggleLabel;
    view->offerText = Fill(t.catsValue, in.name, offer);
    view->minText = FormatCats(bounds.minOffer);
    view->maxText = FormatCats(bounds.maxOffer);
    view->oddsPercent = AcceptOdds(ask, offer, in.haggle.leverage, in.haggle.personality,
                                   in.haggle.prices);

    if (view->purseKnown && view->purseCats < offer)
    {
        view->affordable = false;
        view->shortfall = offer - view->purseCats;
        view->haggleLine = Fill(t.haggleShort, in.name, view->shortfall);
        view->haggleTone = OddsTone_Danger;
        return true;
    }

    std::string odds = t.haggleOdds;
    ReplaceAll(&odds, "{pct}", PercentNumber(view->oddsPercent));
    view->haggleLine = odds;
    view->haggleTone = OddsToneOf(view->oddsPercent);
    // At the ask it is the plain ACEPTAR (CompleteOffer re-evaluates, so a "not
    // yet" the purse now covers joins); below it, one haggle attempt.
    view->acceptIsOffer = offer < ask;
    view->acceptLabel = view->acceptIsOffer ? t.haggleOffer : t.accept;
    return true;
}

// The cage commands step of BuildOfferView (ui-mockup/js/popup_data.js, the caged
// states): a refusal from a prisoner the context menu would offer Golpear and
// Noquear for gets them in the popup too -- the same predicate, so the two never
// disagree. A yes needs no beating; a haggle's row belongs to the slider.
// Without them, a defiant prisoner is not told to hurt someone they cannot.
void ApplyCageCommands(const OfferInput& in, const OfferTexts& t, OfferView* view)
{
    const bool offered = !in.verdict.willJoin && !view->showSlider &&
                         CageCommandsAllowed(in.cage);
    if (offered)
    {
        view->beatLabel = t.beat;
        view->knockoutLabel = KnockoutLabel(t.knockout, in.knockoutOddsKnown, in.knockoutOdds);
        return;
    }
    if (in.verdict.reasonKey == Reason_Defiant && view->reasonRefused)
    {
        view->reasonText = t.reasonDefiantNoBeat;
    }
}

// The duel challenge step of BuildOfferView (ui-mockup section 5, the duel state):
// the caller decided it with CanOfferDuel; it adds a button and, when TotalSparring
// read it, the head-to-head record. It never takes the tier's own answer away.
void ApplyDuelChallenge(const OfferInput& in, const OfferTexts& t, OfferView* view)
{
    if (!in.duel.offered)
    {
        return;
    }
    view->duelLabel = t.duelChallenge;
    if (in.duel.recordKnown)
    {
        view->showDuelRecord = true;
        view->duelRecordLabel = t.duelRecordLabel;
        view->duelRecordText = FormatDuelRecord(in.duel.wins, in.duel.losses, in.duel.draws);
    }
}

// The answers that let someone go (IGNORAR on a plea, PERDONAR on a submission)
// mean nothing to someone who cannot leave: a cage, shackles, a shoulder.
std::string LetGoLabel(bool inCage, const std::string& label)
{
    return inCage ? std::string() : label;
}

OfferView Frame(const std::string& name, ReadoutRoute route, const OfferTexts& t)
{
    OfferView view;
    view.title = t.title;
    view.name = name;
    view.route = route;
    view.routeTag = RouteTag(route, t);
    view.priceLabel = t.priceLabel;
    view.purseLabel = t.purseLabel;
    view.chanceLabel = t.chanceLabel;
    return view;
}

// What the verdict alone puts in the window: price, purse, haggle, reason and
// the accept / remedy / cancel buttons.
OfferView BuildVerdictView(const OfferInput& in, const OfferTexts& t);
} // namespace

OfferTexts DefaultOfferTexts()
{
    OfferTexts t;
    t.title = "Reclutamiento";

    t.accept = "ACEPTAR";
    t.cancel = "CANCELAR";
    t.close = "CERRAR";
    t.notNow = "AHORA NO";
    t.feed = "DARLE COMIDA";
    t.feedMore = "DARLE M\xc3\x81S COMIDA";
    t.beat = "GOLPEAR";
    t.knockout = "NOQUEAR";
    t.ignore = "IGNORAR";
    t.decline = "RECHAZAR";
    t.forgive = "PERDONAR";
    t.routeHelper = "Ayuda";

    t.routeFree = "Libre";
    t.routeCaptivity = "Cautiverio";
    t.routeRescue = "Rescate";
    t.routeSurrender = "Rendicion";
    t.routeFieldYield = ReadoutRouteDefaultText(ReadoutRoute_FieldYield);
    t.routeMercy = "Piedad";
    t.routeAnimal = "Animal";

    t.priceLabel = "PRECIO";
    t.purseLabel = "TIENES";
    t.chanceLabel = "PROBABILIDAD";
    t.free = "Gratis";
    t.catsValue = "{cats} cats";
    t.shortPurse = "No te alcanzan los cats (te faltan {cats})";

    t.bodyRefused = "{name} no se une todav\xc3\xad" "a.";
    t.bodyCats = "{name} se unir\xc3\xa1 a tu grupo por {cats} cats.";
    t.bodyFreedom = "{name} se unir\xc3\xa1 a tu grupo si lo liberas.";
    t.bodySubmission = "{name} ya no tiene voluntad de pelear y desea unirse a tu grupo.";
    t.bodyCare = "{name} te debe la comida y las curas, y desea unirse a tu grupo.";
    t.bodyRescue = "{name} te debe la vida y desea unirse a tu grupo.";
    t.bodyJoins = "{name} desea unirse a tu grupo.";
    t.bodySubmitted = "{name} se arrodilla y te pide unirse a tu grupo.";
    t.bodyHelper = "{name} te ofrece acompa\xc3\xb1" "arte {days} d\xc3\xad" "as, gratis. No podr\xc3\xa1s darle \xc3\xb3rdenes.";
    t.reasonDefiantNoBeat = "Sigue desafiante: d\xc3\xa9jalo con hambre";

    t.allyFreeBody = "{name} es tu aliado (nivel {level}) y se une gratis por amistad.";
    t.allyPaidBody = "Aliado (nivel {level}): {name} te cobra {cats} cats, con {pct}% de descuento por amistad.";
    t.allyPaidFullBody = "Aliado (nivel {level}): {name} te cobra {cats} cats.";
    t.allyTemporaryBody = "Aliado (nivel {level}): {name} te acompa\xc3\xb1" "ar\xc3\xa1 {days} d\xc3\xad" "as como miembro temporal.";
    t.discountTag = "-{pct}%";

    t.duelChallenge = "RETAR A DUELO";
    t.duelRecordLabel = "DUELOS";

    t.haggleLabel = "OFERTA";
    t.haggleOdds = "Probabilidad de aceptar: {pct}%";
    t.haggleOffer = "OFRECER";
    t.haggleOffended = "Se ofendio: hoy no regatea";
    t.haggleShort = "No te alcanzan para esa oferta (te faltan {cats})";
    return t;
}

OddsTone OddsToneOf(int percent)
{
    if (percent >= 70) return OddsTone_Ready;
    if (percent >= 30) return OddsTone_Amber;
    return OddsTone_Danger;
}

ReadoutRoute OfferRouteOf(const Verdict& verdict)
{
    if (verdict.isAnimal) return ReadoutRoute_Animal;
    if (verdict.viaRescue) return ReadoutRoute_Rescue;
    if (verdict.viaCaptivity) return ReadoutRoute_Captivity;
    switch (verdict.reasonKey)
    {
    case Reason_FieldYieldJoins:
    case Reason_FieldYieldNotYet:
        return ReadoutRoute_FieldYield;
    case Reason_Surrendered:
        return ReadoutRoute_Surrender;
    default:
        return ReadoutRoute_Free;
    }
}

OfferView BuildOfferView(const OfferInput& in, const OfferTexts& t)
{
    OfferView view = BuildVerdictView(in, t);
    ApplyCageCommands(in, t, &view);
    ApplyDuelChallenge(in, t, &view);
    return view;
}

namespace
{
OfferView BuildVerdictView(const OfferInput& in, const OfferTexts& t)
{
    const Verdict& v = in.verdict;
    OfferView view = Frame(in.name, OfferRouteOf(v), t);
    view.cancelLabel = in.afterRemedy ? t.notNow : t.cancel;

    if (v.rolled)
    {
        view.showChance = true;
        view.chancePercent = v.chancePercent;
        view.chanceText = Percent(v.chancePercent);
    }

    if (IsPriceOffer(v))
    {
        view.showPrice = true;
        view.priceCats = v.cats;
        view.priceText = Fill(t.catsValue, in.name, v.cats);
        view.priceTag = PriceTag(v, t);
        const std::string allyBody = AllyBody(v, in.name, t);
        view.bodyLine = allyBody.empty() ? Fill(t.bodyCats, in.name, v.cats) : allyBody;

        // The caller's fresh read wins; the verdict carries the purse it was
        // short against when the caller could not read one.
        if (in.purseKnown)
        {
            view.purseKnown = true;
            view.purseCats = in.purse;
        }
        else if (v.reasonKey == Reason_CatsAsk)
        {
            view.purseKnown = true;
            view.purseCats = v.purse;
        }
        if (view.purseKnown)
        {
            view.showPurse = true;
            view.purseText = Fill(t.catsValue, in.name, view.purseCats);
        }

        // Haggling: the slider takes over the purse check and the buttons.
        if (in.haggle.enabled)
        {
            if (in.haggle.locked)
            {
                view.haggleLocked = true;
                view.haggleLine = t.haggleOffended;
                view.haggleTone = OddsTone_Danger;
            }
            else if (ApplyHaggle(in, t, &view))
            {
                return view;
            }
        }

        if (view.purseKnown)
        {
            if (view.purseCats < v.cats)
            {
                view.affordable = false;
                view.shortfall = v.cats - view.purseCats;
                view.reasonText = Fill(t.shortPurse, in.name, view.shortfall);
                view.reasonRefused = true;
                return view;
            }
        }

        if (v.willJoin)
        {
            view.acceptLabel = t.accept;
            return view;
        }
        // A price "not yet" the purse covers by now: still not a yes until the
        // click re-evaluates, so it reads as a refusal.
        view.reasonText = Sentence(v.reason);
        view.reasonRefused = true;
        view.cancelLabel = in.afterRemedy ? t.notNow : t.close;
        return view;
    }

    if (v.willJoin)
    {
        view.showPrice = true;
        view.priceFree = true;
        view.priceText = t.free;
        const std::string allyBody = AllyBody(v, in.name, t);
        view.bodyLine = allyBody.empty() ? JoinBody(v, in.name, t) : allyBody;
        view.acceptLabel = t.accept;
        return view;
    }

    // A no. The rescued NPC paying their debt is a no that is not bad news.
    view.bodyLine = Fill(t.bodyRefused, in.name, 0);
    view.reasonText = Sentence(v.reason);
    view.reasonRefused = !v.owesGift;
    view.extraLabel = RemedyLabel(in.remedy, t);
    view.cancelLabel = in.afterRemedy ? t.notNow : t.close;
    return view;
}
} // namespace

OfferView BuildMercyOfferView(const std::string& name, const std::string& pleaLine,
                              bool canIgnore, bool inCage, const OfferTexts& t)
{
    OfferView view = Frame(name, ReadoutRoute_Mercy, t);
    view.bodyLine = pleaLine;
    view.acceptLabel = t.accept;
    view.extraLabel = canIgnore ? LetGoLabel(inCage, t.ignore) : std::string();
    view.cancelLabel = t.decline;
    return view;
}

OfferView BuildHelperOfferView(const std::string& name, int days, const OfferTexts& t)
{
    OfferView view = Frame(name, ReadoutRoute_Free, t);
    view.routeTag = t.routeHelper;
    std::string body = Fill(t.bodyHelper, name, 0);
    ReplaceAll(&body, "{days}", PercentNumber(days));
    view.bodyLine = body;
    view.showPrice = true;
    view.priceFree = true;
    view.priceText = t.free;
    view.acceptLabel = t.accept;
    view.cancelLabel = t.decline;
    return view;
}

OfferView BuildSubmissionOfferView(const std::string& name, bool inCage, const OfferTexts& t)
{
    OfferView view = Frame(name, ReadoutRoute_FieldYield, t);
    view.bodyLine = Fill(t.bodySubmitted, name, 0);
    view.showPrice = true;
    view.priceFree = true;
    view.priceText = t.free;
    view.acceptLabel = t.accept;
    view.extraLabel = LetGoLabel(inCage, t.forgive);
    view.cancelLabel = t.decline;
    return view;
}
} // namespace totalrecruit
