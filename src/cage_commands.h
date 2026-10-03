#pragma once

// The cage commands, engine side: Golpear and Noquear for a prisoner in the
// player's own cage, done by one of the player's characters with Kenshi's own
// orders, so the outcome (wounds, stress, whether the knockout lands) is the
// engine's and not arithmetic of this mod.
//
//   Golpear  addOrder(BEAT_CAGE_OCCUPANT) + addGoal on the actor. Fallback, when
//            the actor has not changed task within kOrderWatchTimeoutMs:
//            TotalSparring's attack sequence (clear orders/jobs, addOrder
//            FOCUSED_MELEE_ATTACK, attackTarget, attackingYou; not its
//            initCombatMode, see cage_commands.cpp).
//   Noquear  addOrder(STEALTH_KNOCKOUT) + addGoal. Fallback: the engine's own
//            MedicalSystem::knockout on the target, with the actor's
//            assassination skill (0..1, SkillFraction).
//
// checkPlayerOrderForProblems is asked before every order and its answer is
// logged, but it does not decide the fallback: which return value means "a
// problem" is not proven on a running game, and reading it the wrong way round
// would send every order to the fallback. The order watch decides instead -- an
// actor who never changes task did not take the order, whatever the reason.
//
// Credit: the knockout hook (defeats.cpp) counts the defeat. A knockout the
// player ordered here is remembered for kOrderedKnockoutWindowMs, and the hook
// claims it (ClaimOrderedKnockoutCredit), so it counts as the player's even when
// the engine's lastGuyWhoDefeatedMe names nobody -- the fallback knockout is
// exactly such a case. The pure parts live in cage_command_rules.h.
//
// Log: "[cage] <actor> -> <target> beat|knockout via task|fallback" (probeOnClick),
// failures always.
//
// This module never tells the player anything itself: the callers (the context
// menu buttons, the popup's GOLPEARLO remedy) do, with CageCommandProblem. That
// keeps it below recruit_core instead of in a ring with it.

#include <string>

class Character;

namespace totalrecruit
{
enum CageCommandResult
{
    CageCommand_Issued,    // the native order went in (the watch may still fall back)
    CageCommand_NoActor,   // nobody of the player's is selected and awake
    CageCommand_Failed     // the order could not be given at all (logged)
};

// Whether Golpear and Noquear apply to this character now: in the player's cage,
// awake and alive, and submission.allowBeating on (CageCommandsAllowed).
bool CageCommandsAvailable(Character* target);

// The player's selected character, awake and not the target. NULL when nobody
// usable is selected.
Character* SelectedSquadActor(Character* target);

// Orders `actor` to beat / knock out `target` and starts the watch that runs the
// fallback from TickCageCommands. False when the order could not be given at all.
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

// The frame tick: watches the last native order and runs its fallback when the
// actor never took it.
void TickCageCommands();

// For the knockout hook: true (once) when this knockout is one the player ordered
// through a cage command a moment ago.
bool ClaimOrderedKnockoutCredit(Character* victim);

// Load boundary: drops the watched order (raw engine pointers) and the pending
// credits.
void ForgetCageCommands();
} // namespace totalrecruit
