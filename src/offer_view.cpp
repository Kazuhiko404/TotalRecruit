#include "offer_view.h"
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
    case OfferRemedy_Beat:     return t.beat;
    case OfferRemedy_None:
    default:                   return std::string();
    }
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
    t.beat = "GOLPEARLO";
    t.ignore = "IGNORAR";
    t.decline = "RECHAZAR";

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
    return t;
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
        view.bodyLine = Fill(t.bodyCats, in.name, v.cats);

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
        view.bodyLine = JoinBody(v, in.name, t);
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

OfferView BuildMercyOfferView(const std::string& name, const std::string& pleaLine,
                              bool canIgnore, const OfferTexts& t)
{
    OfferView view = Frame(name, ReadoutRoute_Mercy, t);
    view.bodyLine = pleaLine;
    view.acceptLabel = t.accept;
    view.extraLabel = canIgnore ? t.ignore : std::string();
    view.cancelLabel = t.decline;
    return view;
}
} // namespace totalrecruit
