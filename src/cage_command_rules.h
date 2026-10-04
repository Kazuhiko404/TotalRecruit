#pragma once

// PURE: the decisions behind the cage commands (Golpear / Noquear), without the
// engine. cage_commands.cpp is the engine side: it reads the facts, lands the
// blow or issues the knockout order and asks these rules what the facts mean.
//
//   ComposeContextBlock     which buttons stand beside the context menu.
//   JudgeCageClick          what a click on Golpear / Noquear may do now (the
//                           facts re-read at click time).
//   CageCooldownLedger      R2.3's interval between beatings (and between
//                           knockouts), per prisoner and per command, and
//   RestLabel               the wait on a resting command's caption.
//   KnockoutCaption         Noquear's caption: the wait, or the engine's odds.
//   BeatBlunt .. PickBeatPart  Golpear's one capped blow: how hard, where, and
//                           the floor it never crosses.
//   JudgeOrderWatch         did the actor take the knockout order, or does the
//                           fallback have to run (the first phase of
//                           knockout_watch_rules.h NextKnockoutWatchStep).
//   OrderedKnockoutCredit   a knockout the player caused through the mod counts
//                           as the player's, even when the engine names nobody.
//   KnockoutLabel           "NOQUEAR 72%" from the engine's own odds.
//
// No Kenshi / MyGUI include: tested in tests/rules_tests/cage_command_rules_tests.cpp.

#include "char_key.h"

#include <map>
#include <string>
#include <vector>

namespace totalrecruit
{
// One button of the block beside the native context menu, top to bottom.
enum ContextButtonKind
{
    ContextButton_Talk,
    ContextButton_Beat,
    ContextButton_Knockout,
    ContextButton_Count
};

// What decides whether the cage commands are offered for a target.
struct CageCommandFacts
{
    bool allowBeating;   // config: submission.allowBeating
    bool inPlayerCage;   // the target sits in one of the player's cages
    bool down;           // already unconscious (or dead): nothing left to beat
    float beatHoursLeft; // game hours until Golpear may land again on them (0: ready)
    float knockoutHoursLeft; // game hours until Noquear may be ordered again (0: ready)

    CageCommandFacts()
        : allowBeating(true), inPlayerCage(false), down(false), beatHoursLeft(0.0f),
          knockoutHoursLeft(0.0f) {}
};

// True when Golpear and Noquear apply: a prisoner of the player's, awake, and the
// player allowed beating. The buttons show whenever this holds; a command's rest
// only changes its caption (RestLabel, KnockoutCaption) and what its click does
// (JudgeCageClick).
bool CageCommandsAllowed(const CageCommandFacts& facts);

// True when Golpear may land now: the commands apply and it is not resting.
bool BeatReady(const CageCommandFacts& facts);

// True when Noquear may be ordered now: the commands apply and it is not resting.
bool KnockoutReady(const CageCommandFacts& facts);

// What a click on a cage command may do, judged on the facts read at the click
// (a block or a popup may have been built a while ago).
enum CageClickVerdict
{
    CageClick_Go,          // act
    CageClick_NotAllowed,  // the commands no longer apply (freed, down, beating off)
    CageClick_Resting      // the clicked command is still inside its rest (Golpear:
                           // between beatings; Noquear: between knockouts)
};

CageClickVerdict JudgeCageClick(const CageCommandFacts& facts, bool beat);

// The block for one menu: Hablar always, then Golpear and Noquear when the cage
// commands apply.
std::vector<ContextButtonKind> ComposeContextBlock(bool cageCommands);

// --- the order watch -------------------------------------------------------

enum OrderWatchVerdict
{
    OrderWatch_Pending,   // nothing changed yet, still inside the timeout
    OrderWatch_Taken,     // the actor is on the order (or moved on to it)
    OrderWatch_TimedOut   // the timeout passed and the actor never changed task
};

// How long the actor has to take a native order before the fallback runs.
extern const unsigned int kOrderWatchTimeoutMs;

// `taskBefore` is the actor's task when the order went in, `ordered` the task
// that was ordered and `taskNow` the task read this frame (`readNow` false when
// it could not be read). Task values are the engine's TaskType as ints. Elapsed
// time is unsigned milliseconds (GetTickCount differences wrap safely).
OrderWatchVerdict JudgeOrderWatch(int taskBefore, int ordered, bool readNow, int taskNow,
                                  unsigned int elapsedMs, unsigned int timeoutMs);

// --- the credit bridge -----------------------------------------------------

// How long after Noquear's order a knockout still counts as the one the player
// ordered: the order walks the actor to the cage and the door first.
extern const unsigned int kOrderedKnockoutWindowMs;

// How long after Golpear's blow a knockout still counts as the blow's: a few
// seconds around the hit (the engine settles the collapse on its next updates).
// Anything later -- a starvation collapse, someone else's blow -- is not the
// player's blow, and the ordinary credit rules decide it.
extern const unsigned int kBeatCreditWindowMs;

// Which cage command a pending credit came from.
enum OrderedCreditKind
{
    OrderedCredit_None,      // nothing pending (or it expired)
    OrderedCredit_Beat,      // Golpear's blow
    OrderedCredit_Knockout   // Noquear's order
};

// The window each kind lasts: kBeatCreditWindowMs, kOrderedKnockoutWindowMs, 0.
unsigned int CreditWindowMs(OrderedCreditKind kind);

// A knockout either cage command caused (any claimed kind). It is NOT a defeat:
// hitting a caged prisoner is not beating them in a fight (R3.2). It is beating
// stress instead (beating_stress_rules.h).
bool IsCageKnockout(OrderedCreditKind kind);

// Whether the knockout itself adds beating stress: Noquear's does; Golpear's blow
// already added its own when it landed, so its knockout adds nothing more.
bool CageKnockoutAddsStress(OrderedCreditKind kind);

// The knockouts the player caused through the mod and that have not landed yet,
// by target, each with its kind's window. The knockout hook claims one when it
// fires; an expired one is forgotten. Session-only: the load boundary clears it.
class OrderedKnockoutCredit
{
public:
    // The player just landed a blow or ordered a knockout on `target`. A newer
    // remember replaces an older one, kind and window included.
    void Remember(const CharKey& target, unsigned int nowMs, OrderedCreditKind kind);

    // The kind (once) when `target` has a pending credit younger than its window,
    // else OrderedCredit_None; consumes it either way.
    OrderedCreditKind Claim(const CharKey& target, unsigned int nowMs);

    void Clear();
    int Size() const;

private:
    struct Entry
    {
        unsigned int atMs;
        OrderedCreditKind kind;
    };
    typedef std::map<CharKey, Entry> Pending;
    Pending pending_;
};

// --- the rest between beatings and between knockouts (R2.3) -----------------
//
// Golpear rests per prisoner for submission.beatingCooldownHours game hours after
// a blow that landed; Noquear for submission.knockoutCooldownHours after a
// knockout order that put the prisoner down. One ledger per command. Session-only,
// like the defeat episodes (defeats.cpp): the load boundary clears it, so an
// earlier save never inherits a later clock.
class CageCooldownLedger
{
public:
    // The command just landed on `target` at game hour `nowHours`.
    void Remember(const CharKey& target, double nowHours);

    // Game hours until the command may land on `target` again: 0 when it never
    // landed, when the cooldown is 0 or less, or when the clock reads earlier than
    // the landing.
    float HoursLeft(const CharKey& target, double nowHours, float cooldownHours) const;

    void Clear();

private:
    typedef std::map<CharKey, double> Last;
    Last last_;
};

// The whole hours a wait shows: rounded up, 0 when there is none.
int RestHoursShown(float hoursLeft);

// `base` when the command is ready, else "<base> (<hours> h)" with the hours
// rounded up.
std::string RestLabel(const std::string& base, float hoursLeft);

// --- the knockout label ----------------------------------------------------

// The engine's odds as a whole percent, 0..100. The engine reports a fraction;
// a value already above 1 is read as a percent.
int KnockoutPercent(float probability);

// `base` alone when the odds could not be read, otherwise "<base> <pct>%".
std::string KnockoutLabel(const std::string& base, bool haveOdds, float probability);

// Noquear's caption: the wait while it rests ("Noquear (1 h)", RestLabel), else
// the engine's odds (KnockoutLabel).
std::string KnockoutCaption(const std::string& base, float hoursLeft, bool haveOdds,
                            float probability);

// --- the fallback knockout -------------------------------------------------

// MedicalSystem::knockout takes the attacker's skill as 0..1; CharStats::getStat
// reads a skill level, 0..100 (more with gear). Clamped into 0..1.
float SkillFraction(float statLevel);

// --- Golpear: one capped blow ----------------------------------------------
//
// Golpear is one direct hit from the actor on the prisoner's torso, no orders:
// a fight would pull the prisoner out of the cage. The blow is a fist's -- blunt
// only, nothing cut or pierced, so nothing bleeds -- and it is capped so the part
// it lands on never drops below a floor well above the death threshold (a fatal
// part dies at -max health). It may knock the prisoner out; it never kills.

// The blow when the engine's attack damage for this attacker cannot be read.
extern const float kBeatDefaultBlunt;

// The part never goes below -kBeatHealthFloorFraction x its max health.
extern const float kBeatHealthFloorFraction;

// MedicalSystem::HealthPartStatus::PART_TORSO as an int (the enum's first value).
extern const int kBeatTorsoPartType;

// The blunt the blow carries: the engine's whole attack damage for this attacker
// (CharStats::getTotalAttackDamageFor: cut + blunt + pierce, negatives read as
// zero) landed as blunt. kBeatDefaultBlunt when unreadable or zero.
float BeatBlunt(bool readable, float cut, float blunt, float pierce);

// The lowest health the blow may leave the part at: -fraction x maxHealth. Zero
// (the safest floor) when maxHealth is unreadable (<= 0).
float BeatHealthFloor(float maxHealth);

// The blunt that actually lands on a part at `health`: the blow, cut down so the
// part stops at BeatHealthFloor(maxHealth); zero when it is already there or
// below, and never negative (a blow never heals). Assumes the engine takes the
// blunt off the part's health one to one; the engine side re-checks after the
// hit and holds the part at the floor if it went further.
float CapBeatBlunt(float blunt, float health, float maxHealth);

// The health to hold the part at after the blow: `floor` when the blow pushed a
// part that started above it (`before`) past it (`after` < floor), else `after`
// unchanged. Never above `before`: a part the blow found already at or below the
// floor (an earlier fight, a bleed) is never raised -- a blow never heals.
float BeatHeldHealth(float before, float after, float floor);

// One slot of the target's anatomy, as read from the engine. POD: it is filled
// inside a guarded frame.
struct BeatPartFacts
{
    bool present;   // the slot holds a part
    int type;       // HealthPartStatus::whatAmI as an int
    bool fatal;     // HealthPartStatus::fatal
};

// The part the blow lands on: the first torso part (in anatomy order, the chest
// before the stomach on the races checked), else the first fatal part (an
// anatomy with no torso), else -1.
int PickBeatPart(const BeatPartFacts* parts, int count);
} // namespace totalrecruit
