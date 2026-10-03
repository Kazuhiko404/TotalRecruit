#pragma once

// The submission event, engine side: a beaten NPC that yields walks up to the
// player and asks to join.
//
// User request (2026-10-03): "When an NPC is submitted it should walk up to me
// automatically and ask to join. If it is hostile, make it neutral for 1 minute.
// Three options: Accept, Forgive, Reject." The event system is TotalSparring's
// NPC challenge (src/duel/challenge/challenge_director.cpp), ported: one event
// at a time, a throttled sweep picks the NPC, a paused-aware clock drives the
// walk-up, the hold and the forgive window, and pure steps
// (submission_rules.h, approach_rules.h) decide each frame.
//
//   Sweep (every kSubmissionSweepMs, nothing running, no popup or plea open):
//     characters within kSubmissionTriggerMeters of the squad leader that are
//     not the player's, not animals, up, not caged, recruitable, not on the
//     reject cooldown, not asked since their last defeat, and beaten (defeats
//     at the break count, or yielded to the player). Then a peek at the verdict
//     (EvaluatePeek: no id, no clock, no roll); an open field yield is decided
//     once with the key (Evaluate), and a failed roll is not drawn again that day.
//   Approaching: calm re-asserted once a second on that NPC only (pacify.h
//     SafeCalmToward), the move order re-issued to the leader once a second
//     (npc_motion.h). Arrives at kSubmissionArriveMeters; the approach window
//     (submissionApproachSeconds, real seconds frozen on pause) is the timeout.
//   Holding: the NPC stands facing the leader and the popup is up
//     (BuildSubmissionOfferView): ACEPTAR / PERDONAR / RECHAZAR.
//   Accept: Recruit. Forgive: runs kSubmissionFleeMeters away and stays calm
//     for submissionForgiveSeconds (the player can still talk to it), then goes
//     back to what it was. Reject: its defeats are wiped, it refuses every
//     recruit route for submissionRejectCooldownHours (saved with it), and it is
//     hostile to the player leader again.
//   The popup closed with no answer (X, hidden, replaced): treated like a
//     forgive without the flight -- calm in place for the forgive window, then
//     back to what it was.
//   Abort (gone, down, calm broken, too far, timeout): orders cleared, pace
//     given back, and a hostile NPC is hostile again.
//
// Never changes a faction relation.

class Character;

namespace totalrecruit
{
// Called every frame from frame_tick.
void SubmissionTick();

// True while this NPC's submission popup is the one on screen, so the offer
// window routes its buttons to the answers below (offer_popup.cpp).
bool IsSubmissionPending(Character* npc);

// The popup closed, by any path. Clears the on-screen mark only; the next tick
// treats an unanswered close as Dismiss.
void ForgetSubmissionOffer();

// The popup's answers. Each acts only for the NPC of the event in progress.
void AcceptSubmission(Character* npc);
void ForgiveSubmission(Character* npc);
void RejectSubmission(Character* npc);
// The title-bar X: no decision (see the header comment).
void DismissSubmission(Character* npc);

// Load begin: gives the NPC of an event in progress its pace and routine back,
// and drops the event, the clock and the sweep timer.
void ForgetSubmission();
} // namespace totalrecruit
