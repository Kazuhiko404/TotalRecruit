#pragma once

// The cage commands, engine side: Golpear and Noquear for a prisoner in the
// player's own cage, done by one of the player's characters.
//
//   Golpear  one direct blow, at once and with no order at all: the actor's own
//            attack damage (CharStats::getTotalAttackDamageFor) landed as blunt
//            on the prisoner's torso (HealthPartStatus::applyDamage), capped so
//            the part never drops below -0.5 x its max health (BeatBlunt,
//            CapBeatBlunt, PickBeatPart in cage_command_rules.h). It may knock
//            the prisoner out; it never kills, and the prisoner stays in the
//            cage. The native BEAT_CAGE_OCCUPANT order was never taken from the
//            plugin, and its attack fallback started a real fight that pulled
//            the prisoner out of the cage, so neither is used.
//   Noquear  addOrder(STEALTH_KNOCKOUT) + addGoal, so whether the knockout lands
//            is the engine's. Fallback, when the actor has not changed task
//            within kOrderWatchTimeoutMs (watch time: WatchFrameMs stops it while
//            the game is paused, like the land window): the engine's own
//            MedicalSystem::knockout on the target, with the actor's
//            assassination skill (0..1, SkillFraction).
//            It acts once: the goal is persistent, and left in place the actor's
//            AI knocked the prisoner out again every time he stood up. The watch
//            stays on after the order is taken, until the target is down (or
//            dead), the actor abandons the order (kKnockoutAbandonMs off it in a
//            row) or kKnockoutLandWindowMs passes (NextKnockoutWatchStep in
//            knockout_watch_rules.h), and every way it ends -- down, abandoned,
//            window, fallback, a newer order, a load -- lets the actor go
//            (CleanUpKnockoutOrder in knockout_order_cleanup.h: goal removed,
//            the order dropped only when it is ours and alone in the queue,
//            sneaking off only if the order turned it on, the AI re-thinks). The
//            frame tick also sweeps the player's characters for a stray knockout
//            goal no watch owns (an older save) and removes the goal -- never an
//            order -- when its target is a prisoner in the player's cage.
//
// checkPlayerOrderForProblems is asked before the knockout order and its answer
// is logged, but it does not decide the fallback: which return value means "a
// problem" is not proven on a running game, and reading it the wrong way round
// would send every order to the fallback. The order watch decides instead -- an
// actor who never changes task did not take the order, whatever the reason.
//
// Credit: the knockout hook (defeats.cpp) counts the defeat. A blow or a
// knockout the player gave here is remembered for kOrderedKnockoutWindowMs, and
// the hook claims it (ClaimOrderedKnockoutCredit), so a knockout it causes counts
// as the player's even when the engine's lastGuyWhoDefeatedMe names nobody --
// the direct blow and the fallback knockout are exactly such cases. The pure
// parts live in cage_command_rules.h.
//
// Log (probeOnClick): "[cage] <actor> -> <target> beat hit (blunt X, part health
// A -> B)", "[cage] <actor> -> <target> knockout via task|fallback" and "[cage]
// <actor> knockout cleanup (goal removed, stealth off|kept) after <why>";
// failures always.
//
// This module never tells the player anything itself: the callers (the context
// menu buttons, the popup's GOLPEAR / NOQUEAR buttons) do, with
// CageCommandProblem. That keeps it below recruit_core instead of in a ring with it.

#include <string>

#include "cage_command_rules.h"

class Character;

namespace totalrecruit
{
enum CageCommandResult
{
    CageCommand_Issued,    // the blow landed, or the knockout order went in (the
                           // watch may still fall back)
    CageCommand_NoActor,   // nobody of the player's is selected and awake
    CageCommand_Failed     // the blow or the order could not be given at all (logged)
};

// The facts CageCommandsAllowed judges, read for this character now: in the
// player's cage, down (unconscious or dead), submission.allowBeating. The popup
// (offer_view.h OfferInput::cage) and the context menu read the same facts.
CageCommandFacts ReadCageCommandFacts(Character* target);

// Whether Golpear and Noquear apply to this character now: in the player's cage,
// awake and alive, and submission.allowBeating on (CageCommandsAllowed).
bool CageCommandsAvailable(Character* target);

// The engine's knockout odds for the selected characters against `target`
// (PlayerInterface::getPlayerTaskProbability, STEALTH_KNOCKOUT), as the engine
// reports them (KnockoutPercent reads either scale). False when it gives none.
bool ReadKnockoutOdds(Character* target, float* out);

// The player's selected character, awake and not the target. NULL when nobody
// usable is selected.
Character* SelectedSquadActor(Character* target);

// IssueBeat: `actor` lands one capped blow on `target` now (no order, no watch).
// IssueKnockout: orders the knockout and starts the watch (TickCageCommands runs
// the fallback and the cleanup); a watch already running is ended and its actor
// cleaned up first. False when the blow or the order could not be given.
bool IssueBeat(Character* actor, Character* target);
bool IssueKnockout(Character* actor, Character* target);

// The same with the selected character as the actor (SelectedSquadActor).
CageCommandResult OrderSelectedToBeat(Character* target);
CageCommandResult OrderSelectedToKnockout(Character* target);

// The line the caller tells the player for a result that is not Issued
// (messages.json "cage.noActor" / "cage.failed"); empty for Issued.
std::string CageCommandProblem(CageCommandResult result);

// Button captions (messages.json "cage.button.*"). The knockout one carries the
// engine's own odds for the selected characters
// (PlayerInterface::getPlayerTaskProbability, STEALTH_KNOCKOUT): "Noquear 72%",
// the plain label when the engine gives none.
std::string BeatButtonLabel();
std::string KnockoutButtonLabel(Character* target);

// The frame tick: watches the last knockout order (fallback when the actor never
// took it, cleanup once the target is down or the window passes) and, every
// kStrayGoalSweepMs, heals actors stuck with a stray knockout goal.
void TickCageCommands();

// For the knockout hook: true (once) when this knockout follows a cage command
// the player gave a moment ago (a blow or a knockout order).
bool ClaimOrderedKnockoutCredit(Character* victim);

// Load boundary: lets the watched actor go (CleanUpKnockoutOrder, while the
// characters still exist and still resolve to their keys), drops the watch and the
// pending credits.
void ForgetCageCommands();
} // namespace totalrecruit
