#include "common.h"
#include "cage_commands.h"
#include "cage_command_rules.h"
#include "beating_stress.h"
#include "char_key_reader.h"
#include "config.h"
#include "engine_reads.h"
#include "knockout_order_cleanup.h"
#include "knockout_watch_rules.h"
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
// second one replaces it -- the player changed their mind -- and the first actor
// is cleaned up. The watch stays on after the order is taken, until the target is
// down or the land window passes (knockout_watch_rules.h), and every way it ends
// lets the actor go (knockout_order_cleanup.h). The pointers are never trusted on
// their own: each tick re-resolves them against the keys read when the order went
// in (StillSameCharacter), so an unloaded or freed character is never acted on. An
// actor that no longer resolves drops the watch with no cleanup; a target that no
// longer resolves reads as unreadable and the actor is let go.
struct OrderWatch
{
    bool active;
    KnockoutWatchPhase phase;
    Character* actor;
    Character* target;
    CharKey actorKey;
    CharKey targetKey;
    int taskBefore;
    // The actor was sneaking when the order went in: the cleanup leaves it on.
    bool stealthBefore;
    // The watch's own clock (WatchFrameMs: real time that stops while the game is
    // paused): watch time since the order, the real tick it was last advanced at,
    // and how long in a row the actor has been off the order (NextAbandonedMs).
    unsigned int elapsedMs;
    DWORD lastTickMs;
    unsigned int abandonedMs;

    OrderWatch()
        : active(false), phase(KnockoutPhase_AwaitingTake), actor(NULL), target(NULL),
          taskBefore(0), stealthBefore(false), elapsedMs(0), lastTickMs(0), abandonedMs(0) {}
};

OrderWatch g_watch;
OrderedKnockoutCredit g_credit;
// R2.3: when Golpear last landed on each prisoner, and when Noquear last put each
// one down (game hours, session-only).
CageCooldownLedger g_beatCooldown;
CageCooldownLedger g_knockoutCooldown;

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

// Starts a command's rest on `target` (R2.3), from the game hour it landed.
void StartCooldown(CageCooldownLedger* ledger, Character* target)
{
    CharKey key;
    double now = 0.0;
    if (SafeCharKey(target, &key) && SafeGameHours(&now))
    {
        ledger->Remember(key, now);
    }
}

// Game hours until a command may land on `target` again (0: ready, or unreadable
// -- a wait the player cannot be shown is no wait).
float CooldownHoursLeft(const CageCooldownLedger& ledger, Character* target, float cooldownHours)
{
    CharKey key;
    double now = 0.0;
    if (target == NULL || !SafeFindCharKey(target, &key) || !SafeGameHours(&now))
    {
        return 0.0f;
    }
    return ledger.HoursLeft(key, now, cooldownHours);
}

float BeatHoursLeft(Character* target)
{
    return CooldownHoursLeft(g_beatCooldown, target, GetConfig().submission.beatingCooldownHours);
}

float KnockoutHoursLeft(Character* target)
{
    return CooldownHoursLeft(g_knockoutCooldown, target,
                             GetConfig().submission.knockoutCooldownHours);
}

// A blow, or the knockout order: each kind has its own window (CreditWindowMs).
void RememberCredit(Character* target, OrderedCreditKind kind)
{
    CharKey key;
    if (SafeCharKey(target, &key))
    {
        g_credit.Remember(key, GetTickCount(), kind);
    }
}

// --- the watch's end -----------------------------------------------------------

// Closes the watch (if any) and lets its actor go. Copied out and closed before
// acting, so nothing it calls can run it twice.
// An actor who no longer resolves is not cleaned up: there is nobody to clean.
void EndWatch(const char* why)
{
    if (!g_watch.active)
    {
        return;
    }
    const OrderWatch watch = g_watch;
    g_watch = OrderWatch();
    Character* actor = StillSameCharacter(watch.actor, watch.actorKey);
    if (actor == NULL)
    {
        if (GetConfig().probeOnClick)
        {
            LogInfo(std::string("[cage] knockout watch dropped (actor gone) ") + why);
        }
        return;
    }
    CleanUpKnockoutOrder(actor, StillSameCharacter(watch.target, watch.targetKey),
                         watch.stealthBefore, why);
}

// This frame's facts for the knockout watch, for its actor and target as they
// resolve this tick (NULL: no longer the same character).
KnockoutWatchFacts ReadWatchFacts(const OrderWatch& watch, Character* actor, Character* target)
{
    KnockoutWatchFacts facts;
    facts.actorGone = actor == NULL;
    if (facts.actorGone)
    {
        return facts;
    }
    facts.phase = watch.phase;
    facts.taskBefore = watch.taskBefore;
    facts.ordered = (int)STEALTH_KNOCKOUT;
    facts.readTask = SafeCurrentTask(actor, &facts.taskNow);
    bool out = false;
    bool dead = false;
    const bool readOut = target != NULL && SafeIsKnockedOut(target, &out);
    const bool readDead = target != NULL && SafeIsDead(target, &dead);
    facts.targetUnreadable = !readOut && !readDead;
    facts.targetDown = (readOut && out) || (readDead && dead);
    facts.elapsedMs = watch.elapsedMs;
    ReadKnockoutPursuit(actor, &facts.orderQueued, &facts.goalCurrent);
    facts.abandonedMs = watch.abandonedMs;
    return facts;
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

    if (landed > 0.0f)
    {
        // Before the blow (a knockout it causes may fire the hook at once), only
        // for a blow that can knock out at all, and only for a moment.
        RememberCredit(target, OrderedCredit_Beat);
        // A fist: blunt only, no bleeding, no armour penetration.
        const Damages blow(0.0f, landed, 0.0f, 0.0f, 0.0f);
        if (!SafeApplyDamage(part, blow))
        {
            LogFailure(actor, target, "beat", "applyDamage raised");
            return false;
        }
        StartCooldown(&g_beatCooldown, target);
        // Every blow that landed wears them down (beating_stress.h), knockout or not.
        AddBeatingStress(target, BeatingBlow_Hit);
    }

    // CapBeatBlunt assumes the engine takes the blunt off one to one; if it took
    // more, the part is held at the floor. Only after a blow that landed, and
    // never above where the blow found it (BeatHeldHealth): a part already at or
    // below the floor gets no blow and is not touched at all.
    float after = before;
    if (landed > 0.0f)
    {
        float maxAfter = maxHealth;
        if (SafePartHealth(part, &after, &maxAfter))
        {
            const float held = BeatHeldHealth(before, after, BeatHealthFloor(maxHealth));
            if (held != after)
            {
                after = held;
                SafeSetPartHealth(part, after);
            }
        }
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

    // A new order replaces the watched one: its actor is let go first, so the
    // stealth read below is the actor's own and not what the old order left.
    EndWatch("replaced by a new order");

    Ogre::Vector3 where;
    if (!SafePosition(target, &where))
    {
        LogFailure(actor, target, "knockout", "target position unreadable");
        return false;
    }

    // The watch keeps these keys and re-resolves both characters every tick.
    CharKey actorKey;
    CharKey targetKey;
    if (!SafeCharKey(actor, &actorKey) || !SafeCharKey(target, &targetKey))
    {
        LogFailure(actor, target, "knockout", "character key unreadable");
        return false;
    }

    int taskBefore = (int)NULL_TASK;
    SafeCurrentTask(actor, &taskBefore);
    bool stealthBefore = false;
    ReadStealthMode(actor, &stealthBefore);

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

    RememberCredit(target, OrderedCredit_Knockout);

    g_watch.active = true;
    g_watch.phase = KnockoutPhase_AwaitingTake;
    g_watch.actor = actor;
    g_watch.target = target;
    g_watch.actorKey = actorKey;
    g_watch.targetKey = targetKey;
    g_watch.taskBefore = taskBefore;
    g_watch.stealthBefore = stealthBefore;
    g_watch.elapsedMs = 0;
    g_watch.lastTickMs = GetTickCount();
    g_watch.abandonedMs = 0;
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
    facts.beatHoursLeft = facts.inPlayerCage ? BeatHoursLeft(target) : 0.0f;
    facts.knockoutHoursLeft = facts.inPlayerCage ? KnockoutHoursLeft(target) : 0.0f;
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

namespace
{
// The facts the button was shown for, read again at the click.
CageCommandResult CheckClick(Character* target, bool beat)
{
    switch (JudgeCageClick(ReadCageCommandFacts(target), beat))
    {
    case CageClick_NotAllowed: return CageCommand_NotAllowed;
    case CageClick_Resting:    return beat ? CageCommand_BeatResting : CageCommand_KnockoutResting;
    case CageClick_Go:
    default:                   return CageCommand_Issued;
    }
}
} // namespace

CageCommandResult OrderSelectedToBeat(Character* target)
{
    const CageCommandResult check = CheckClick(target, true);
    if (check != CageCommand_Issued)
    {
        return check;
    }
    Character* actor = SelectedSquadActor(target);
    if (actor == NULL)
    {
        return CageCommand_NoActor;
    }
    return LandBeatHit(actor, target) ? CageCommand_Issued : CageCommand_Failed;
}

CageCommandResult OrderSelectedToKnockout(Character* target)
{
    const CageCommandResult check = CheckClick(target, false);
    if (check != CageCommand_Issued)
    {
        return check;
    }
    Character* actor = SelectedSquadActor(target);
    if (actor == NULL)
    {
        return CageCommand_NoActor;
    }
    return IssueKnockoutOrder(actor, target) ? CageCommand_Issued : CageCommand_Failed;
}

std::string CageCommandProblem(CageCommandResult result, Character* target)
{
    switch (result)
    {
    case CageCommand_NoActor:
        return Say("cage.noActor", "Selecciona a alguien de tu grupo para hacerlo", NULL);
    case CageCommand_Failed:
        return Say("cage.failed", "No se pudo dar la orden", NULL);
    case CageCommand_NotAllowed:
        return Say("cage.notAllowed", "Ya no puedes hacerle eso", NULL);
    case CageCommand_BeatResting:
        return Fill(Say("cage.beatResting", "Podr\xc3\xa1s volver a golpearlo en {hours} h", NULL),
                    CharacterName(target), 0, RestHoursShown(BeatHoursLeft(target)));
    case CageCommand_KnockoutResting:
        return Fill(Say("cage.knockoutResting", "Podr\xc3\xa1s volver a noquearlo en {hours} h", NULL),
                    CharacterName(target), 0, RestHoursShown(KnockoutHoursLeft(target)));
    case CageCommand_Issued:
    default:
        return std::string();
    }
}

std::string BeatButtonLabel(Character* target)
{
    return RestLabel(Say("cage.button.beat", "Golpear", NULL), BeatHoursLeft(target));
}

std::string KnockoutButtonLabel(Character* target)
{
    float probability = 0.0f;
    const bool haveOdds = ReadKnockoutOdds(target, &probability);
    return KnockoutCaption(Say("cage.button.knockout", "Noquear", NULL), KnockoutHoursLeft(target),
                           haveOdds, probability);
}

void TickCageCommands()
{
    // Heals actors stuck with a knockout goal no watch owns (rate-limited inside).
    Character* actor = g_watch.active ? StillSameCharacter(g_watch.actor, g_watch.actorKey) : NULL;
    SweepStrayKnockoutGoals(actor);

    if (!g_watch.active)
    {
        return;
    }
    Character* target = StillSameCharacter(g_watch.target, g_watch.targetKey);

    // The watch's clock first: nothing passes while the game is paused.
    const DWORD now = GetTickCount();
    bool paused = false;
    SafeIsPaused(&paused);
    const unsigned int frameMs = WatchFrameMs(now - g_watch.lastTickMs, paused);
    g_watch.lastTickMs = now;
    g_watch.elapsedMs += frameMs;

    KnockoutWatchFacts facts = ReadWatchFacts(g_watch, actor, target);
    // The abandoned streak: off the order for kKnockoutAbandonMs in a row ends the
    // wait with a cleanup and no fallback.
    g_watch.abandonedMs = NextAbandonedMs(g_watch.abandonedMs, ActorAbandonedKnockout(facts),
                                          frameMs);
    facts.abandonedMs = g_watch.abandonedMs;

    const KnockoutWatchStep step = NextKnockoutWatchStep(
        facts, kOrderWatchTimeoutMs, kKnockoutLandWindowMs, kKnockoutAbandonMs);
    if (step == KnockoutStep_Wait)
    {
        return;
    }
    if (step == KnockoutStep_Taken)
    {
        // Not the end: the goal stays until the knockout lands, then goes.
        g_watch.phase = KnockoutPhase_AwaitingDown;
        LogKnockoutResolved(g_watch, "task");
        return;
    }

    // Copied out before acting: the fallback must run once, and the cleanup
    // closes the watch.
    const OrderWatch watch = g_watch;
    if (!KnockoutStepCleansUp(step))
    {
        // The actor no longer resolves: nothing to clean, nothing to act on.
        g_watch = OrderWatch();
        if (GetConfig().probeOnClick)
        {
            LogInfo(std::string("[cage] knockout watch dropped (") + KnockoutStepName(step) + ")");
        }
        return;
    }
    if (step == KnockoutStep_Fallback)
    {
        if (KnockoutFallback(actor, target))
        {
            LogKnockoutResolved(watch, "fallback");
        }
        else
        {
            LogFailure(actor, target, "knockout", "fallback raised");
        }
    }
    const std::string why = std::string("after ") + KnockoutStepName(step);
    EndWatch(why.c_str());
}

OrderedCreditKind ResolveCageKnockout(Character* victim)
{
    // Cheapest first: almost every knockout in the world was not ordered here.
    if (victim == NULL || g_credit.Size() == 0)
    {
        return OrderedCredit_None;
    }
    CharKey key;
    if (!SafeFindCharKey(victim, &key))
    {
        return OrderedCredit_None;
    }
    const OrderedCreditKind kind = g_credit.Claim(key, GetTickCount());
    if (kind == OrderedCredit_Knockout)
    {
        // Noquear landed: it rests from now (Golpear's rest started with the blow).
        StartCooldown(&g_knockoutCooldown, victim);
    }
    return kind;
}

void ForgetCageCommands()
{
    // Load begin: the characters are still alive, so the watched actor is let go
    // before its pointer is dropped (the goal would otherwise be saved with it).
    EndWatch("at load");
    g_credit.Clear();
    g_beatCooldown.Clear();
    g_knockoutCooldown.Clear();
}
} // namespace totalrecruit
