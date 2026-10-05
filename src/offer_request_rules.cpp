#include "offer_request_rules.h"

namespace totalrecruit
{
OfferRequest::OfferRequest()
    : key(), trigger(OfferTrigger_DuelWin), verdict(OfferVerdict_Roll), lastSeenAt(0.0f),
      consumed(false)
{
}

const char* OfferTriggerName(OfferTrigger trigger)
{
    switch (trigger)
    {
    case OfferTrigger_DuelWin: return "duel win";
    case OfferTrigger_Heal:    return "heal";
    default:                   return "unknown";
    }
}

const char* OfferVerdictName(OfferRequestVerdict verdict)
{
    switch (verdict)
    {
    case OfferVerdict_Roll: return "roll";
    case OfferVerdict_Join: return "join";
    case OfferVerdict_Calm: return "calm";
    }
    return "unknown";
}

const char* OfferRequestResultName(OfferRequestResult result)
{
    switch (result)
    {
    case OfferRequest_Queued:      return "queued";
    case OfferRequest_Refreshed:   return "refreshed";
    case OfferRequest_SameGesture: return "same gesture, already used";
    }
    return "unknown";
}

float OfferGestureGapSec(OfferTrigger trigger)
{
    return trigger == OfferTrigger_Heal ? kOfferHealGestureGapSec : 0.0f;
}

int OfferRequestQueue::IndexOf(const CharKey& key) const
{
    for (unsigned int i = 0; i < entries_.size(); ++i)
    {
        if (entries_[i].key == key)
            return (int)i;
    }
    return -1;
}

OfferRequestResult OfferRequestQueue::Request(const CharKey& key, OfferTrigger trigger,
                                              OfferRequestVerdict verdict, float now)
{
    const int at = IndexOf(key);
    if (at < 0)
    {
        OfferRequest r;
        r.key = key;
        r.trigger = trigger;
        r.verdict = verdict;
        r.lastSeenAt = now;
        entries_.push_back(r);
        return OfferRequest_Queued;
    }

    OfferRequest& r = entries_[at];
    if (!r.consumed)
    {
        // A settled roll is newer news than an open one; an open one never undoes it.
        if (verdict != OfferVerdict_Roll)
        {
            r.verdict = verdict;
            r.trigger = trigger;
        }
        r.lastSeenAt = now;
        return OfferRequest_Refreshed;
    }

    if (trigger == r.trigger && now - r.lastSeenAt < OfferGestureGapSec(trigger))
    {
        r.lastSeenAt = now;
        return OfferRequest_SameGesture;
    }

    r.trigger = trigger;
    r.verdict = verdict;
    r.lastSeenAt = now;
    r.consumed = false;
    return OfferRequest_Queued;
}

bool OfferRequestQueue::FindPending(const CharKey& key, OfferRequest* out) const
{
    const int at = IndexOf(key);
    if (at < 0 || entries_[at].consumed)
        return false;
    if (out != 0)
        *out = entries_[at];
    return true;
}

bool OfferRequestQueue::HasPending() const
{
    return PendingCount() > 0;
}

int OfferRequestQueue::PendingCount() const
{
    int count = 0;
    for (unsigned int i = 0; i < entries_.size(); ++i)
    {
        if (!entries_[i].consumed)
            ++count;
    }
    return count;
}

bool OfferRequestQueue::Consume(const CharKey& key, float now)
{
    const int at = IndexOf(key);
    if (at < 0 || entries_[at].consumed)
        return false;
    entries_[at].consumed = true;
    entries_[at].lastSeenAt = now;
    return true;
}

int OfferRequestQueue::Expire(float now)
{
    int expired = 0;
    std::vector<OfferRequest> kept;
    for (unsigned int i = 0; i < entries_.size(); ++i)
    {
        const OfferRequest& r = entries_[i];
        const float since = now - r.lastSeenAt;
        if (!r.consumed && since >= kOfferRequestTtlSec)
        {
            ++expired;
            continue;
        }
        if (r.consumed && since >= OfferGestureGapSec(r.trigger))
            continue;
        kept.push_back(r);
    }
    entries_.swap(kept);
    return expired;
}

void OfferRequestQueue::Clear()
{
    entries_.clear();
}

const char* RequestedPlanName(RequestedOfferPlan plan)
{
    switch (plan)
    {
    case RequestedPlan_WalkUp:  return "walk up and offer";
    case RequestedPlan_Calm:    return "calm in place";
    case RequestedPlan_Nothing: return "nothing (not hostile)";
    }
    return "unknown";
}

RequestedOfferPlan PlanRequestedOffer(OfferRequestVerdict verdict, bool joinsFree, bool hostile)
{
    if (verdict == OfferVerdict_Join || (verdict == OfferVerdict_Roll && joinsFree))
        return RequestedPlan_WalkUp;
    return hostile ? RequestedPlan_Calm : RequestedPlan_Nothing;
}

bool JoinsForFree(bool willJoin, Currency currency, int cats, bool temporary)
{
    if (!willJoin || temporary || currency == Currency_Refused)
        return false;
    return currency != Currency_Cats || cats <= 0;
}

bool RequestSkipConsumes(CandidateSkip skip)
{
    switch (skip)
    {
    case CandidateSkip_Disabled:
    case CandidateSkip_Mine:
    case CandidateSkip_Excluded:
    case CandidateSkip_Caged:
        return true;
    default:
        return false;
    }
}
} // namespace totalrecruit
