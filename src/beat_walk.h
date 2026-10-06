#pragma once

// Golpear walks first, engine side (beat-feedback T1). The blow used to land the
// moment the button was pressed, from anywhere, with nothing on screen: it looked
// like the button did nothing. Now the selected character walks to the prisoner
// and the blow lands only once it is within reach (submissionBeatReachMeters);
// already within reach, it lands at once.
//
// The walk reuses npc_motion.h: MoveTo (the move order of TotalSparring's
// challenge walk-up, into the prisoner's building), DistanceMeters and IsUp; on
// arrival StopWalking(arrived) halts the actor and Face turns it to the prisoner.
// The order is given once -- a caged prisoner does not move -- and never
// re-issued, so a player who sends the actor somewhere else is never overridden.
// The clock is approach_driver.h's EventClock: real seconds that stop while the
// game is paused.
//
// One pending strike per actor (up to kMaxBeatWalks at once), keyed by the
// actor's CharKey: an address is not an identity, so the actor and the prisoner
// are re-resolved from their keys every tick (StillSameCharacter) like the
// walk-up's walker and the knockout watch. A second Golpear by the same actor
// replaces its pending strike (silently: the new one is the answer). Two actors
// may walk to the same prisoner; whoever arrives first lands, and the second
// finds Golpear resting and is told so.
//
// Each frame the pure rules decide (beat_walk_rules.h NextBeatWalkStep): actor
// gone or down, prisoner no longer a valid target or resting (JudgeCageClick, the
// click's own judgement), land in reach, timeout (submissionBeatWalkSeconds), or
// no closer for kBeatWalkStallSeconds (blocked, or the player gave the actor
// another order). Any abort lands nothing and starts no rest; no order is touched
// on an abort (the actor may already be doing what the player told it). A load
// drops every pending strike with no message.
//
// This module knows neither the blow nor the player. cage_commands hands it the
// two things it owns (BeatWalkHandlers::judge, ::land) and the caller -- the
// context menu's Golpear, the popup's GOLPEAR -- hands it who reports the outcome
// (::report, beat_walk_feedback.h). That keeps it below cage_commands and below
// recruit_core instead of in a ring with either.
//
// Log (probeOnClick): "[cage] <actor> -> <target> beat walk started (D m)",
// "... beat walk arrived (D m)", "... beat walk aborted (<why>)".

#include "beat_walk_rules.h"

#include <string>

class Character;

namespace totalrecruit
{
const int kMaxBeatWalks = 8;

enum BeatOutcomeKind
{
    BeatOutcome_Landed,   // in reach: the blow landed
    BeatOutcome_Aborted   // no blow (BeatOutcome::reason)
};

// What the reporter is told. The pointers are the characters as they resolve now
// (NULL when they no longer do); the names were read at the click, so an actor
// who is gone can still be named.
struct BeatOutcome
{
    BeatOutcomeKind kind;
    BeatWalkAbort reason;
    Character* actor;
    Character* target;
    std::string actorName;
    std::string targetName;

    BeatOutcome() : kind(BeatOutcome_Aborted), reason(BeatAbort_None), actor(NULL), target(NULL) {}
};

// Golpear's verdict on the prisoner now (JudgeCageClick on ReadCageCommandFacts).
typedef CageClickVerdict (*BeatTargetJudge)(Character* target);
// Lands the blow; false when it could not be landed at all (logged by the lander).
typedef bool (*BeatLander)(Character* actor, Character* target);
// Tells the player (and the prisoner's complaint, T2): called for every landing and
// every abort, NULL to report nothing.
typedef void (*BeatOutcomeSink)(const BeatOutcome& outcome);

struct BeatWalkHandlers
{
    BeatTargetJudge judge;
    BeatLander land;
    BeatOutcomeSink report;

    BeatWalkHandlers() : judge(NULL), land(NULL), report(NULL) {}
};

enum BeatWalkStart
{
    BeatWalkStart_Landed,   // in reach at the click: landed and reported
    BeatWalkStart_Walking,  // the actor is on its way
    BeatWalkStart_Ended,    // aborted at the click (reported), nothing landed
    BeatWalkStart_Failed    // nothing could be done (no key, no move order, no slot,
                            // or the blow in reach failed): not reported -- the
                            // caller says cage.failed
};

// Starts Golpear for `actor` on `target`: lands at once in reach, else gives the
// move order and keeps the strike pending. The click's facts are the caller's to
// check first (OrderSelectedToBeat); `handlers.judge` and `.land` are required.
BeatWalkStart StartBeatWalk(Character* actor, Character* target, const BeatWalkHandlers& handlers);

// The frame tick (from TickCageCommands): one step of every pending strike.
void TickBeatWalks();

// Load boundary: drops every pending strike, with no message and no order touched.
void ForgetBeatWalks();
} // namespace totalrecruit
