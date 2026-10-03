#pragma once

// PURE: the decisions behind the cage commands (Golpear / Noquear), without the
// engine. cage_commands.cpp is the engine side: it reads the facts, issues the
// native orders and asks these rules what the facts mean.
//
//   ComposeContextBlock     which buttons stand beside the context menu.
//   JudgeOrderWatch         did the actor take the native order, or does the
//                           fallback have to run.
//   OrderedKnockoutCredit   a knockout the player ordered through the mod counts
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

    CageCommandFacts() : allowBeating(true), inPlayerCage(false), down(false) {}
};

// True when Golpear and Noquear apply: a prisoner of the player's, awake, and the
// player allowed beating.
bool CageCommandsAllowed(const CageCommandFacts& facts);

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

// How long after the order a knockout still counts as the one the player ordered.
// A native beating walks to the cage and lands several blows first.
extern const unsigned int kOrderedKnockoutWindowMs;

// The knockouts the player ordered through the mod and that have not landed yet,
// by target. The knockout hook claims one when it fires; an expired one is
// forgotten. Session-only: the load boundary clears it.
class OrderedKnockoutCredit
{
public:
    // The player just ordered a beating or a knockout on `target`.
    void Remember(const CharKey& target, unsigned int nowMs);

    // True (once) when `target` has a pending order younger than the window;
    // consumes it either way.
    bool Claim(const CharKey& target, unsigned int nowMs);

    void Clear();
    int Size() const;

private:
    typedef std::map<CharKey, unsigned int> Pending;
    Pending pending_;
};

// --- the knockout label ----------------------------------------------------

// The engine's odds as a whole percent, 0..100. The engine reports a fraction;
// a value already above 1 is read as a percent.
int KnockoutPercent(float probability);

// `base` alone when the odds could not be read, otherwise "<base> <pct>%".
std::string KnockoutLabel(const std::string& base, bool haveOdds, float probability);

// --- the fallback knockout -------------------------------------------------

// MedicalSystem::knockout takes the attacker's skill as 0..1; CharStats::getStat
// reads a skill level, 0..100 (more with gear). Clamped into 0..1.
float SkillFraction(float statLevel);
} // namespace totalrecruit
