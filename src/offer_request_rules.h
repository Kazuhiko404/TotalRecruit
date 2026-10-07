#pragma once

// The proactive offer's queue, without the engine.
//
// User request (2026-10-05, proactive-offer): "agregar un evento que el npc o animal abra
// el popup de que quiere unirse, no tener yo que hablar de nuevo [...] despues de ganarle
// en duelo [...] o curar". A gesture of the player (a won duel, a heal by the squad) asks
// the submission event (submission_director.cpp) for one offer from that character; the
// event's sweep serves requested offers before the candidates it finds by defeats.
//
//   Request: one entry per character key. Asked again while it is still pending, it is
//     only refreshed (a treatment calls every frame); a settled roll (a duel's Join or
//     Calm) replaces a pending Roll, never the reverse.
//   Consume: the sweep served it (walked up or calmed), it could never be served
//     (already the player's, excluded, caged), or the player talked by hand. A used-up
//     request does not come back while the same gesture goes on: asked again for the
//     same trigger within OfferGestureGapSec of the last time it was seen, it is the same
//     gesture and ignored. Every won duel is its own gesture (gap 0).
//   Expire: a pending request not seen for kOfferRequestTtlSec (real seconds, frozen on
//     pause) is dropped: the character never got up or never came within reach.
//
// The plan for a served request: a settled yes walks up and opens the window, a settled
// no calms in place (when it is hostile); a Roll asks the verdict (rules.h Evaluate) and
// walks up only for a free, lasting join (the window is free and ACEPTAR is a plain
// Recruit).
// tests/rules_tests/offer_request_rules_tests.cpp.

#include "char_key.h"
#include "submission_rules.h"
#include "verdict.h"

#include <vector>

namespace totalrecruit
{
enum OfferTrigger
{
    OfferTrigger_DuelWin = 0,
    OfferTrigger_Heal,
    OfferTrigger_Count
};

const char* OfferTriggerName(OfferTrigger trigger);

// What the gesture already knows about the answer.
enum OfferRequestVerdict
{
    OfferVerdict_Roll = 0,   // nothing yet: the verdict (Evaluate) decides when it is served
    OfferVerdict_Join,       // the roll passed (a won duel): walk up and offer
    OfferVerdict_Calm        // the roll failed (a won duel in Balanced): calm in place
};

const char* OfferVerdictName(OfferRequestVerdict verdict);

// A pending request no one served for this long is dropped (event seconds).
const float kOfferRequestTtlSec = 300.0f;
// A heal asked again within this of the last frame it was seen is the same treatment.
const float kOfferHealGestureGapSec = 30.0f;

// PURE: how long after it was last seen a used-up request still stands for the same
// gesture: 0 for a won duel (each is new), kOfferHealGestureGapSec for a heal.
float OfferGestureGapSec(OfferTrigger trigger);

struct OfferRequest
{
    CharKey key;
    OfferTrigger trigger;
    OfferRequestVerdict verdict;
    float lastSeenAt;
    bool consumed;

    OfferRequest();
};

enum OfferRequestResult
{
    OfferRequest_Queued = 0,    // a new request (or a new gesture after a used one)
    OfferRequest_Refreshed,     // already pending: seen again (its verdict may be settled now)
    OfferRequest_SameGesture    // used up and the same gesture still goes on: ignored
};

const char* OfferRequestResultName(OfferRequestResult result);

class OfferRequestQueue
{
public:
    OfferRequestResult Request(const CharKey& key, OfferTrigger trigger,
                               OfferRequestVerdict verdict, float now);
    // The pending (not used-up) request for `key`.
    bool FindPending(const CharKey& key, OfferRequest* out) const;
    bool HasPending() const;
    int PendingCount() const;
    // Uses up the pending request for `key`; false when there was none.
    bool Consume(const CharKey& key, float now);
    // Drops pending requests past kOfferRequestTtlSec (returns how many) and used-up
    // ones past their gesture gap.
    int Expire(float now);
    void Clear();

private:
    int IndexOf(const CharKey& key) const;

    std::vector<OfferRequest> entries_;
};

enum RequestedOfferPlan
{
    RequestedPlan_WalkUp = 0,   // walk up to the player and open the window
    RequestedPlan_Calm,         // stop attacking for failCalmSeconds where it stands
    RequestedPlan_Nothing       // a no from someone not hostile: nothing to calm, no line
};

const char* RequestedPlanName(RequestedOfferPlan plan);

// PURE: Join walks up; Roll walks up when the verdict is a free join. A no (Calm, or a
// Roll that is not a free join) calms only someone `hostile` to the squad -- the calm
// is the user's "unos segundos de neutralidad" instead of being attacked.
RequestedOfferPlan PlanRequestedOffer(OfferRequestVerdict verdict, bool joinsFree, bool hostile);

// PURE: whether a verdict is a join the event's window can offer: free (no cats
// asked), lasting (not an ally's temporary stay) and a yes.
bool JoinsForFree(bool willJoin, Currency currency, int cats, bool temporary);

// PURE: whether a screening skip means the request can never be served (it is used
// up), rather than "not now" (busy, down: it waits).
bool RequestSkipConsumes(CandidateSkip skip);

// How far from the squad leader a gesture still asks for an offer: the event's own
// reach (kSubmissionAbortMeters, where the walk-up asks and gives up). The one
// source of truth for the offer's reach (offer_requests.cpp WithinOfferReach) and
// for how far a won duel's rival is looked for (priority-one A).
const float kOfferReachMeters = kSubmissionAbortMeters;

// PURE: within the offer's reach: a distance that was read (>= 0), at most
// kOfferReachMeters.
bool InOfferReach(float metersToLeader);

// PURE: how far from any squad member the rival of a finished duel is looked for
// (recruit_core.cpp FindNearSquad). It covers the whole offer reach: a search that
// stopped at 80 m left a duel won with the rival 80-120 m away recruiting nothing,
// while the offer would have reached them.
float DuelRivalSearchMeters();
} // namespace totalrecruit
