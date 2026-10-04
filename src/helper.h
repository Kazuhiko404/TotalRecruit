#pragma once

// The free helper, engine side: a much stronger ally walks up, offers to escort the
// player for a few days for free, and -- accepted -- follows and fights for the
// player without ever taking an order.
//
// User request (2026-10-03): "There should be a chance that higher-level allied NPCs
// with good personalities offer to help you for some days, but you can't control
// them -- similar to how Kenshi's mercenaries work, but free."
//
// Mechanism: the native contract (A) first, the code-driven escort (B) as fallback.
// The helper stays in its own faction (the player cannot select-and-order it as a
// member). With helperNativeBodyguard on (default), accepting gives its platoon the
// engine's own Bodyguard contract (ai_packages.h AssignBodyguard: package
// 5090-gamedata.base, the squad leader as employer, the hours left), exactly what a
// hired mercenary's dialogue sets; a helper standing in a squad of others is first
// moved to a platoon of its own. Every watch checks the contract is still there
// (helper_rules.h NativeWatchStep): gone, it is assigned again once; gone again, or
// never assigned, or the option off, B takes over. Every end removes the contract
// and resets the platoon's packages to its template (ai_packages.h EndContract).
// B, the fallback: every kHelperEscortRepathMs it keeps to its spot by the leader
// (PlanEscort / EscortOffsetMeters: walk when near, run when far) and, while the
// leader fights, attacks the leader's target with TotalSparring's fight_starter
// sequence (npc_motion.h EngageFor). The rest of this module:
//
//   - The offer (once per in-game day per eligible ally, helper_rules.h
//     ScreenHelperCandidate + HelperOfferRoll; within kHelperTriggerMeters of the
//     leader; every kOfferSweepMs). The walk-up is the submission event's
//     (approach_driver.h), on the same one-event slot, so the two never overlap. On
//     the way: "Quiere ofrecerte ayuda" over its head; on arrival the popup
//     (offer_view.h BuildHelperOfferView): ACEPTAR / RECHAZAR. RECHAZAR (and the
//     squad turning on a helper) starts helperRejectCooldownDays of no offers, saved
//     with the character (total_recruit.helper_reject_until). The title-bar X is no
//     answer: it walks off and does not ask again that day.
//   - The contract (every kHelperEscortRepathMs): expiry hour saved with the
//     character (total_recruit.helper_expiry_hour); helper_rules.h StepHelper decides
//     the warning and the end. Each tick: the temporary-enemy marks between it and the
//     squad are cleared (pacify.h SafeClearTempEnemies), then the native watch or the
//     fallback escort above. The fallback logs the task it actually runs whenever it
//     changes.
//   - The end: time up ("Hasta aqui llego"), attacked or knocked out by the squad
//     ("No vuelvo a ayudarte", plus the reject cooldown), dead or recruited (quiet).
//     The native contract is ended and its packages reset; its orders are cleared and
//     it goes back to its routine.
//   - After a load: the contract comes back with the character's save; the first
//     tick that finds it within kHelperRearmMeters of the leader re-arms the escort
//     ("Sigo contigo"), and the next watch checks the native contract is still held.
//   - The card: a selected helper shows "Ayudante: le quedan N dias" (readout.cpp).
//   - Debug: helperDebugKey (a Windows virtual-key code; 0 = off). Pressed with an
//     NPC selected, that NPC offers at once, past the roll, the level and the
//     personality (the screen's verdict is logged); pressed on someone already
//     helping, it logs the native contract's state and package and assigns it when
//     missing. Every step logs as "[helper]".

#include <string>

class Character;

namespace totalrecruit
{
// Called every frame from frame_tick.
void HelperTick();

// True while this NPC's helper offer is the popup on screen (offer_popup.cpp routes
// its buttons to the answers below).
bool IsHelperOfferPending(Character* npc);
// The popup closed, by any path: clears the on-screen mark only.
void ForgetHelperOffer();
// The popup's answers; each acts only for the NPC of the offer in progress.
void AcceptHelperOffer(Character* npc);
void RejectHelperOffer(Character* npc);
// The title-bar X: no answer.
void DismissHelperOffer(Character* npc);

// Whether `npc` is helping the player now, and the whole days left.
bool HelperDaysLeft(Character* npc, int* daysLeft);

// Load begin: the offer in progress, every live pointer and the table go.
void ForgetHelpers();

// The save bridges (saved_ints.cpp). Thread-safe.
int SavedHelperExpiryHour(const std::string& persistedId);
void RestoreSavedHelperExpiryHour(const std::string& persistedId, int expiryHour);
int SavedHelperRejectUntil(const std::string& persistedId);
void RestoreSavedHelperRejectUntil(const std::string& persistedId, int untilHour);
} // namespace totalrecruit
