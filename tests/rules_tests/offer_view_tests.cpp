// Tests for src/offer_view.cpp: what the recruit popup shows for each outcome
// (ui-mockup/readout.html, section 5).

#include "test_support.h"
#include "offer_view.h"

using namespace totalrecruit;

namespace
{
OfferTexts Texts()
{
    return DefaultOfferTexts();
}

Verdict CatsVerdict(bool willJoin, int cats, int purse)
{
    Verdict v;
    v.willJoin = willJoin;
    v.currency = Currency_Cats;
    v.cats = cats;
    v.purse = purse;
    v.reasonKey = willJoin ? Reason_CatsJoin : Reason_CatsAsk;
    v.reason = willJoin ? "se une por 9570 cats" : "pide 9570 cats y tienes 3200";
    return v;
}

void JoinsForCatsShowsPriceAndPurse()
{
    const char* name = "joins for cats: price, purse, Accept and Cancel";
    OfferInput in;
    in.name = "Kareem";
    in.verdict = CatsVerdict(true, 9570, 0);
    in.verdict.viaCaptivity = true;
    in.verdict.rolled = true;
    in.verdict.chancePercent = 58;
    in.purseKnown = true;
    in.purse = 12000;
    const OfferTexts t = Texts();
    const OfferView v = BuildOfferView(in, t);

    CHECK(v.title == t.title);
    CHECK(v.name == "Kareem");
    CHECK(v.route == ReadoutRoute_Captivity);
    CHECK(v.routeTag == t.routeCaptivity);
    CHECK(v.showPrice && !v.priceFree);
    CHECK(v.priceCats == 9570);
    CHECK(v.priceText == "9.570 cats");
    CHECK(v.showPurse && v.purseKnown);
    CHECK(v.purseCats == 12000);
    CHECK(v.purseText == "12.000 cats");
    CHECK(v.affordable);
    CHECK(v.shortfall == 0);
    CHECK(v.showChance);
    CHECK(v.chanceText == "58%");
    CHECK(v.reasonText.empty());
    CHECK(v.acceptLabel == t.accept);
    CHECK(v.cancelLabel == t.cancel);
    CHECK(v.extraLabel.empty());
    CHECK(v.bodyLine.find("Kareem") == 0);
    CHECK(v.bodyLine.find("9.570") != std::string::npos);
}

void ShortPurseHidesAcceptAndSaysTheShortfall()
{
    const char* name = "can't afford: purse below price -> red shortfall, no Accept";
    OfferInput in;
    in.name = "Merc";
    in.verdict = CatsVerdict(false, 9570, 3200);
    in.purseKnown = true;
    in.purse = 3200;
    const OfferTexts t = Texts();
    const OfferView v = BuildOfferView(in, t);

    CHECK(v.showPrice);
    CHECK(v.priceCats == 9570);
    CHECK(v.showPurse);
    CHECK(v.purseCats == 3200);
    CHECK(!v.affordable);
    CHECK(v.shortfall == 6370);
    CHECK(v.reasonRefused);
    CHECK(v.reasonText == "No te alcanzan los cats (te faltan 6.370)");
    CHECK(v.acceptLabel.empty());
    CHECK(v.extraLabel.empty());
    CHECK(v.cancelLabel == t.cancel);
    // The NPC is willing for that price; the body says the price.
    CHECK(v.bodyLine.find("9.570") != std::string::npos);
}

void ShortPurseReadFromTheVerdictWhenNotSupplied()
{
    const char* name = "can't afford: the verdict's purse counts when the caller read none";
    OfferInput in;
    in.name = "Merc";
    in.verdict = CatsVerdict(false, 500, 120);
    const OfferView v = BuildOfferView(in, Texts());
    CHECK(v.showPurse && v.purseKnown);
    CHECK(v.purseCats == 120);
    CHECK(!v.affordable);
    CHECK(v.shortfall == 380);
    CHECK(v.acceptLabel.empty());
}

void PurseDroppedBelowAYesStillBlocksAccept()
{
    const char* name = "a yes whose price the purse no longer covers hides Accept";
    OfferInput in;
    in.name = "Kareem";
    in.verdict = CatsVerdict(true, 1000, 0);
    in.purseKnown = true;
    in.purse = 999;
    const OfferView v = BuildOfferView(in, Texts());
    CHECK(!v.affordable);
    CHECK(v.shortfall == 1);
    CHECK(v.acceptLabel.empty());
    CHECK(v.reasonRefused);
}

void UnknownPurseNeverBlocks()
{
    const char* name = "an unread purse is not a no: Accept stays, no purse row";
    OfferInput in;
    in.name = "Kareem";
    in.verdict = CatsVerdict(true, 1000, 0);
    const OfferView v = BuildOfferView(in, Texts());
    CHECK(v.showPrice);
    CHECK(!v.showPurse);
    CHECK(v.affordable);
    CHECK(!v.acceptLabel.empty());
}

void RefusalIsRedWithCloseOnly()
{
    const char* name = "loyalty refusal: red reason, Close alone, no info box";
    OfferInput in;
    in.name = "Paladin";
    in.verdict.reasonKey = Reason_MilitaryLoyalty;
    in.verdict.currency = Currency_Submission;
    in.verdict.reason = "sirve con lealtad a su faccion";
    in.purseKnown = true;
    in.purse = 50000;
    const OfferTexts t = Texts();
    const OfferView v = BuildOfferView(in, t);
    CHECK(v.route == ReadoutRoute_Free);
    CHECK(v.reasonRefused);
    CHECK(v.reasonText == "Sirve con lealtad a su faccion");
    CHECK(v.acceptLabel.empty());
    CHECK(v.extraLabel.empty());
    CHECK(v.cancelLabel == t.close);
    CHECK(!v.showPrice && !v.showPurse && !v.showChance);
    CHECK(v.bodyLine.find("Paladin") == 0);
}

void RefusalWithRemedyOffersIt()
{
    const char* name = "refusal with a remedy: the remedy button next to Close";
    OfferInput in;
    in.name = "Fremp";
    in.verdict.reasonKey = Reason_Starving;
    in.verdict.currency = Currency_Care;
    in.verdict.wantsFood = true;
    in.verdict.viaCaptivity = true;
    in.verdict.reason = "hambriento: dale de comer";
    in.remedy = OfferRemedy_Feed;
    const OfferTexts t = Texts();
    OfferView v = BuildOfferView(in, t);
    CHECK(v.route == ReadoutRoute_Captivity);
    CHECK(v.extraLabel == t.feed);
    CHECK(v.acceptLabel.empty());
    CHECK(v.cancelLabel == t.close);
    CHECK(v.reasonText == "Hambriento: dale de comer");
    // Not in the player's cage: the cage commands are not on offer.
    CHECK(v.beatLabel.empty() && v.knockoutLabel.empty());

    in.remedy = OfferRemedy_FeedMore;
    in.afterRemedy = true;
    v = BuildOfferView(in, t);
    CHECK(v.extraLabel == t.feedMore);
    CHECK(v.cancelLabel == t.notNow);
}

void RemedyNeverRidesOnAYes()
{
    const char* name = "a yes shows no remedy button; after a remedy Cancel is 'not now'";
    OfferInput in;
    in.name = "Fremp";
    in.verdict.willJoin = true;
    in.verdict.currency = Currency_Care;
    in.verdict.viaCaptivity = true;
    in.remedy = OfferRemedy_Feed;
    in.afterRemedy = true;
    const OfferTexts t = Texts();
    const OfferView v = BuildOfferView(in, t);
    CHECK(v.extraLabel.empty());
    CHECK(v.acceptLabel == t.accept);
    CHECK(v.cancelLabel == t.notNow);
}

void RescueIsFree()
{
    const char* name = "rescue: free, Accept and Cancel";
    OfferInput in;
    in.name = "Esclavo";
    in.verdict.willJoin = true;
    in.verdict.viaRescue = true;
    in.verdict.reasonKey = Reason_RescueJoins;
    const OfferTexts t = Texts();
    const OfferView v = BuildOfferView(in, t);
    CHECK(v.route == ReadoutRoute_Rescue);
    CHECK(v.showPrice && v.priceFree);
    CHECK(v.priceText == t.free);
    CHECK(!v.showPurse);
    CHECK(v.acceptLabel == t.accept);
    CHECK(v.cancelLabel == t.cancel);
    CHECK(v.reasonText.empty());
}

void FieldYieldJoinsFreeWithOdds()
{
    const char* name = "field-yield yes: Sometido (its own tag, not Rendicion), free, the rolled odds";
    OfferInput in;
    in.name = "Soldado";
    in.verdict.willJoin = true;
    in.verdict.currency = Currency_Submission;
    in.verdict.reasonKey = Reason_FieldYieldJoins;
    in.verdict.rolled = true;
    in.verdict.chancePercent = 41;
    const OfferView v = BuildOfferView(in, Texts());
    CHECK(v.route == ReadoutRoute_FieldYield);
    CHECK(v.routeTag == "Sometido");
    CHECK(v.priceFree);
    CHECK(v.showChance && v.chanceText == "41%");

    OfferInput surrendered = in;
    surrendered.verdict.reasonKey = Reason_Surrendered;
    CHECK(BuildOfferView(surrendered, Texts()).route == ReadoutRoute_Surrender);
}

void GiftIsNotRed()
{
    const char* name = "a rescued NPC paying you is a plain line, not a red one";
    OfferInput in;
    in.name = "Esclavo";
    in.verdict.viaRescue = true;
    in.verdict.owesGift = true;
    in.verdict.reasonKey = Reason_RescueRepays;
    in.verdict.reason = "no puede seguirte, pero te deja algo en pago";
    const OfferView v = BuildOfferView(in, Texts());
    CHECK(!v.reasonText.empty());
    CHECK(!v.reasonRefused);
    CHECK(v.acceptLabel.empty());
}

void AnimalStomachIsNotMoney()
{
    const char* name = "animal: verdict.cats is stomach, never a price";
    OfferInput in;
    in.name = "Garru";
    in.verdict.isAnimal = true;
    in.verdict.currency = Currency_Care;
    in.verdict.cats = 40;
    in.verdict.reason = "tiene hambre";
    const OfferView v = BuildOfferView(in, Texts());
    CHECK(v.route == ReadoutRoute_Animal);
    CHECK(!v.showPrice);
    CHECK(v.reasonRefused);
}

void MercyHasThreeAnswers()
{
    const char* name = "mercy plea: plea as body, Accept / Ignore / Decline";
    const OfferTexts t = Texts();
    OfferView v = BuildMercyOfferView("Bandido", "Bandido: Piedad.", true, false, t);
    CHECK(v.route == ReadoutRoute_Mercy);
    CHECK(v.routeTag == t.routeMercy);
    CHECK(v.bodyLine == "Bandido: Piedad.");
    CHECK(v.acceptLabel == t.accept);
    CHECK(v.extraLabel == t.ignore);
    CHECK(v.cancelLabel == t.decline);
    CHECK(!v.showPrice && v.reasonText.empty());

    v = BuildMercyOfferView("Bandido", "Bandido: Piedad.", false, false, t);
    CHECK(v.extraLabel.empty());
}

// User report 2026-10-03: "if he is in a cage, don't allow ignore or forgive,
// since he can't leave". A caged plea is take them or refuse; never IGNORAR.
void CagedPleaHasNoIgnore()
{
    const char* name = "mercy plea in a cage: Accept / Decline, no IGNORAR";
    const OfferTexts t = Texts();
    const OfferView v = BuildMercyOfferView("Bandido", "Bandido: Piedad.", true, true, t);
    CHECK(v.acceptLabel == t.accept);
    CHECK(v.extraLabel.empty());
    CHECK(v.cancelLabel == t.decline);
    CHECK(v.beatLabel.empty() && v.knockoutLabel.empty());
}

// The defiant Holy Warden of the report: Cautiverio, 87%, in the player's cage,
// awake, beating allowed. The popup gives the native cage commands.
OfferInput CagedDefiant()
{
    OfferInput in;
    in.name = "Holy Warden";
    in.verdict.viaCaptivity = true;
    in.verdict.currency = Currency_Submission;
    in.verdict.reasonKey = Reason_Defiant;
    in.verdict.reason = "sigue desafiante: d\xc3\xa9jalo con hambre o h\xc3\xa9relo";
    in.verdict.rolled = true;
    in.verdict.chancePercent = 87;
    in.cage.allowBeating = true;
    in.cage.inPlayerCage = true;
    in.cage.down = false;
    return in;
}

void CagedRefusalOffersTheCageCommands()
{
    const char* name = "caged refusal: GOLPEAR and NOQUEAR <odds> beside CERRAR";
    const OfferTexts t = Texts();
    OfferInput in = CagedDefiant();
    in.knockoutOddsKnown = true;
    in.knockoutOdds = 0.72f;
    OfferView v = BuildOfferView(in, t);
    CHECK(v.route == ReadoutRoute_Captivity);
    CHECK(v.reasonRefused);
    CHECK(v.reasonText == "Sigue desafiante: d\xc3\xa9jalo con hambre o h\xc3\xa9relo");
    CHECK(v.acceptLabel.empty());
    CHECK(v.extraLabel.empty());
    CHECK(v.beatLabel == t.beat);
    CHECK(v.knockoutLabel == t.knockout + " 72%");
    CHECK(v.cancelLabel == t.close);

    // No odds from the engine: the plain label, as in the context menu.
    in.knockoutOddsKnown = false;
    v = BuildOfferView(in, t);
    CHECK(v.knockoutLabel == t.knockout);
}

void CagedHungryOffersFoodAndTheCageCommands()
{
    const char* name = "caged and hungry with food: DARLE COMIDA + GOLPEAR + NOQUEAR + CERRAR";
    const OfferTexts t = Texts();
    OfferInput in = CagedDefiant();
    in.verdict.currency = Currency_Care;
    in.verdict.wantsFood = true;
    in.verdict.reasonKey = Reason_Starving;
    in.verdict.reason = "hambriento: dale de comer";
    in.remedy = OfferRemedy_Feed;
    const OfferView v = BuildOfferView(in, t);
    CHECK(v.extraLabel == t.feed);
    CHECK(v.beatLabel == t.beat);
    CHECK(v.knockoutLabel == t.knockout);
    CHECK(v.cancelLabel == t.close);
    CHECK(v.acceptLabel.empty());
}

void CageCommandsFollowTheContextMenuRule()
{
    const char* name = "caged refusal: cage commands only where the context menu has them";
    const OfferTexts t = Texts();

    OfferInput outside = CagedDefiant();
    outside.cage.inPlayerCage = false;
    OfferView v = BuildOfferView(outside, t);
    CHECK(v.beatLabel.empty() && v.knockoutLabel.empty());

    OfferInput down = CagedDefiant();
    down.cage.down = true;
    v = BuildOfferView(down, t);
    CHECK(v.beatLabel.empty() && v.knockoutLabel.empty());

    // allowBeating off: no commands, and the reason no longer says to hurt them.
    OfferInput gentle = CagedDefiant();
    gentle.cage.allowBeating = false;
    v = BuildOfferView(gentle, t);
    CHECK(v.beatLabel.empty() && v.knockoutLabel.empty());
    CHECK(v.reasonText == t.reasonDefiantNoBeat);
    CHECK(v.reasonText.find("h\xc3\xa9relo") == std::string::npos);
    CHECK(v.reasonRefused);
    CHECK(v.cancelLabel == t.close);
}

void CagedYesKeepsAcceptAndCancel()
{
    const char* name = "caged yes: ACEPTAR / CANCELAR only, no cage commands";
    const OfferTexts t = Texts();
    OfferInput in = CagedDefiant();
    in.verdict.willJoin = true;
    in.verdict.reasonKey = Reason_Intimidated;
    const OfferView v = BuildOfferView(in, t);
    CHECK(v.acceptLabel == t.accept);
    CHECK(v.cancelLabel == t.cancel);
    CHECK(v.extraLabel.empty());
    CHECK(v.beatLabel.empty() && v.knockoutLabel.empty());
}

void CageCommandsNeverRideTheSlider()
{
    const char* name = "caged cats haggle: the slider's row, no cage commands";
    const OfferTexts t = Texts();
    OfferInput in;
    in.name = "Kareem";
    in.verdict = CatsVerdict(false, 9570, 3200);
    in.verdict.viaCaptivity = true;
    in.purseKnown = true;
    in.purse = 12000;
    in.haggle.enabled = true;
    in.haggle.leverage = 8;
    in.cage.inPlayerCage = true;
    OfferView v = BuildOfferView(in, t);
    CHECK(v.showSlider);
    CHECK(v.beatLabel.empty() && v.knockoutLabel.empty());

    // Haggling off, the purse short: a plain refusal, so the commands are there.
    in.haggle.enabled = false;
    in.purse = 3200;
    v = BuildOfferView(in, t);
    CHECK(!v.showSlider);
    CHECK(v.acceptLabel.empty());
    CHECK(v.beatLabel == t.beat && v.knockoutLabel == t.knockout);
}

void DefaultsAreComplete()
{
    const char* name = "every built-in text is present";
    const OfferTexts t = Texts();
    CHECK(!t.title.empty() && !t.accept.empty() && !t.cancel.empty() && !t.close.empty());
    CHECK(!t.notNow.empty() && !t.feed.empty() && !t.feedMore.empty());
    CHECK(t.beat == "GOLPEAR" && t.knockout == "NOQUEAR");
    CHECK(!t.reasonDefiantNoBeat.empty());
    CHECK(!t.ignore.empty() && !t.decline.empty() && !t.free.empty());
    CHECK(!t.priceLabel.empty() && !t.purseLabel.empty() && !t.chanceLabel.empty());
    CHECK(t.catsValue.find("{cats}") != std::string::npos);
    CHECK(t.shortPurse.find("{cats}") != std::string::npos);
    CHECK(t.bodyCats.find("{name}") != std::string::npos);
    CHECK(t.bodyRefused.find("{name}") != std::string::npos);
    CHECK(!t.haggleLabel.empty() && !t.haggleOffer.empty() && !t.haggleOffended.empty());
    CHECK(t.haggleOdds.find("{pct}") != std::string::npos);
    CHECK(t.haggleShort.find("{cats}") != std::string::npos);
}

// ------------------------------------------------------------------ haggle
// ui-mockup/readout.html section 5, states 2-4 (js/popup_data.js).

OfferInput HaggleKareem(int offer)
{
    OfferInput in;
    in.name = "Kareem";
    in.verdict = CatsVerdict(true, 9570, 0);
    in.verdict.viaCaptivity = true;
    in.purseKnown = true;
    in.purse = 12000;
    in.haggle.enabled = true;
    in.haggle.leverage = 8;
    in.haggle.personality = Temperament_Normal;
    in.haggle.offer = offer;
    return in;
}

void OddsTonesSplitAtSeventyAndThirty()
{
    const char* name = "haggle odds tone: ready >= 70, amber 30-69, danger < 30";
    CHECK(OddsToneOf(100) == OddsTone_Ready);
    CHECK(OddsToneOf(70) == OddsTone_Ready);
    CHECK(OddsToneOf(69) == OddsTone_Amber);
    CHECK(OddsToneOf(30) == OddsTone_Amber);
    CHECK(OddsToneOf(29) == OddsTone_Danger);
    CHECK(OddsToneOf(0) == OddsTone_Danger);
}

void HaggleOffKeepsTheTakeItOrLeaveItWindow()
{
    const char* name = "haggle off: no slider, no line, ACEPTAR as before";
    OfferInput in = HaggleKareem(7650);
    in.haggle.enabled = false;
    const OfferTexts t = Texts();
    const OfferView v = BuildOfferView(in, t);
    CHECK(!v.showSlider);
    CHECK(v.haggleLine.empty());
    CHECK(!v.haggleLocked);
    CHECK(v.acceptLabel == t.accept);
    CHECK(!v.acceptIsOffer);
}

void SliderStartsAtTheAskWithAccept()
{
    const char* name = "haggle: the slider starts at the ask, 100%, ACEPTAR";
    const OfferTexts t = Texts();
    const OfferView v = BuildOfferView(HaggleKareem(0), t);
    CHECK(v.showSlider);
    CHECK(v.haggleLabel == t.haggleLabel);
    CHECK(v.haggleBounds.minOffer == 4790 && v.haggleBounds.maxOffer == 9570);
    CHECK(v.sliderSteps == 478);
    CHECK(v.sliderStep == 478);
    CHECK(v.offerCats == 9570);
    CHECK(v.offerText == "9.570 cats");
    CHECK(v.minText == "4.790");
    CHECK(v.maxText == "9.570");
    CHECK(v.oddsPercent == 100);
    CHECK(v.haggleTone == OddsTone_Ready);
    CHECK(v.haggleLine == "Probabilidad de aceptar: 100%");
    CHECK(v.acceptLabel == t.accept);
    CHECK(!v.acceptIsOffer);
    CHECK(v.affordable);
    CHECK(v.reasonText.empty());
    CHECK(v.priceCats == 9570);
}

void OfferBelowTheAskIsAnOffer()
{
    const char* name = "haggle: an offer below the ask reads OFRECER with its odds";
    const OfferTexts t = Texts();
    const OfferView v = BuildOfferView(HaggleKareem(7650), t);
    CHECK(v.showSlider);
    CHECK(v.offerCats == 7650);
    CHECK(v.sliderStep == 286);
    CHECK(v.offerText == "7.650 cats");
    // 100 - 20% * 2 + 8 = 68
    CHECK(v.oddsPercent == 68);
    CHECK(v.haggleTone == OddsTone_Amber);
    CHECK(v.haggleLine == "Probabilidad de aceptar: 68%");
    CHECK(v.acceptLabel == t.haggleOffer);
    CHECK(v.acceptIsOffer);
    CHECK(v.cancelLabel == t.cancel);
    CHECK(v.affordable);
    CHECK(v.reasonText.empty());
    // The price row still says what they asked.
    CHECK(v.priceText == "9.570 cats");

    const OfferView low = BuildOfferView(HaggleKareem(4790), t);
    CHECK(low.oddsPercent == 8);
    CHECK(low.haggleTone == OddsTone_Danger);
}

void OfferIsHeldOnTheSlider()
{
    const char* name = "haggle: an offer outside the slider is clamped and snapped";
    const OfferView floorView = BuildOfferView(HaggleKareem(100), Texts());
    CHECK(floorView.offerCats == 4790);
    CHECK(floorView.sliderStep == 0);
    const OfferView snapped = BuildOfferView(HaggleKareem(7656), Texts());
    CHECK(snapped.offerCats == 7660);
}

void UnaffordableOfferHidesOffer()
{
    const char* name = "haggle: the purse is checked against the offer";
    OfferInput in = HaggleKareem(0);
    in.verdict = CatsVerdict(false, 9570, 6000);
    in.purse = 6000;
    in.haggle.leverage = -6;
    in.haggle.personality = Temperament_Brave;
    const OfferTexts t = Texts();

    const OfferView atAsk = BuildOfferView(in, t);
    CHECK(atAsk.showSlider);
    CHECK(!atAsk.affordable);
    CHECK(atAsk.shortfall == 3570);
    CHECK(atAsk.haggleLine == "No te alcanzan para esa oferta (te faltan 3.570)");
    CHECK(atAsk.haggleTone == OddsTone_Danger);
    CHECK(atAsk.acceptLabel.empty());
    CHECK(!atAsk.acceptIsOffer);
    // The shortfall lives in the slider's line, never on the reason plate, so a
    // drag never changes the window's height.
    CHECK(atAsk.reasonText.empty());
    CHECK(atAsk.cancelLabel == t.cancel);

    in.haggle.offer = 5640;
    const OfferView within = BuildOfferView(in, t);
    CHECK(within.affordable);
    CHECK(within.shortfall == 0);
    CHECK(within.acceptIsOffer);
    CHECK(within.acceptLabel == t.haggleOffer);
    CHECK(within.reasonText.empty());
}

void OffendedTodayShowsTheLockNotTheSlider()
{
    const char* name = "haggle: offended today -> locked line, no slider, ACEPTAR at the raised price";
    OfferInput in = HaggleKareem(7650);
    in.verdict = CatsVerdict(true, 11490, 0);
    in.haggle.locked = true;
    const OfferTexts t = Texts();
    const OfferView v = BuildOfferView(in, t);
    CHECK(!v.showSlider);
    CHECK(v.haggleLocked);
    CHECK(v.haggleLine == t.haggleOffended);
    CHECK(v.haggleTone == OddsTone_Danger);
    CHECK(v.priceCats == 11490);
    CHECK(v.acceptLabel == t.accept);
    CHECK(!v.acceptIsOffer);

    // Locked and short: the purse plate as before, plus the lock line.
    in.purse = 3200;
    const OfferView shortLocked = BuildOfferView(in, t);
    CHECK(shortLocked.haggleLocked);
    CHECK(!shortLocked.affordable);
    CHECK(shortLocked.acceptLabel.empty());
    CHECK(!shortLocked.reasonText.empty());
}

void FreeRoutesNeverHaggle()
{
    const char* name = "haggle: rescue, field yield, refusals and mercy have no slider";
    const OfferTexts t = Texts();

    OfferInput rescue;
    rescue.name = "Esclavo";
    rescue.verdict.willJoin = true;
    rescue.verdict.viaRescue = true;
    rescue.haggle.enabled = true;
    OfferView v = BuildOfferView(rescue, t);
    CHECK(!v.showSlider && v.haggleLine.empty());

    OfferInput yield;
    yield.name = "Soldado";
    yield.verdict.willJoin = true;
    yield.verdict.currency = Currency_Submission;
    yield.verdict.reasonKey = Reason_FieldYieldJoins;
    yield.haggle.enabled = true;
    v = BuildOfferView(yield, t);
    CHECK(!v.showSlider && v.haggleLine.empty());

    OfferInput refused;
    refused.name = "Paladin";
    refused.verdict.reasonKey = Reason_MilitaryLoyalty;
    refused.haggle.enabled = true;
    refused.haggle.locked = true;
    v = BuildOfferView(refused, t);
    CHECK(!v.showSlider && v.haggleLine.empty() && !v.haggleLocked);

    OfferInput animal;
    animal.name = "Garru";
    animal.verdict.isAnimal = true;
    animal.verdict.currency = Currency_Care;
    animal.verdict.cats = 40;
    animal.haggle.enabled = true;
    v = BuildOfferView(animal, t);
    CHECK(!v.showSlider && v.haggleLine.empty());

    v = BuildMercyOfferView("Bandido", "Bandido: Piedad.", true, false, t);
    CHECK(!v.showSlider && v.haggleLine.empty() && !v.acceptIsOffer);

    v = BuildSubmissionOfferView("Bandido", false, t);
    CHECK(!v.showSlider && v.haggleLine.empty() && !v.acceptIsOffer);
}

// cage-animals-submission T3: a beaten NPC walked up to ask. Sometido tag, the
// kneeling line, free, and Aceptar / Perdonar / Rechazar.
void SubmissionHasAcceptForgiveReject()
{
    const char* name = "submission event: kneeling body, free, Accept / Forgive / Reject";
    const OfferTexts t = Texts();
    const OfferView v = BuildSubmissionOfferView("Bandido", false, t);
    CHECK(v.route == ReadoutRoute_FieldYield);
    CHECK(v.routeTag == t.routeFieldYield);
    CHECK(v.name == "Bandido");
    CHECK(v.bodyLine == "Bandido se arrodilla y te pide unirse a tu grupo.");
    CHECK(v.showPrice && v.priceFree && v.priceText == t.free);
    CHECK(!v.showPurse && !v.showChance);
    CHECK(v.reasonText.empty() && !v.reasonRefused);
    CHECK(v.acceptLabel == t.accept);
    CHECK(v.extraLabel == t.forgive);
    CHECK(v.cancelLabel == t.decline);
    CHECK(t.forgive == "PERDONAR");
    CHECK(t.bodySubmitted.find("{name}") != std::string::npos);
}

// The director screens out the caged (ScreenCandidate), but the player can pick
// the NPC up on the way: whoever cannot leave is never offered PERDONAR.
void CagedSubmissionHasNoForgive()
{
    const char* name = "submission event, not free to leave: Accept / Reject, no PERDONAR";
    const OfferTexts t = Texts();
    const OfferView v = BuildSubmissionOfferView("Bandido", true, t);
    CHECK(v.acceptLabel == t.accept);
    CHECK(v.extraLabel.empty());
    CHECK(v.cancelLabel == t.decline);
}

// ------------------------------------------------------------ The helper
// ui-mockup/readout.html section 5, the helper state (js/popup_data.js).
void HelperOffersFreeHelpWithAcceptAndReject()
{
    const char* name = "helper offer: Ayuda tag, the days in the body, free, ACEPTAR / RECHAZAR";
    const OfferTexts t = Texts();
    const OfferView v = BuildHelperOfferView("Beep", 4, t);
    CHECK(v.route == ReadoutRoute_Free);
    CHECK(v.routeTag == t.routeHelper && t.routeHelper == "Ayuda");
    CHECK(v.name == "Beep");
    CHECK(v.bodyLine.find("Beep te ofrece acompa") == 0);
    CHECK(v.bodyLine.find(" 4 d") != std::string::npos);
    CHECK(v.bodyLine.find("{days}") == std::string::npos);
    CHECK(v.bodyLine.find("gratis") != std::string::npos);
    CHECK(v.showPrice && v.priceFree && v.priceText == t.free);
    CHECK(!v.showPurse && !v.showChance && !v.showSlider);
    CHECK(v.reasonText.empty());
    CHECK(v.acceptLabel == t.accept);
    CHECK(v.extraLabel.empty() && v.beatLabel.empty() && v.knockoutLabel.empty());
    CHECK(v.cancelLabel == t.decline);
    CHECK(t.bodyHelper.find("{name}") != std::string::npos);
    CHECK(t.bodyHelper.find("{days}") != std::string::npos);
}

// ------------------------------------------------------- Balanced allies
// ui-mockup/readout.html section 5, the ally states (js/popup_data.js).

Verdict AllyPaid(int cats, int level, int discount)
{
    Verdict v = CatsVerdict(true, cats, 0);
    v.allyTiered = true;
    v.allyLevel = level;
    v.allyDiscountPercent = discount;
    return v;
}

void APayingAllyNamesTheTierAndTheDiscount()
{
    const char* name = "ally paid: body names level and discount, PRECIO carries the -N% tag";
    OfferInput in;
    in.name = "Gor";
    in.verdict = AllyPaid(1840, 34, 14);
    in.purseKnown = true;
    in.purse = 5000;
    const OfferTexts t = Texts();
    const OfferView v = BuildOfferView(in, t);
    CHECK(v.bodyLine == "Aliado (nivel 34): Gor te cobra 1.840 cats, con 14% de descuento por amistad.");
    CHECK(v.showPrice && !v.priceFree);
    CHECK(v.priceText == "1.840 cats");
    CHECK(v.priceTag == "-14%");
    CHECK(v.acceptLabel == t.accept);
    CHECK(v.reasonText.empty());
}

void APayingAllyWithNoDiscountSaysJustThePrice()
{
    const char* name = "ally paid at 0%: no tag, the plain ally body";
    OfferInput in;
    in.name = "Gor";
    in.verdict = AllyPaid(2000, 60, 0);
    const OfferView v = BuildOfferView(in, Texts());
    CHECK(v.priceTag.empty());
    CHECK(v.bodyLine == "Aliado (nivel 60): Gor te cobra 2.000 cats.");
}

void AShortAllyStillSeesTheTierAndTheShortfall()
{
    const char* name = "ally paid, short purse: the tier body stays, the shortfall is red, no Accept";
    OfferInput in;
    in.name = "Gor";
    in.verdict = AllyPaid(1840, 34, 14);
    in.verdict.willJoin = false;
    in.verdict.reasonKey = Reason_CatsAsk;
    in.purseKnown = true;
    in.purse = 840;
    const OfferView v = BuildOfferView(in, Texts());
    CHECK(v.bodyLine.find("14%") != std::string::npos);
    CHECK(v.priceTag == "-14%");
    CHECK(!v.affordable);
    CHECK(v.shortfall == 1000);
    CHECK(v.reasonRefused);
    CHECK(v.acceptLabel.empty());
}

void AHaggledAllyKeepsTheTierBody()
{
    const char* name = "ally paid with haggling: the slider works on the discounted ask, the body keeps the tier";
    OfferInput in;
    in.name = "Gor";
    in.verdict = AllyPaid(1840, 34, 14);
    in.purseKnown = true;
    in.purse = 5000;
    in.haggle.enabled = true;
    const OfferView v = BuildOfferView(in, Texts());
    CHECK(v.showSlider);
    CHECK(v.haggleBounds.maxOffer == 1840);
    CHECK(v.priceTag == "-14%");
    CHECK(v.bodyLine.find("nivel 34") != std::string::npos);
}

void ATemporaryAllySaysForHowManyDays()
{
    const char* name = "ally temporary: free, the body says the days";
    OfferInput in;
    in.name = "Beep";
    in.verdict.willJoin = true;
    in.verdict.currency = Currency_None;
    in.verdict.reasonKey = Reason_AllyTemporary;
    in.verdict.allyTiered = true;
    in.verdict.allyLevel = 85;
    in.verdict.temporary = true;
    in.verdict.temporaryDays = 6;
    const OfferTexts t = Texts();
    const OfferView v = BuildOfferView(in, t);
    CHECK(v.showPrice && v.priceFree);
    CHECK(v.priceTag.empty());
    CHECK(v.bodyLine.find("Aliado (nivel 85): Beep te acompa") == 0);
    CHECK(v.bodyLine.find(" 6 d") != std::string::npos);
    CHECK(v.acceptLabel == t.accept);
    CHECK(!v.showSlider);
}

void AFreeGoodAllySaysItIsFriendship()
{
    const char* name = "ally free (Good, under 20): Gratis, the body says it is friendship";
    OfferInput in;
    in.name = "Ruka";
    in.verdict.willJoin = true;
    in.verdict.currency = Currency_None;
    in.verdict.reasonKey = Reason_AllyJoins;
    in.verdict.rolled = true;
    in.verdict.chancePercent = 45;
    in.verdict.allyTiered = true;
    in.verdict.allyLevel = 12;
    const OfferView v = BuildOfferView(in, Texts());
    CHECK(v.priceFree);
    CHECK(v.bodyLine == "Ruka es tu aliado (nivel 12) y se une gratis por amistad.");
    CHECK(v.showChance);
}

void ARefusingAllyReadsAsARefusal()
{
    const char* name = "ally refused (too weak, cruel, greedy): the usual refusal window with the reason";
    OfferInput in;
    in.name = "Beep";
    in.verdict.currency = Currency_Refused;
    in.verdict.reasonKey = Reason_AllyLeaderTooWeak;
    in.verdict.reason = "eres demasiado debil para que te acompane";
    in.verdict.allyTiered = true;
    in.verdict.allyLevel = 95;
    const OfferTexts t = Texts();
    const OfferView v = BuildOfferView(in, t);
    CHECK(v.reasonRefused);
    CHECK(v.reasonText == "Eres demasiado debil para que te acompane");
    CHECK(v.acceptLabel.empty());
    CHECK(v.cancelLabel == t.close);
    CHECK(!v.showPrice);
    CHECK(v.priceTag.empty());
}

void AnUntieredPriceHasNoTag()
{
    const char* name = "a plain bribe or cage price never carries a discount tag";
    OfferInput in;
    in.name = "Merc";
    in.verdict = CatsVerdict(true, 9570, 0);
    in.verdict.allyDiscountPercent = 30;   // ignored without allyTiered
    const OfferView v = BuildOfferView(in, Texts());
    CHECK(v.priceTag.empty());
    CHECK(v.bodyLine == "Merc se unir\xc3\xa1 a tu grupo por 9.570 cats.");
}

void TheAllyTextsAreComplete()
{
    const char* name = "every ally template is present with its placeholders";
    const OfferTexts t = Texts();
    CHECK(t.allyFreeBody.find("{level}") != std::string::npos);
    CHECK(t.allyPaidBody.find("{pct}") != std::string::npos && t.allyPaidBody.find("{cats}") != std::string::npos);
    CHECK(t.allyPaidFullBody.find("{cats}") != std::string::npos);
    CHECK(t.allyTemporaryBody.find("{days}") != std::string::npos);
    CHECK(t.discountTag.find("{pct}") != std::string::npos);
}

// A level-85 temporary ally the caller offers the duel for (duel_challenge_rules.h
// CanOfferDuel), with the head-to-head record read from TotalSparring.
OfferInput DuelableTemporaryAlly()
{
    OfferInput in;
    in.name = "Beep";
    in.verdict.willJoin = true;
    in.verdict.currency = Currency_None;
    in.verdict.reasonKey = Reason_AllyTemporary;
    in.verdict.allyTiered = true;
    in.verdict.allyLevel = 85;
    in.verdict.temporary = true;
    in.verdict.temporaryDays = 6;
    in.duel.offered = true;
    in.duel.recordKnown = true;
    in.duel.wins = 2;
    in.duel.losses = 1;
    in.duel.draws = 0;
    return in;
}

void ADuelableAllyKeepsItsTierAndAddsTheDuel()
{
    const char* name = "duel: the tier's own answer stays, RETAR A DUELO and the record are added";
    const OfferTexts t = Texts();
    const OfferView v = BuildOfferView(DuelableTemporaryAlly(), t);
    CHECK(v.acceptLabel == t.accept);
    CHECK(v.bodyLine.find("Aliado (nivel 85): Beep te acompa") == 0);
    CHECK(v.duelLabel == t.duelChallenge);
    CHECK(v.duelLabel == "RETAR A DUELO");
    CHECK(v.showDuelRecord);
    CHECK(v.duelRecordLabel == t.duelRecordLabel);
    CHECK(v.duelRecordText == "2-1");
    CHECK(v.cancelLabel == t.cancel);
}

void NoDuelWithoutTheCallersYes()
{
    const char* name = "duel: not offered -> no button and no record, whatever the record says";
    OfferInput in = DuelableTemporaryAlly();
    in.duel.offered = false;
    const OfferView v = BuildOfferView(in, Texts());
    CHECK(v.duelLabel.empty());
    CHECK(!v.showDuelRecord);
    CHECK(v.duelRecordText.empty());
}

void AnUnreadRecordStillOffersTheDuel()
{
    const char* name = "duel: the record could not be read -> the button without the record row";
    OfferInput in = DuelableTemporaryAlly();
    in.duel.recordKnown = false;
    const OfferView v = BuildOfferView(in, Texts());
    CHECK(!v.duelLabel.empty());
    CHECK(!v.showDuelRecord);
}

void ARefusingEliteAllyCanStillBeChallenged()
{
    const char* name = "duel: a cruel 80+ ally refuses, and the duel is the way in (draws in the record)";
    OfferInput in;
    in.name = "Cruel";
    in.verdict.currency = Currency_Refused;
    in.verdict.reasonKey = Reason_AllyCruel;
    in.verdict.reason = "no camina al lado de nadie";
    in.verdict.allyTiered = true;
    in.verdict.allyLevel = 90;
    in.duel.offered = true;
    in.duel.recordKnown = true;
    in.duel.wins = 0;
    in.duel.losses = 3;
    in.duel.draws = 1;
    const OfferTexts t = Texts();
    const OfferView v = BuildOfferView(in, t);
    CHECK(v.reasonRefused);
    CHECK(v.acceptLabel.empty());
    CHECK(v.duelLabel == t.duelChallenge);
    CHECK(v.duelRecordText == "0-3-1");
    CHECK(v.cancelLabel == t.close);
}

void TheDuelRidesBesideTheSlider()
{
    const char* name = "duel: a greedy 80+ ally's haggle keeps its slider and adds the duel";
    OfferInput in = HaggleKareem(0);
    in.verdict.allyTiered = true;
    in.verdict.allyLevel = 85;
    in.duel.offered = true;
    const OfferView v = BuildOfferView(in, Texts());
    CHECK(v.showSlider);
    CHECK(!v.duelLabel.empty());
}

void TheDuelTextsAreComplete()
{
    const char* name = "duel: the button and the record label have built-in text";
    const OfferTexts t = Texts();
    CHECK(t.duelChallenge == "RETAR A DUELO");
    CHECK(t.duelRecordLabel == "DUELOS");
}
} // namespace

void RunOfferViewTests()
{
    ADuelableAllyKeepsItsTierAndAddsTheDuel();
    NoDuelWithoutTheCallersYes();
    AnUnreadRecordStillOffersTheDuel();
    ARefusingEliteAllyCanStillBeChallenged();
    TheDuelRidesBesideTheSlider();
    TheDuelTextsAreComplete();
    APayingAllyNamesTheTierAndTheDiscount();
    APayingAllyWithNoDiscountSaysJustThePrice();
    AShortAllyStillSeesTheTierAndTheShortfall();
    AHaggledAllyKeepsTheTierBody();
    ATemporaryAllySaysForHowManyDays();
    AFreeGoodAllySaysItIsFriendship();
    ARefusingAllyReadsAsARefusal();
    AnUntieredPriceHasNoTag();
    TheAllyTextsAreComplete();
    JoinsForCatsShowsPriceAndPurse();
    ShortPurseHidesAcceptAndSaysTheShortfall();
    ShortPurseReadFromTheVerdictWhenNotSupplied();
    PurseDroppedBelowAYesStillBlocksAccept();
    UnknownPurseNeverBlocks();
    RefusalIsRedWithCloseOnly();
    RefusalWithRemedyOffersIt();
    RemedyNeverRidesOnAYes();
    RescueIsFree();
    FieldYieldJoinsFreeWithOdds();
    GiftIsNotRed();
    AnimalStomachIsNotMoney();
    MercyHasThreeAnswers();
    CagedPleaHasNoIgnore();
    CagedRefusalOffersTheCageCommands();
    CagedHungryOffersFoodAndTheCageCommands();
    CageCommandsFollowTheContextMenuRule();
    CagedYesKeepsAcceptAndCancel();
    CageCommandsNeverRideTheSlider();
    DefaultsAreComplete();
    OddsTonesSplitAtSeventyAndThirty();
    HaggleOffKeepsTheTakeItOrLeaveItWindow();
    SliderStartsAtTheAskWithAccept();
    OfferBelowTheAskIsAnOffer();
    OfferIsHeldOnTheSlider();
    UnaffordableOfferHidesOffer();
    OffendedTodayShowsTheLockNotTheSlider();
    FreeRoutesNeverHaggle();
    SubmissionHasAcceptForgiveReject();
    CagedSubmissionHasNoForgive();
    HelperOffersFreeHelpWithAcceptAndReject();
}
