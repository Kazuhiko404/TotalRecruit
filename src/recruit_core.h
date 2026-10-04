#pragma once

#include <string>

#include "verdict.h"

class Character;
class RootObject;

namespace totalrecruit
{
// Resolves the character behind a context-menu target, or null when the target
// is not a character (or has already been freed).
Character* CharacterFromTarget(RootObject* target);

// True when the configured exclusions allow recruiting this character.
// outReason receives a short explanation when the answer is false.
bool IsRecruitable(Character* character, std::string* outReason);

// Moves the character into the player's faction and current squad.
bool Recruit(Character* character, std::string* outMessage);

// The full path behind the overlay button: evaluates the rules, charges
// whatever the NPC wants, and either offers the FCS dialogue or recruits
// directly. Reports the outcome to the player either way -- a refusal always
// comes with the reason for it.
void AttemptRecruit(Character* character);

// Shows a line in Kenshi's own message roller.
void TellPlayer(const std::string& message, bool isProblem);

// Takes the agreed cats out of the player's purse, reporting either way.
// Returns false when the money was not there, in which case no recruit should
// follow. A price of zero always succeeds.
bool SettlePrice(Character* character, int cats, const std::string& name);

// Gives back what SettlePrice took, for a purchase that then failed (the engine
// refused the recruit). Without it the player paid and got nobody.
void RefundPrice(Character* character, int cats, const std::string& name);

// Called once the player has answered the offer window. On acceptance the price
// is paid and the recruit goes through; on refusal the NPC says so and nothing
// changes. Money only ever moves here, so walking away costs nothing.
void CompleteOffer(Character* character, bool accepted);

// The player pressed OFRECER: `offer` cats instead of the asking price. One
// attempt, drawn on the same daily seed as the join roll (ResolveHaggle with the
// key and day MintRollKey gives now). Accepted: the offer is charged and they
// join. Refused: they are offended for the rest of the in-game day (no haggling,
// a raised ask) and the window reopens on that. An offer at the ask is
// CompleteOffer.
void CompleteHaggle(Character* character, int offer);

// Acts on what a captive is short of rather than answering their refusal --
// feeding them, for now. Reopens the offer afterwards so the player sees what
// changed without working through the context menu again.
void HelpCaptive(Character* character);

// ------------------------------------------------------------ Duel challenge
// An elite Balanced ally can be challenged to a TotalSparring duel and joins for
// good on a win (duel_challenge_rules.h has the rules, ts_bridge.h the plumbing).

// Whether RETAR A DUELO is on offer for `npc` with this verdict now: CanOfferDuel over
// duelChallengeEnabled, TsAvailable, the tiered-ally verdict, the elite cut level and
// the NPC's cooldown.
bool DuelOfferedFor(Character* npc, const Verdict& verdict);

// TotalSparring's head-to-head record of `challenger` (a squad member) against `npc`.
// Reads its history database: call it on an event only (popup open, selection change).
bool ReadDuelRecord(Character* challenger, Character* npc, int* wins, int* losses, int* draws);

// RETAR A DUELO: the selected squad member (the cage commands' actor rule) challenges
// `npc`; the challenge is recorded and TotalSparring's Dueling window opens. Says why
// to the player when it cannot (nobody selected, TotalSparring busy or not ready).
void ChallengeToDuel(Character* npc);

// The frame tick: attaches to TotalSparring, expires stale challenges and acts on the
// bouts TotalSparring reported (a win recruits for good, a loss starts the cooldown).
void DuelChallengeTick();
} // namespace totalrecruit
