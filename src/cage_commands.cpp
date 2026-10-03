#include "common.h"
#include "cage_commands.h"
#include "cage_command_rules.h"
#include "char_key_reader.h"
#include "config.h"
#include "engine_reads.h"
#include "log.h"
#include "messages.h"

// OrdersReceiver (clearOrders / clearJobs). It defines taskPriority, as Tasker.h
// does, so the actor's current task is read in engine_reads.cpp instead.
#include <kenshi/AI/AITaskSystem.h>
#include <kenshi/CharStats.h>

namespace totalrecruit
{
namespace
{
// Engine pointers can already be freed, so every dereference is guarded, and
// each guard keeps a POD-only frame -- C2712 counts every C++ object in the
// whole function, not just those inside the __try block.

enum CageCommand
{
    Command_Beat,
    Command_Knockout
};

// The one native order being watched: the last one the player gave. A second
// command replaces it -- the player changed their mind, and the actor's task is
// then about the new one. Raw pointers, only read behind guards and dropped at a
// load (ForgetCageCommands).
struct OrderWatch
{
    bool active;
    CageCommand command;
    Character* actor;
    Character* target;
    int ordered;
    int taskBefore;
    DWORD startedMs;

    OrderWatch()
        : active(false), command(Command_Beat), actor(NULL), target(NULL),
          ordered(0), taskBefore(0), startedMs(0)
    {
    }
};

OrderWatch g_watch;
OrderedKnockoutCredit g_credit;

const char* VerbOf(CageCommand command)
{
    return command == Command_Beat ? "beat" : "knockout";
}

TaskType TaskOf(CageCommand command)
{
    return command == Command_Beat ? BEAT_CAGE_OCCUPANT : STEALTH_KNOCKOUT;
}

bool SafePosition(Character* character, Ogre::Vector3* out)
{
    TR_TRY
    {
        *out = character->getPosition();
        return true;
    }
    TR_CATCH { return false; }
}

// Asked the way the player's own order path asks; logged, never trusted (see the
// header: its polarity is unproven).
bool SafeOrderProblems(Character* actor, TaskType task, Character* target, bool* out)
{
    TR_TRY
    {
        *out = actor->checkPlayerOrderForProblems(task, target);
        return true;
    }
    TR_CATCH { return false; }
}

// TotalPredator's dispatch (predator_logic.cpp InternalExecuteEatCorpse): the
// order clears what the actor was doing, the goal makes the AI pursue it.
bool SafeOrder(Character* actor, TaskType task, Character* target, const Ogre::Vector3& where)
{
    TR_TRY
    {
        actor->addOrder(NULL, task, target, false, true, where);
        actor->addGoal(task, target);
        return true;
    }
    TR_CATCH { return false; }
}

// --- the beating fallback: TotalSparring's fight_starter, step by step --------
//
// Without its last step, initCombatMode: CombatClass.h includes "Enums.h"
// relative to the kenshi folder, which this project's include path does not
// list, and the method is virtual, so it cannot be declared locally (notes
// section 11). TotalSparring's own comment calls the addOrder the essential part.

bool SafeDropCurrentActivity(Character* actor)
{
    TR_TRY
    {
        OrdersReceiver* orders = actor->getOrdersReciever();
        if (orders != NULL)
        {
            orders->clearOrders();
            orders->clearJobs();
        }
        return true;
    }
    TR_CATCH { return false; }
}

// The order's location is the attacker's own position (Sparring reference parity).
bool SafeAttackOrder(Character* actor, Character* target, const Ogre::Vector3& where)
{
    TR_TRY
    {
        actor->addOrder(NULL, FOCUSED_MELEE_ATTACK, target, false, true, where);
        actor->attackTarget(target);
        target->attackingYou(actor, true, true);
        return true;
    }
    TR_CATCH { return false; }
}

bool BeatFallback(Character* actor, Character* target)
{
    Ogre::Vector3 where;
    if (!SafePosition(actor, &where))
    {
        return false;
    }
    SafeDropCurrentActivity(actor);
    if (!SafeAttackOrder(actor, target, where))
    {
        return false;
    }
    return true;
}

// --- the knockout fallback: the engine's own knockout -------------------------

bool SafeAssassinationLevel(Character* actor, float* out)
{
    TR_TRY
    {
        CharStats* stats = actor->getStats();
        if (stats == NULL)
        {
            return false;
        }
        *out = stats->getStat(STAT_ASSASSINATION, false);
        return true;
    }
    TR_CATCH { return false; }
}

bool SafeKnockout(Character* target, float skill01)
{
    TR_TRY
    {
        MedicalSystem* medical = target->getMedical();
        if (medical == NULL)
        {
            return false;
        }
        medical->knockout(skill01);
        return true;
    }
    TR_CATCH { return false; }
}

bool KnockoutFallback(Character* actor, Character* target)
{
    // Already out (the order landed after all, or someone else got there): a
    // second knockout would only extend it.
    bool down = false;
    if (SafeIsKnockedOut(target, &down) && down)
    {
        return true;
    }
    float level = 0.0f;
    SafeAssassinationLevel(actor, &level);
    return SafeKnockout(target, SkillFraction(level));
}

// --- logging -----------------------------------------------------------------

void LogResolved(const OrderWatch& watch, const char* via)
{
    if (GetConfig().probeOnClick)
    {
        LogInfo("[cage] " + CharacterName(watch.actor) + " -> " + CharacterName(watch.target) +
                " " + VerbOf(watch.command) + " via " + via);
    }
}

void LogFailure(Character* actor, Character* target, CageCommand command, const char* what)
{
    LogError("[cage] " + CharacterName(actor) + " -> " + CharacterName(target) + " " +
             VerbOf(command) + " failed: " + what);
}

void RememberCredit(Character* target)
{
    CharKey key;
    if (SafeCharKey(target, &key))
    {
        g_credit.Remember(key, GetTickCount());
    }
}

bool Issue(CageCommand command, Character* actor, Character* target)
{
    if (actor == NULL || target == NULL)
    {
        return false;
    }

    Ogre::Vector3 where;
    if (!SafePosition(target, &where))
    {
        LogFailure(actor, target, command, "target position unreadable");
        return false;
    }

    const TaskType task = TaskOf(command);
    int taskBefore = (int)NULL_TASK;
    SafeCurrentTask(actor, &taskBefore);

    bool problems = false;
    const bool askedProblems = SafeOrderProblems(actor, task, target, &problems);

    if (!SafeOrder(actor, task, target, where))
    {
        LogFailure(actor, target, command, "addOrder raised");
        return false;
    }

    if (GetConfig().probeOnClick)
    {
        char line[96];
        _snprintf_s(line, sizeof(line), _TRUNCATE,
                    " ordered (task %d, was %d, checkPlayerOrderForProblems=%s)",
                    (int)task, taskBefore,
                    askedProblems ? (problems ? "true" : "false") : "unreadable");
        LogInfo("[cage] " + CharacterName(actor) + " -> " + CharacterName(target) + " " +
                VerbOf(command) + line);
    }

    RememberCredit(target);

    g_watch.active = true;
    g_watch.command = command;
    g_watch.actor = actor;
    g_watch.target = target;
    g_watch.ordered = (int)task;
    g_watch.taskBefore = taskBefore;
    g_watch.startedMs = GetTickCount();
    return true;
}

void RunFallback(const OrderWatch& watch)
{
    const bool ok = (watch.command == Command_Beat) ? BeatFallback(watch.actor, watch.target)
                                                    : KnockoutFallback(watch.actor, watch.target);
    if (!ok)
    {
        LogFailure(watch.actor, watch.target, watch.command, "fallback raised");
        return;
    }
    LogResolved(watch, "fallback");
}

CageCommandResult OrderSelected(CageCommand command, Character* target)
{
    Character* actor = SelectedSquadActor(target);
    if (actor == NULL)
    {
        return CageCommand_NoActor;
    }
    return Issue(command, actor, target) ? CageCommand_Issued : CageCommand_Failed;
}

bool SafeSelectedCharacter(Character** out)
{
    TR_TRY
    {
        *out = NULL;
        if (ou == NULL || ou->player == NULL)
        {
            return false;
        }
        *out = ou->player->selectedCharacter.getCharacter();
        return true;
    }
    TR_CATCH { return false; }
}

bool SafeKnockoutOdds(Character* target, float* out)
{
    TR_TRY
    {
        if (ou == NULL || ou->player == NULL)
        {
            return false;
        }
        return ou->player->getPlayerTaskProbability(STEALTH_KNOCKOUT, target, *out);
    }
    TR_CATCH { return false; }
}
} // namespace

CageCommandFacts ReadCageCommandFacts(Character* target)
{
    CageCommandFacts facts;
    facts.allowBeating = GetConfig().submission.allowBeating;
    if (target == NULL)
    {
        facts.inPlayerCage = false;
        return facts;
    }
    facts.inPlayerCage = IsInPlayerCage(target);
    bool down = false;
    bool dead = false;
    facts.down = (SafeIsDown(target, &down) && down) || (SafeIsDead(target, &dead) && dead);
    return facts;
}

bool CageCommandsAvailable(Character* target)
{
    return CageCommandsAllowed(ReadCageCommandFacts(target));
}

bool ReadKnockoutOdds(Character* target, float* out)
{
    return target != NULL && out != NULL && SafeKnockoutOdds(target, out);
}

Character* SelectedSquadActor(Character* target)
{
    Character* actor = NULL;
    if (!SafeSelectedCharacter(&actor) || actor == NULL || actor == target)
    {
        return NULL;
    }
    bool mine = false;
    if (!SafeIsPlayerCharacter(actor, &mine) || !mine)
    {
        return NULL;
    }
    bool down = false;
    if (!SafeIsKnockedOut(actor, &down) || down)
    {
        return NULL;
    }
    return actor;
}

bool IssueBeat(Character* actor, Character* target)
{
    return Issue(Command_Beat, actor, target);
}

bool IssueKnockout(Character* actor, Character* target)
{
    return Issue(Command_Knockout, actor, target);
}

CageCommandResult OrderSelectedToBeat(Character* target)
{
    return OrderSelected(Command_Beat, target);
}

CageCommandResult OrderSelectedToKnockout(Character* target)
{
    return OrderSelected(Command_Knockout, target);
}

std::string CageCommandProblem(CageCommandResult result)
{
    switch (result)
    {
    case CageCommand_NoActor:
        return Say("cage.noActor", "Selecciona a alguien de tu grupo para hacerlo", NULL);
    case CageCommand_Failed:
        return Say("cage.failed", "No se pudo dar la orden", NULL);
    case CageCommand_Issued:
    default:
        return std::string();
    }
}

std::string BeatButtonLabel()
{
    return Say("cage.button.beat", "Golpear", NULL);
}

std::string KnockoutButtonLabel(Character* target)
{
    float probability = 0.0f;
    const bool haveOdds = ReadKnockoutOdds(target, &probability);
    return KnockoutLabel(Say("cage.button.knockout", "Noquear", NULL), haveOdds, probability);
}

void TickCageCommands()
{
    if (!g_watch.active)
    {
        return;
    }

    int taskNow = (int)NULL_TASK;
    const bool readNow = SafeCurrentTask(g_watch.actor, &taskNow);
    const OrderWatchVerdict verdict =
        JudgeOrderWatch(g_watch.taskBefore, g_watch.ordered, readNow, taskNow,
                        GetTickCount() - g_watch.startedMs, kOrderWatchTimeoutMs);
    if (verdict == OrderWatch_Pending)
    {
        return;
    }

    // Copied out and closed before acting: the fallback must run once.
    const OrderWatch watch = g_watch;
    g_watch = OrderWatch();

    if (verdict == OrderWatch_Taken)
    {
        LogResolved(watch, "task");
        return;
    }
    RunFallback(watch);
}

bool ClaimOrderedKnockoutCredit(Character* victim)
{
    // Cheapest first: almost every knockout in the world was not ordered here.
    if (victim == NULL || g_credit.Size() == 0)
    {
        return false;
    }
    CharKey key;
    if (!SafeFindCharKey(victim, &key))
    {
        return false;
    }
    return g_credit.Claim(key, GetTickCount());
}

void ForgetCageCommands()
{
    g_watch = OrderWatch();
    g_credit.Clear();
}
} // namespace totalrecruit
