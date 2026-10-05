// Tests for src/offer_request_rules.cpp: the queue of offers a gesture of the player
// asked for (a won duel, a heal), one per gesture, the plan for each request (walk up
// and offer, or calm in place) and which screening skips use a request up.

#include "test_support.h"
#include "offer_request_rules.h"

#include <cmath>

using namespace totalrecruit;

namespace
{
const CharKey kWolf(7, 1);
const CharKey kBandit(9, 4);

void ARequestIsQueuedOnceAndFoundByKey()
{
    const char* name = "offer queue: a request is queued once and found by its key";
    OfferRequestQueue q;
    CHECK(!q.HasPending());
    CHECK(q.Request(kWolf, OfferTrigger_Heal, OfferVerdict_Roll, 10.0f) == OfferRequest_Queued);
    CHECK(q.HasPending());
    CHECK(q.PendingCount() == 1);
    OfferRequest r;
    CHECK(q.FindPending(kWolf, &r));
    CHECK(r.trigger == OfferTrigger_Heal && r.verdict == OfferVerdict_Roll && !r.consumed);
    CHECK(!q.FindPending(kBandit, &r));
    // A treatment calls every frame: the same pending request is only refreshed.
    CHECK(q.Request(kWolf, OfferTrigger_Heal, OfferVerdict_Roll, 10.5f) == OfferRequest_Refreshed);
    CHECK(q.PendingCount() == 1);
}

void APreRolledVerdictReplacesARollButNotTheOtherWayRound()
{
    const char* name = "offer queue: a duel's settled roll replaces a pending heal roll, never the reverse";
    OfferRequestQueue q;
    q.Request(kWolf, OfferTrigger_Heal, OfferVerdict_Roll, 1.0f);
    CHECK(q.Request(kWolf, OfferTrigger_DuelWin, OfferVerdict_Join, 2.0f) == OfferRequest_Refreshed);
    OfferRequest r;
    CHECK(q.FindPending(kWolf, &r));
    CHECK(r.verdict == OfferVerdict_Join && r.trigger == OfferTrigger_DuelWin);
    q.Request(kWolf, OfferTrigger_Heal, OfferVerdict_Roll, 3.0f);
    CHECK(q.FindPending(kWolf, &r));
    CHECK(r.verdict == OfferVerdict_Join && r.trigger == OfferTrigger_DuelWin);
    // A newer settled roll wins over an older one.
    q.Request(kWolf, OfferTrigger_DuelWin, OfferVerdict_Calm, 4.0f);
    CHECK(q.FindPending(kWolf, &r));
    CHECK(r.verdict == OfferVerdict_Calm);
}

void AConsumedHealDoesNotPopAgainUntilANewGesture()
{
    const char* name = "offer queue: a used-up heal stays used while the treatment goes on; a new heal later asks again";
    OfferRequestQueue q;
    q.Request(kWolf, OfferTrigger_Heal, OfferVerdict_Roll, 0.0f);
    CHECK(q.Consume(kWolf, 5.0f));
    CHECK(!q.HasPending());
    CHECK(!q.Consume(kWolf, 5.0f));
    // The bandage keeps going: each frame is the same gesture.
    CHECK(q.Request(kWolf, OfferTrigger_Heal, OfferVerdict_Roll, 6.0f) == OfferRequest_SameGesture);
    CHECK(q.Request(kWolf, OfferTrigger_Heal, OfferVerdict_Roll, 30.0f) == OfferRequest_SameGesture);
    CHECK(!q.HasPending());
    // A pause longer than the gap: a new heal is a new gesture.
    CHECK(q.Request(kWolf, OfferTrigger_Heal, OfferVerdict_Roll,
                    30.0f + OfferGestureGapSec(OfferTrigger_Heal)) == OfferRequest_Queued);
    CHECK(q.HasPending());
}

void EveryDuelWinIsANewGesture()
{
    const char* name = "offer queue: every won duel is its own gesture; a different trigger is too";
    OfferRequestQueue q;
    CHECK(OfferGestureGapSec(OfferTrigger_DuelWin) == 0.0f);
    CHECK(OfferGestureGapSec(OfferTrigger_Heal) > 0.0f);
    q.Request(kBandit, OfferTrigger_DuelWin, OfferVerdict_Join, 0.0f);
    q.Consume(kBandit, 1.0f);
    CHECK(q.Request(kBandit, OfferTrigger_DuelWin, OfferVerdict_Join, 1.0f) == OfferRequest_Queued);
    q.Consume(kBandit, 2.0f);
    q.Request(kWolf, OfferTrigger_Heal, OfferVerdict_Roll, 2.0f);
    q.Consume(kWolf, 2.0f);
    CHECK(q.Request(kWolf, OfferTrigger_DuelWin, OfferVerdict_Join, 3.0f) == OfferRequest_Queued);
}

void APendingRequestExpiresAndAUsedOneIsForgotten()
{
    const char* name = "offer queue: a request nobody answered expires; a used one is forgotten after its gap";
    OfferRequestQueue q;
    q.Request(kWolf, OfferTrigger_Heal, OfferVerdict_Roll, 0.0f);
    q.Request(kBandit, OfferTrigger_Heal, OfferVerdict_Roll, 0.0f);
    q.Consume(kBandit, 0.0f);
    CHECK(q.Expire(kOfferRequestTtlSec - 1.0f) == 0);
    CHECK(q.HasPending());
    // Refreshed by a frame of treatment: the TTL counts from the last time it was asked.
    q.Request(kWolf, OfferTrigger_Heal, OfferVerdict_Roll, 100.0f);
    CHECK(q.Expire(kOfferRequestTtlSec + 50.0f) == 0);
    CHECK(q.Expire(100.0f + kOfferRequestTtlSec) == 1);
    CHECK(!q.HasPending());
    // The used bandit record went with the gap: a new heal asks at once.
    CHECK(q.Request(kBandit, OfferTrigger_Heal, OfferVerdict_Roll, 500.0f) == OfferRequest_Queued);
    q.Clear();
    CHECK(!q.HasPending());
}

void ARequestPlansAWalkUpOrACalm()
{
    const char* name = "offer plan: a settled yes walks up, a settled no calms, a roll asks the verdict";
    CHECK(PlanRequestedOffer(OfferVerdict_Join, false) == RequestedPlan_WalkUp);
    CHECK(PlanRequestedOffer(OfferVerdict_Calm, true) == RequestedPlan_Calm);
    CHECK(PlanRequestedOffer(OfferVerdict_Roll, true) == RequestedPlan_WalkUp);
    CHECK(PlanRequestedOffer(OfferVerdict_Roll, false) == RequestedPlan_Calm);
    CHECK(RequestedPlanName(RequestedPlan_Calm)[0] != '\0');
}

void OnlyAFreeLastingYesIsOfferedForNothing()
{
    const char* name = "offer plan: the window offers a free, lasting join; a price or a stay is not one";
    CHECK(JoinsForFree(true, Currency_Submission, 0, false));
    CHECK(JoinsForFree(true, Currency_None, 0, false));
    CHECK(JoinsForFree(true, Currency_Care, 0, false));
    CHECK(JoinsForFree(true, Currency_Cats, 0, false));
    CHECK(!JoinsForFree(true, Currency_Cats, 500, false));
    CHECK(!JoinsForFree(true, Currency_None, 0, true));
    CHECK(!JoinsForFree(false, Currency_Submission, 0, false));
    CHECK(!JoinsForFree(true, Currency_Refused, 0, false));
}

void ALastingSkipUsesTheRequestUp()
{
    const char* name = "offer plan: mine, excluded or caged uses the request up; busy or down waits";
    CHECK(RequestSkipConsumes(CandidateSkip_Mine));
    CHECK(RequestSkipConsumes(CandidateSkip_Excluded));
    CHECK(RequestSkipConsumes(CandidateSkip_Caged));
    CHECK(RequestSkipConsumes(CandidateSkip_Disabled));
    CHECK(!RequestSkipConsumes(CandidateSkip_Busy));
    CHECK(!RequestSkipConsumes(CandidateSkip_NotUp));
    CHECK(!RequestSkipConsumes(CandidateSkip_None));
}

void EveryNameIsReadable()
{
    const char* name = "offer names: triggers, verdicts and results read in English for the log";
    CHECK(OfferTriggerName(OfferTrigger_DuelWin)[0] != '\0');
    CHECK(OfferTriggerName(OfferTrigger_Heal)[0] != '\0');
    CHECK(OfferVerdictName(OfferVerdict_Roll)[0] != '\0');
    CHECK(OfferRequestResultName(OfferRequest_SameGesture)[0] != '\0');
}
} // namespace

void RunOfferRequestRulesTests()
{
    ARequestIsQueuedOnceAndFoundByKey();
    APreRolledVerdictReplacesARollButNotTheOtherWayRound();
    AConsumedHealDoesNotPopAgainUntilANewGesture();
    EveryDuelWinIsANewGesture();
    APendingRequestExpiresAndAUsedOneIsForgotten();
    ARequestPlansAWalkUpOrACalm();
    OnlyAFreeLastingYesIsOfferedForNothing();
    ALastingSkipUsesTheRequestUp();
    EveryNameIsReadable();
}
