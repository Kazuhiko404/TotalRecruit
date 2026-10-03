#pragma once

// PURE: one defeat per knockout.
//
// A knockout is an episode, not an event: the engine calls MedicalSystem::knockout
// several times while a body settles and keeps calling it while they lie there,
// and the right-click looks at the same body again every time the player clicks.
// A debounce in game hours used to separate "a second beating" from "an echo of
// the first", and a long knockout outlived it and counted twice.
//
// The rule is TotalSparring's seen-standing rule (src/spar/domain/
// phantom_ko_rules): a defeat counts when they are down AND nothing has counted
// yet in this episode; seeing them on their feet again re-arms it. A knockout
// hook call that found them standing just before it ran is itself proof they
// were up, so it always opens a new episode.
//
// Who gets the credit is a separate question (DecideDefeatCredit, defeat_rules.h)
// asked before this one. No engine -- tested in
// tests/rules_tests/defeat_episode_rules_tests.cpp.

#include "defeat_rules.h"

namespace totalrecruit
{
namespace DefeatEpisode
{
struct State
{
    // A defeat was already counted since they were last seen standing.
    bool countedThisEpisode;
    // They were seen on their feet since the last counted defeat (diagnostics:
    // it tells "went down after being seen up" from "first seen already down").
    bool seenStanding;

    State() : countedThisEpisode(false), seenStanding(false) {}
};

// How the plugin came to see them down.
enum Sighting
{
    // The knockout hook, with the victim read standing just before the native
    // knockout ran: the transition itself, so a new episode.
    Sighting_KnockoutFromStanding,
    // Already down when looked at: a repeat knockout call as the body settles,
    // the right-click, the beating's explicit credit, or an unreadable state.
    Sighting_FoundDown
};

enum Verdict
{
    // The hook saw the fall from standing.
    Verdict_CountNewEpisode,
    // Down, nothing counted since they were last seen standing.
    Verdict_CountAfterStanding,
    // Down, never seen standing, nothing counted yet: counts once.
    Verdict_CountFirstSeenDown,
    // Already counted in this episode.
    Verdict_AlreadyCounted
};

// They were seen on their feet: the next knockout is a new episode.
State NoteStanding(State state);

// Whether this sighting is a defeat that counts.
Verdict Judge(const State& state, Sighting sighting);

bool Counts(Verdict verdict);

// The whole question: the credit first (DecideDefeatCredit), then the episode. A
// knockout the player did not inflict never counts, however fresh the episode.
bool CountsForPlayer(DefeatCredit credit, Verdict verdict);

// The state once a defeat has counted.
State AfterCount(State state);

// For logs only.
const char* VerdictName(Verdict verdict);
} // namespace DefeatEpisode
} // namespace totalrecruit
