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
// Rest (R2.3): after a blow that landed, Golpear rests on that prisoner for
// submission.beatingCooldownHours game hours; after a knockout order that put the
// prisoner down (the hook claimed its credit, ResolveCageKnockout), Noquear rests
// for submission.knockoutCooldownHours. One CageCooldownLedger per command,
// session-only like the defeat episodes. The buttons stay; each caption carries
// the wait ("Golpear (1 h)", RestLabel; "Noquear (1 h)", KnockoutCaption) and a
// click only says so.
//
// Every click re-reads the facts the button was shown for (JudgeCageClick): a
// block or a popup built a while ago never acts on a prisoner freed, knocked out
// or out of the cage since.
//
// Credit: a blow that landed is remembered for kBeatCreditWindowMs (a few seconds
// around the hit: its knockout comes at once or not at all), a knockout order for
// kOrderedKnockoutWindowMs (it walks to the cage first), and the knockout hook
// (defeats.cpp) claims it (ResolveCageKnockout), which identifies the knockout as
// the cage command's even when the engine's lastGuyWhoDefeatedMe names nobody --
// the direct blow and the fallback knockout are exactly such cases. A cage
// command's knockout is NOT a defeat (cage-beating-stress): it is beating stress
// (beating_stress.h). Each blow that landed adds its stress at the hit; a knockout
// order's knockout adds its own when it is claimed. A later knockout (a starvation
// collapse) is no cage command's, and the ordinary defeat rules decide it. The
// pure parts live in cage_command_rules.h.
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
    CageCommand_Failed,    // the blow or the order could not be given at all (logged)
    CageCommand_NotAllowed, // at the click the commands no longer apply (freed, down,
                            // out of the cage, beating off): nothing was done
    CageCommand_BeatResting,    // Golpear still inside the interval between beatings
    CageCommand_KnockoutResting // Noquear still inside the interval between knockouts
};

// The facts CageCommandsAllowed judges, read for this character now: in the
// player's cage, down (unconscious or dead), submission.allowBeating, and the game
// hours Golpear and Noquear still rest on them. The popup (offer_view.h OfferInput::cage),
// the context menu and every click read the same facts.
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

// The same with the selected character as the actor (SelectedSquadActor), after
// re-reading the facts at the click (NotAllowed / *Resting: nothing is done).
CageCommandResult OrderSelectedToBeat(Character* target);
CageCommandResult OrderSelectedToKnockout(Character* target);

// The line the caller tells the player for a result that is not Issued
// (messages.json "cage.noActor" / "cage.failed" / "cage.notAllowed" /
// "cage.beatResting" / "cage.knockoutResting", the last two with the hours left on
// `target`); empty for Issued.
std::string CageCommandProblem(CageCommandResult result, Character* target);

// Button captions (messages.json "cage.button.*"). Golpear carries the hours it
// still rests on `target` ("Golpear (1 h)", RestLabel). The knockout one carries
// the hours it still rests ("Noquear (1 h)"), else the engine's own odds for the
// selected characters (PlayerInterface::getPlayerTaskProbability,
// STEALTH_KNOCKOUT): "Noquear 72%", the plain label when the engine gives none
// (KnockoutCaption).
std::string BeatButtonLabel(Character* target);
std::string KnockoutButtonLabel(Character* target);

// The frame tick: watches the last knockout order (fallback when the actor never
// took it, cleanup once the target is down or the window passes) and, every
// kStrayGoalSweepMs, heals actors stuck with a stray knockout goal.
void TickCageCommands();

// What the knockout hook learns from ResolveCageKnockout: which cage command (if
// any) caused this knockout, and the beating stress the knockout itself added.
struct CageKnockout
{
    OrderedCreditKind kind;
    float stressGain;   // 0..1; 0 for a blow's knockout (the blow added its own)

    CageKnockout() : kind(OrderedCredit_None), stressGain(0.0f) {}
};

// For the knockout hook: which cage command (once) this knockout follows, given a
// moment ago -- a blow or a knockout order -- or OrderedCredit_None. A knockout
// order's knockout is Noquear landing: its rest starts here and it adds its
// beating stress (CageKnockoutAddsStress).
CageKnockout ResolveCageKnockout(Character* victim);

// Load boundary: lets the watched actor go (CleanUpKnockoutOrder, while the
// characters still exist and still resolve to their keys), drops the watch and the
// pending credits.
void ForgetCageCommands();
} // namespace totalrecruit
