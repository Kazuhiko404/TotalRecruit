#pragma once

// The proactive offer, engine side: the gestures of the player that ask the submission
// event (submission_director.h) for an offer, and the queue the event's sweep serves.
//
// User request (2026-10-05, proactive-offer): "agregar un evento que el npc o animal abra
// el popup de que quiere unirse, no tener yo que hablar de nuevo, porque despues de
// ganarle en duelo varias veces o curar [...] sigue igual con agro y me ataca".
//
//   A won duel (recruit_core.cpp OnDuelWon, offerOnDuelWin): Join when the duel join
//     roll passed (Cheat always), Calm when it failed (Balanced).
//   A heal (rescue.cpp first aid / doctoring hooks, offerOnHeal): a member of the
//     player's squad treats someone who is not; Roll -- the verdict decides when the
//     patient is up again.
//   Hablar by hand (recruit_core.cpp AttemptRecruit) uses a pending request up: the
//     player is already talking.
//
// The pure queue (one request per gesture, expiry) is offer_request_rules.h; this file
// keeps it on a paused-aware clock, reads the keys and logs every step as "[offer] ...".

#include "offer_request_rules.h"

class Character;
class MedicalSystem;

namespace totalrecruit
{
// A gesture of the player asks for an offer from `npc`.
void RequestOffer(Character* npc, OfferTrigger trigger, OfferRequestVerdict verdict);

// The heal hooks: `healer` treats the patient of `medical`. Requests a Roll offer when
// offerOnHeal is on, the healer is the player's and the patient is not.
void NoteHealForOffer(MedicalSystem* medical, Character* healer);

// Whether any request waits to be served.
bool HasRequestedOffers();

// The pending request for `npc` (read key: no id minted).
bool FindRequestedOffer(Character* npc, OfferRequest* out);

// Uses up the pending request for `npc`, if any; `why` goes to the log.
void ConsumeRequestedOffer(Character* npc, const char* why);

// Whether `npc` stands within the event's reach of the squad leader
// (offer_request_rules.h InOfferReach, kOfferReachMeters): out of it, a won duel
// keeps the old behaviour.
bool WithinOfferReach(Character* npc);

// Every frame (from SubmissionTick): the clock, and requests nobody served expire.
void OfferRequestsTick();

// Load begin: every request and the clock are dropped.
void ForgetOfferRequests();
} // namespace totalrecruit
