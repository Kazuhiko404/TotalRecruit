#include "route_decisions.h"

namespace totalrecruit
{
// The price both the open world and the cage quote, in one place: it used to be
// written out twice, word for word, one in each route.
//
// A price that cannot be paid is never a refusal, only a "not yet" with the
// numbers attached. When the purse cannot be read the offer stands, because
// refusing on the strength of a pointer that would not resolve is the worse
// failure; the charge itself is all-or-nothing (money_rules.cpp).
void ApplyCatsOffer(Verdict* verdict, const PriceInputs& price, const PurseFact& purse,
                    const Prices& prices)
{
    verdict->currency = Currency_Cats;
    verdict->cats = FinalPriceInCats(price, prices);
    verdict->purse = purse.cats;

    if (purse.known && purse.cats < verdict->cats)
    {
        verdict->reasonKey = Reason_CatsAsk;
        return;
    }

    verdict->willJoin = true;
    verdict->reasonKey = Reason_CatsJoin;
}
} // namespace totalrecruit
