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

    in.remedy = OfferRemedy_Beat;
    v = BuildOfferView(in, t);
    CHECK(v.extraLabel == t.beat);

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
    OfferView v = BuildMercyOfferView("Bandido", "Bandido: Piedad.", true, t);
    CHECK(v.route == ReadoutRoute_Mercy);
    CHECK(v.routeTag == t.routeMercy);
    CHECK(v.bodyLine == "Bandido: Piedad.");
    CHECK(v.acceptLabel == t.accept);
    CHECK(v.extraLabel == t.ignore);
    CHECK(v.cancelLabel == t.decline);
    CHECK(!v.showPrice && v.reasonText.empty());

    v = BuildMercyOfferView("Bandido", "Bandido: Piedad.", false, t);
    CHECK(v.extraLabel.empty());
}

void DefaultsAreComplete()
{
    const char* name = "every built-in text is present";
    const OfferTexts t = Texts();
    CHECK(!t.title.empty() && !t.accept.empty() && !t.cancel.empty() && !t.close.empty());
    CHECK(!t.notNow.empty() && !t.feed.empty() && !t.feedMore.empty() && !t.beat.empty());
    CHECK(!t.ignore.empty() && !t.decline.empty() && !t.free.empty());
    CHECK(!t.priceLabel.empty() && !t.purseLabel.empty() && !t.chanceLabel.empty());
    CHECK(t.catsValue.find("{cats}") != std::string::npos);
    CHECK(t.shortPurse.find("{cats}") != std::string::npos);
    CHECK(t.bodyCats.find("{name}") != std::string::npos);
    CHECK(t.bodyRefused.find("{name}") != std::string::npos);
}
} // namespace

void RunOfferViewTests()
{
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
    DefaultsAreComplete();
}
