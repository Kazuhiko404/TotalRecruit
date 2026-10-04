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
//  - Any refusal shows its reason in red and hides ACEPTAR. Food the player can
//    deliver adds its button.
//  - A refusal from a prisoner the cage commands apply to (cage_command_rules.h
//    CageCommandsAllowed, the same predicate as the context-menu block) adds
//    GOLPEAR and NOQUEAR (with the engine's odds), whatever the route said. A yes
//    never carries them, and neither does a haggle (the slider owns that row).
//    Without them a defiant prisoner's reason drops "hurt them".
//  - Someone who cannot leave (in a cage, shackled, carried) is never offered the
//    answers that let them go: no IGNORAR on a plea, no PERDONAR on a submission.
//  - A free join says so; the odds appear only when the verdict rolled.
//  - A cats price can be haggled (OfferHaggleInput): a slider from the floor to
//    the ask, the live odds of AcceptOdds, the purse checked against the offer,
//    OFRECER below the ask and ACEPTAR at it. Offended today: the lock line and
//    no slider. Never for a free route.
//  - A Balanced ally whose level tier decided (Verdict::allyTiered) says so in the
//    body: the friend's price with its discount ("Aliado (nivel 34): ... con 14%
//    de descuento por amistad", PRECIO tagged "-14%"), the free friend, or the
//    temporary member and their days. A tier refusal reads like any refusal.
//  - An ally the duel is offered for (OfferDuelInput, decided by the caller with
//    duel_challenge_rules.h CanOfferDuel) keeps all of the above and adds RETAR A
//    DUELO to the row, and the head-to-head record as an info row ("DUELOS 2-1")
//    when TotalSparring could read it.
//
// The words come in as OfferTexts (messages.json "popup" block, resolved by
// npc_speech.cpp), so this stays testable and a translation changes no rule.

#include <string>

#include "cage_command_rules.h"
#include "haggle_rules.h"
#include "readout_model.h"
#include "verdict.h"

namespace totalrecruit
{
// What the remedy button does, when the plugin can actually deliver it. Chosen by
// the caller, which knows whether the player carries food. The beating is not a
// remedy: it is the cage commands (OfferInput::cage), offered by their own rule.
enum OfferRemedy
{
    OfferRemedy_None,
    OfferRemedy_Feed,
    // The popup reopened after a meal and they still want more.
    OfferRemedy_FeedMore
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
    // The cage commands as popup buttons: "GOLPEAR" (+ " (1 h)" while it rests,
    // RestLabel), "NOQUEAR" (+ " 72%", or " (1 h)" while it rests: KnockoutCaption).
    std::string beat;
    std::string knockout;
    std::string ignore;
    std::string decline;
    // The submission event's third answer: let them go (submission_director.h).
    std::string forgive;
    // The free helper's route tag (helper.h): "Ayuda".
    std::string routeHelper;

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
    std::string caredForLabel;   // "LO CUIDASTE", the cared-for bonus row
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
    // The submission event: "{name} se arrodilla y te pide unirse a tu grupo."
    std::string bodySubmitted;
    // The free helper: "{name} te ofrece acompanarte {days} dias, gratis. No podras
    // darle ordenes."
    std::string bodyHelper;

    // Balanced allies by level (ally_tier_rules.h, popup.ally.*). Templates also
    // take {level} (the NPC's), {pct} (the friendship discount) and {days}.
    std::string allyFreeBody;       // "{name} es tu aliado (nivel {level}) y se une gratis por amistad."
    std::string allyPaidBody;       // "Aliado (nivel {level}): {name} te cobra {cats} cats, con {pct}% de descuento por amistad."
    std::string allyPaidFullBody;   // the same with no discount to speak of
    std::string allyTemporaryBody;  // "Aliado (nivel {level}): {name} te acompanara {days} dias como miembro temporal."
    std::string discountTag;        // "-{pct}%", beside the price

    // Reason_Defiant when the cage commands are not offered: the half of "hunger
    // or hurt them" the player can still act on (popup.reason.defiantNoBeat).
    std::string reasonDefiantNoBeat;

    // The duel challenge (duel_challenge_rules.h, popup.duel.*).
    std::string duelChallenge;    // "RETAR A DUELO"
    std::string duelRecordLabel;  // "DUELOS", the info row of the head-to-head record
    std::string duelJoinLabel;    // "SI LE GANAS", what the next win does
    std::string duelJoinChance;   // "{chance}% de que se una" (Balanced)
    std::string duelJoinSure;     // "se une" (Cheat)

    // The haggle section (popup.haggle.*).
    std::string haggleLabel;    // "OFERTA"
    std::string haggleOdds;     // "Probabilidad de aceptar: {pct}%"
    std::string haggleOffer;    // "OFRECER", the amber button below the ask
    std::string haggleOffended; // "Se ofendio: hoy no regatea"
    std::string haggleShort;    // "No te alcanzan para esa oferta (te faltan {cats})"
};

// The colour of the line under the slider: ready from 70%, amber 30-69, danger
// under 30 (the shortfall and offended lines are danger too).
enum OddsTone
{
    OddsTone_Ready,
    OddsTone_Amber,
    OddsTone_Danger
};

OddsTone OddsToneOf(int percent);

// What a haggle needs beyond the verdict. Read by the caller (recruit_core.cpp,
// from character_facts ReadHaggleTerms); `enabled` false = take it or leave it.
struct OfferHaggleInput
{
    // haggleEnabled, and a roll key and a game clock were there to seed the draw.
    bool enabled;
    // Offended today (haggle_ledger.h): no slider, the lock line instead.
    bool locked;
    // HaggleLeverage of the talking player against the NPC.
    int leverage;
    Temperament personality;
    Prices prices;
    // The slider's offer in cats; 0 or less = start at the ask.
    int offer;

    OfferHaggleInput()
        : enabled(false), locked(false), leverage(0), personality(Temperament_Normal),
          prices(), offer(0)
    {
    }
};

// The duel challenge beside the tier's answer (duel_challenge_rules.h). Decided by
// the caller (recruit_core.cpp, CanOfferDuel); the record is TotalSparring's
// head-to-head of the selected squad member against this NPC, read when the popup
// opens.
struct OfferDuelInput
{
    // RETAR A DUELO is on offer.
    bool offered;
    // The record could be read (TotalSparring answered); only then is the row drawn.
    bool recordKnown;
    int wins;
    int losses;
    int draws;
    // What the next win does (duel_join_rules.h DuelNextWinOdds): known when the caller
    // could say (Balanced needs the record), sure in Cheat, else a percent.
    bool joinKnown;
    bool joinSure;
    int joinChance;

    OfferDuelInput()
        : offered(false), recordKnown(false), wins(0), losses(0), draws(0), joinKnown(false),
          joinSure(false), joinChance(0)
    {
    }
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
    OfferHaggleInput haggle;
    // What decides the cage commands, read the way the context menu reads it
    // (cage_commands.h ReadCageCommandFacts). The default (not in the player's
    // cage) offers none.
    CageCommandFacts cage;
    // The engine's knockout odds for the selected character, for NOQUEAR's label.
    bool knockoutOddsKnown;
    float knockoutOdds;
    // RETAR A DUELO and the record row; the default offers none.
    OfferDuelInput duel;

    OfferInput() : name(), verdict(), purseKnown(false), purse(0),
                   remedy(OfferRemedy_None), afterRemedy(false), haggle(), cage(),
                   knockoutOddsKnown(false), knockoutOdds(0.0f), duel()
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
    // The friendship discount already in the price ("-14%"), drawn beside it.
    // Empty when there is none.
    std::string priceTag;

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

    // The Balanced cage's cared-for bonus already inside that chance
    // (cared_for_rules.h): "LO CUIDASTE  +12%", only when it was drawn.
    bool showCaredFor;
    std::string caredForLabel;
    std::string caredForText;

    // The head-to-head duel record ("DUELOS  2-1"), only beside RETAR A DUELO.
    bool showDuelRecord;
    std::string duelRecordLabel;
    std::string duelRecordText;
    // What the next win does ("SI LE GANAS  65% de que se una"), only beside RETAR A DUELO.
    bool showDuelJoin;
    std::string duelJoinLabel;
    std::string duelJoinText;

    // Empty when there is nothing to explain. Red when it is a no.
    std::string reasonText;
    bool reasonRefused;

    // Empty = the button is hidden. Cancel is always there. In the row: accept,
    // extra (the remedy / IGNORAR / PERDONAR), duel, beat, knockout, cancel.
    std::string acceptLabel;
    std::string extraLabel;
    // RETAR A DUELO (duel_challenge_rules.h): next to the tier's own answer.
    std::string duelLabel;
    std::string beatLabel;
    std::string knockoutLabel;
    std::string cancelLabel;

    // The haggle section (cats prices only, ui-mockup section 5 states 2-4). With
    // the slider, `affordable` / `shortfall` are against the OFFER, and the
    // shortfall is the slider's line, never the reason plate: a drag must not
    // change the window's height.
    bool showSlider;
    HaggleBounds haggleBounds;
    // Slider positions 0..sliderSteps (haggle_rules.h OfferAtStep).
    int sliderSteps;
    int sliderStep;
    int offerCats;
    std::string haggleLabel;
    std::string offerText;
    std::string minText;
    std::string maxText;
    int oddsPercent;
    // The line under the slider -- the odds, the shortfall for the offer, or the
    // offended lock. Empty = no haggle section at all.
    std::string haggleLine;
    OddsTone haggleTone;
    bool haggleLocked;
    // The amber button is OFRECER (one haggle attempt), not ACEPTAR.
    bool acceptIsOffer;

    OfferView()
        : route(ReadoutRoute_Free), showPrice(false), priceFree(false), priceCats(0),
          showPurse(false), purseKnown(false), purseCats(0), affordable(true), shortfall(0),
          showChance(false), chancePercent(0), showCaredFor(false), showDuelRecord(false), showDuelJoin(false),
          reasonRefused(false),
          showSlider(false), haggleBounds(), sliderSteps(0), sliderStep(0), offerCats(0),
          oddsPercent(100), haggleTone(OddsTone_Ready), haggleLocked(false), acceptIsOffer(false)
    {
    }
};

// The route tag the popup and the card agree on, from the verdict alone.
ReadoutRoute OfferRouteOf(const Verdict& verdict);

OfferView BuildOfferView(const OfferInput& in, const OfferTexts& texts);

// A crippled enemy begging (mercy.cpp): the plea is the body, the answers are
// take them / let them go (only when `canIgnore` and they are not `inCage`) /
// refuse. `inCage` is engine_reads SafeInPrison: a cage, shackles, carried.
OfferView BuildMercyOfferView(const std::string& name, const std::string& pleaLine,
                              bool canIgnore, bool inCage, const OfferTexts& texts);

// A beaten NPC that walked up to ask (submission_director.cpp): the Sometido
// tag, the kneeling line as the body, a free join, and three answers --
// ACEPTAR (they join), PERDONAR (they go, calm for a while; never when `inCage`),
// RECHAZAR (defeats wiped, no recruiting for the cooldown, hostile again).
OfferView BuildSubmissionOfferView(const std::string& name, bool inCage,
                                   const OfferTexts& texts);

// An ally that walked up to offer free help (helper.cpp): the Ayuda tag, the offer
// with its `days` as the body, free, and two answers -- ACEPTAR (they escort the
// player for those days, taking no orders) and RECHAZAR (no offer again for the
// reject cooldown).
OfferView BuildHelperOfferView(const std::string& name, int days, const OfferTexts& texts);
} // namespace totalrecruit
