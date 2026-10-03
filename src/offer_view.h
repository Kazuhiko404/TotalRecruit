#pragma once

// What the recruit popup shows, without the engine.
//
// Every Hablar outcome opens the same window (offer_popup.cpp): a yes with its
// price, a price the purse cannot meet, a refusal and why, a refusal the player
// can act on, a plea for mercy, a rescue. This decides what goes in each slot of
// that window -- the words, the numbers, which buttons exist -- from the verdict
// the click already reached and the player's purse. The window only draws it.
//
// The rules (ui-mockup/readout.html, section 5):
//  - A cats price is shown with the purse it is checked against. A purse below
//    the price makes the purse red, adds the shortfall as the red line and hides
//    ACEPTAR: there is nothing to accept.
//  - Any refusal shows its reason in red and hides ACEPTAR. A remedy the player
//    can deliver (food, a beating) adds its button.
//  - A free join says so; the odds appear only when the verdict rolled.
//
// The words come in as OfferTexts (messages.json "popup" block, resolved by
// npc_speech.cpp), so this stays testable and a translation changes no rule.

#include <string>

#include "readout_model.h"
#include "verdict.h"

namespace totalrecruit
{
// What the third button does, when the plugin can actually deliver it. Chosen by
// the caller, which knows whether the player carries food or can beat them.
enum OfferRemedy
{
    OfferRemedy_None,
    OfferRemedy_Feed,
    // The popup reopened after a meal and they still want more.
    OfferRemedy_FeedMore,
    OfferRemedy_Beat
};

// Every word the popup can show. Templates take {name} and {cats}.
struct OfferTexts
{
    std::string title;

    std::string accept;
    std::string cancel;
    std::string close;
    std::string notNow;
    std::string feed;
    std::string feedMore;
    std::string beat;
    std::string ignore;
    std::string decline;

    std::string routeFree;
    std::string routeCaptivity;
    std::string routeRescue;
    std::string routeSurrender;
    std::string routeFieldYield;
    std::string routeMercy;
    std::string routeAnimal;

    std::string priceLabel;
    std::string purseLabel;
    std::string chanceLabel;
    std::string free;
    std::string catsValue;     // "{cats} cats"
    std::string shortPurse;    // "No te alcanzan los cats (te faltan {cats})"

    std::string bodyRefused;   // "{name} no se une todavia."
    std::string bodyCats;      // "{name} se unira a tu grupo por {cats} cats."
    std::string bodyFreedom;
    std::string bodySubmission;
    std::string bodyCare;
    std::string bodyRescue;
    std::string bodyJoins;
};

// The built-in Spanish, used for any key messages.json does not have.
OfferTexts DefaultOfferTexts();

struct OfferInput
{
    std::string name;
    Verdict verdict;
    // The player's purse, when it could be read.
    bool purseKnown;
    int purse;
    OfferRemedy remedy;
    // Reopened after a remedy: Cancel reads "not now" instead.
    bool afterRemedy;

    OfferInput() : name(), verdict(), purseKnown(false), purse(0),
                   remedy(OfferRemedy_None), afterRemedy(false)
    {
    }
};

struct OfferView
{
    std::string title;
    std::string name;
    ReadoutRoute route;
    std::string routeTag;
    std::string bodyLine;

    // The info box. Each row only when it applies.
    std::string priceLabel;
    std::string purseLabel;
    std::string chanceLabel;

    bool showPrice;
    // Free join: the price row says so, in green.
    bool priceFree;
    int priceCats;
    std::string priceText;

    bool showPurse;
    bool purseKnown;
    int purseCats;
    std::string purseText;
    // False only when the purse was read and holds less than the price.
    bool affordable;
    int shortfall;

    bool showChance;
    int chancePercent;
    std::string chanceText;

    // Empty when there is nothing to explain. Red when it is a no.
    std::string reasonText;
    bool reasonRefused;

    // Empty = the button is hidden. Cancel is always there.
    std::string acceptLabel;
    std::string extraLabel;
    std::string cancelLabel;

    OfferView()
        : route(ReadoutRoute_Free), showPrice(false), priceFree(false), priceCats(0),
          showPurse(false), purseKnown(false), purseCats(0), affordable(true), shortfall(0),
          showChance(false), chancePercent(0), reasonRefused(false)
    {
    }
};

// The route tag the popup and the card agree on, from the verdict alone.
ReadoutRoute OfferRouteOf(const Verdict& verdict);

OfferView BuildOfferView(const OfferInput& in, const OfferTexts& texts);

// A crippled enemy begging (mercy.cpp): the plea is the body, the answers are
// take them / let them go (only when `canIgnore`) / refuse.
OfferView BuildMercyOfferView(const std::string& name, const std::string& pleaLine,
                              bool canIgnore, const OfferTexts& texts);
} // namespace totalrecruit
