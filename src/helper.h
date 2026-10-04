#pragma once

// The free helper, engine side: a much stronger ally walks up, offers to escort the
// player for a few days for free, and -- accepted -- follows and fights for the
// player without ever taking an order.
//
// User request (2026-10-03): "There should be a chance that higher-level allied NPCs
// with good personalities offer to help you for some days, but you can't control
// them -- similar to how Kenshi's mercenaries work, but free."
//
// Mechanism: code-driven escort (B), not the native "AI contract" (A). The native
// hire is a dialogue line whose AI-contract reference sets the platoon's
// blackboard->contractJob (KEP UniqueCharacterFix.cpp:406-414). Driving it from code
// was ruled out on evidence: AI/Blackboard.h is not in the KenshiLib headers, so the
// contract can neither be read back (did it take?) nor cleared (the squad hit them);
// Dialogue::_doActions and startConversation are private and this mod's own dialogue
// bridge has never fired in game (dialogue_bridge.cpp OpenConversation is disabled
// for that reason); and the one free vanilla line (14191, "Escaping slave- work
// together civilian") carries a fixed duration shared with the escaped-slave quests.
// So the helper stays in its own faction (the player cannot select-and-order it as
// a member) and this module keeps it at the leader's side:
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
//   - The contract (every kEscortTickMs): expiry hour saved with the character
//     (total_recruit.helper_expiry_hour); helper_rules.h StepHelper decides the
//     warning and the end. Each tick: the temporary-enemy marks between it and the
//     squad are cleared (pacify.h SafeClearTempEnemies); while the leader fights, it
//     is ordered onto the leader's own target (npc_motion.h AttackFor,
//     FOCUSED_MELEE_ATTACK); otherwise, out of combat, the escort order is re-issued
//     on the rung of helper_rules.h NextEscortRung -- BODYGUARD, then
//     FOLLOW_PLAYER_ORDER, then the walk-up's own move order -- climbing whenever it
//     falls behind. The task it actually runs is logged whenever it changes, so the
//     log shows which rung the engine honours.
//   - The end: time up ("Hasta aqui llego"), attacked or knocked out by the squad
//     ("No vuelvo a ayudarte", plus the reject cooldown), dead or recruited (quiet).
//     Its orders are cleared and it goes back to its routine.
//   - After a load: the contract comes back with the character's save; the first
//     tick that finds it within kHelperRearmMeters of the leader re-arms the escort
//     ("Sigo contigo").
//   - The card: a selected helper shows "Ayudante: le quedan N dias" (readout.cpp).
//   - Debug: helperDebugKey (a Windows virtual-key code; 0 = off). Pressed with an
//     NPC selected, that NPC offers at once, past the roll, the level and the
//     personality (the screen's verdict is logged). Every step logs as "[helper]".

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
