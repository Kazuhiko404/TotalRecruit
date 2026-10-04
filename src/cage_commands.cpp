#include "common.h"
#include "cage_commands.h"
#include "cage_command_rules.h"
#include "char_key_reader.h"
#include "config.h"
#include "engine_reads.h"
#include "log.h"
#include "messages.h"

#include <kenshi/CharStats.h>
#include <kenshi/Damages.h>

namespace totalrecruit
{
namespace
{
// Engine pointers can already be freed, so every dereference is guarded, and
// each guard keeps a POD-only frame -- C2712 counts every C++ object in the
// whole function, not just those inside the __try block.

typedef MedicalSystem::HealthPartStatus HealthPart;

// --- Golpear: one direct, capped blow ----------------------------------------
//
// No orders at all: the native BEAT_CAGE_OCCUPANT order was never taken from a
// plugin, and the attack fallback started a real fight that pulled the prisoner
// out of the cage. The blow is the actor's own attack damage, landed as blunt on
// the prisoner's torso and capped by the pure rules (cage_command_rules.h).

const int kBeatPartScan = 32;

struct AttackNumbers
{
    float cut;
    float blunt;
    float pierce;
};

// Unguarded: getTotalAttackDamageFor returns a Damages, a C++ object (C2712).
// SafeAttackDamage holds the guard.
bool ReadAttackDamageUnguarded(Character* actor, Character* target, AttackNumbers* out)
{
    CharStats* stats = actor->getStats();
    if (stats == NULL)
    {
        return false;
    }
    const Damages damage = stats->getTotalAttackDamageFor(target);
    out->cut = damage.cut;
    out->blunt = damage.blunt;
    out->pierce = damage.pierce;
    return true;
}

bool SafeAttackDamage(Character* actor, Character* target, AttackNumbers* out)
{
    TR_TRY { return ReadAttackDamageUnguarded(actor, target, out); }
    TR_CATCH { return false; }
}

// The torso part the blow lands on (PickBeatPart over the anatomy).
bool SafeBeatPart(Character* target, HealthPart** out)
{
    TR_TRY
    {
        MedicalSystem* medical = target->getMedical();
        if (medical == NULL)
        {
            return false;
        }
        BeatPartFacts facts[kBeatPartScan];
        int count = (int)medical->anatomy.size();
        if (count > kBeatPartScan)
        {
            count = kBeatPartScan;
        }
        for (int i = 0; i < count; ++i)
        {
            HealthPart* part = medical->anatomy[(uint32_t)i];
            facts[i].present = part != NULL;
            facts[i].type = part != NULL ? (int)part->whatAmI : -1;
            facts[i].fatal = part != NULL && part->fatal;
        }
        const int index = PickBeatPart(facts, count);
        if (index < 0)
        {
            return false;
        }
        *out = medical->anatomy[(uint32_t)index];
        return true;
    }
    TR_CATCH { return false; }
}

bool SafePartHealth(HealthPart* part, float* health, float* maxHealth)
{
    TR_TRY
    {
        *health = part->flesh;
        *maxHealth = part->maxHealth();
        return true;
    }
    TR_CATCH { return false; }
}

// The Damages is built by the caller: this guarded frame holds no C++ object.
bool SafeApplyDamage(HealthPart* part, const Damages& damage)
{
    TR_TRY
    {
        part->applyDamage(damage);
        return true;
    }
    TR_CATCH { return false; }
}

bool SafeSetPartHealth(HealthPart* part, float health)
{
    TR_TRY
    {
        part->flesh = health;
        return true;
    }
    TR_CATCH { return false; }
}

// --- the knockout order and its fallback -------------------------------------

// The one native order being watched: the last knockout the player gave. A
// second one replaces it -- the player changed their mind. Raw pointers, only
// read behind guards and dropped at a load (ForgetCageCommands).
struct OrderWatch
{
    bool active;
    Character* actor;
    Character* target;
    int taskBefore;
    DWORD startedMs;

    OrderWatch() : active(false), actor(NULL), target(NULL), taskBefore(0), startedMs(0) {}
};

OrderWatch g_watch;
OrderedKnockoutCredit g_credit;

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

// The engine's own knockout.
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

void LogFailure(Character* actor, Character* target, const char* verb, const char* what)
{
    LogError("[cage] " + CharacterName(actor) + " -> " + CharacterName(target) + " " + verb +
             " failed: " + what);
}

void LogKnockoutResolved(const OrderWatch& watch, const char* via)
{
    if (GetConfig().probeOnClick)
    {
        LogInfo("[cage] " + CharacterName(watch.actor) + " -> " + CharacterName(watch.target) +
                " knockout via " + via);
    }
}

void LogBeatHit(Character* actor, Character* target, float blunt, float before, float after)
{
    if (GetConfig().probeOnClick)
    {
        char line[96];
        _snprintf_s(line, sizeof(line), _TRUNCATE,
                    " beat hit (blunt %.1f, part health %.1f -> %.1f)", blunt, before, after);
        LogInfo("[cage] " + CharacterName(actor) + " -> " + CharacterName(target) + line);
    }
}

void RememberCredit(Character* target)
{
    CharKey key;
    if (SafeCharKey(target, &key))
    {
        g_credit.Remember(key, GetTickCount());
    }
}

// --- the two commands ----------------------------------------------------------

bool LandBeatHit(Character* actor, Character* target)
{
    if (actor == NULL || target == NULL)
    {
        return false;
    }

    HealthPart* part = NULL;
    if (!SafeBeatPart(target, &part))
    {
        LogFailure(actor, target, "beat", "no torso part readable");
        return false;
    }
    float before = 0.0f;
    float maxHealth = 0.0f;
    if (!SafePartHealth(part, &before, &maxHealth))
    {
        LogFailure(actor, target, "beat", "part health unreadable");
        return false;
    }

    AttackNumbers attack = { 0.0f, 0.0f, 0.0f };
    const bool readable = SafeAttackDamage(actor, target, &attack);
    const float landed = CapBeatBlunt(BeatBlunt(readable, attack.cut, attack.blunt, attack.pierce),
                                      before, maxHealth);

    // Before the blow: a knockout it causes may fire the hook at once.
    RememberCredit(target);

    if (landed > 0.0f)
    {
        // A fist: blunt only, no bleeding, no armour penetration.
        const Damages blow(0.0f, landed, 0.0f, 0.0f, 0.0f);
        if (!SafeApplyDamage(part, blow))
        {
            LogFailure(actor, target, "beat", "applyDamage raised");
            return false;
        }
    }

    // CapBeatBlunt assumes the engine takes the blunt off one to one; whatever it
    // did, the part is held at the floor.
    const float floor = BeatHealthFloor(maxHealth);
    float after = before;
    float maxAfter = maxHealth;
    if (SafePartHealth(part, &after, &maxAfter) && after < floor)
    {
        after = floor;
        SafeSetPartHealth(part, after);
    }
    LogBeatHit(actor, target, landed, before, after);
    return true;
}

bool IssueKnockoutOrder(Character* actor, Character* target)
{
    if (actor == NULL || target == NULL)
    {
        return false;
    }

    Ogre::Vector3 where;
    if (!SafePosition(target, &where))
    {
        LogFailure(actor, target, "knockout", "target position unreadable");
        return false;
    }

    int taskBefore = (int)NULL_TASK;
    SafeCurrentTask(actor, &taskBefore);

    bool problems = false;
    const bool askedProblems = SafeOrderProblems(actor, STEALTH_KNOCKOUT, target, &problems);

    if (!SafeOrder(actor, STEALTH_KNOCKOUT, target, where))
    {
        LogFailure(actor, target, "knockout", "addOrder raised");
        return false;
    }

    if (GetConfig().probeOnClick)
    {
        char line[96];
        _snprintf_s(line, sizeof(line), _TRUNCATE,
                    " knockout ordered (task %d, was %d, checkPlayerOrderForProblems=%s)",
                    (int)STEALTH_KNOCKOUT, taskBefore,
                    askedProblems ? (problems ? "true" : "false") : "unreadable");
        LogInfo("[cage] " + CharacterName(actor) + " -> " + CharacterName(target) + line);
    }

    RememberCredit(target);

    g_watch.active = true;
    g_watch.actor = actor;
    g_watch.target = target;
    g_watch.taskBefore = taskBefore;
    g_watch.startedMs = GetTickCount();
    return true;
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
    return LandBeatHit(actor, target);
}

bool IssueKnockout(Character* actor, Character* target)
{
    return IssueKnockoutOrder(actor, target);
}

CageCommandResult OrderSelectedToBeat(Character* target)
{
    Character* actor = SelectedSquadActor(target);
    if (actor == NULL)
    {
        return CageCommand_NoActor;
    }
    return LandBeatHit(actor, target) ? CageCommand_Issued : CageCommand_Failed;
}

CageCommandResult OrderSelectedToKnockout(Character* target)
{
    Character* actor = SelectedSquadActor(target);
    if (actor == NULL)
    {
        return CageCommand_NoActor;
    }
    return IssueKnockoutOrder(actor, target) ? CageCommand_Issued : CageCommand_Failed;
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
        JudgeOrderWatch(g_watch.taskBefore, (int)STEALTH_KNOCKOUT, readNow, taskNow,
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
        LogKnockoutResolved(watch, "task");
        return;
    }
    if (!KnockoutFallback(watch.actor, watch.target))
    {
        LogFailure(watch.actor, watch.target, "knockout", "fallback raised");
        return;
    }
    LogKnockoutResolved(watch, "fallback");
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
