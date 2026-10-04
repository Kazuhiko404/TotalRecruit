#include "defeat_episode_rules.h"

namespace totalrecruit
{
namespace DefeatEpisode
{
const unsigned int kKnockoutEchoWindowMs = 5000;

namespace
{
bool InsideEchoWindow(const State& state, unsigned int nowMs)
{
    // Unsigned subtraction: correct across a GetTickCount wrap.
    return state.everCounted && (nowMs - state.lastCountMs) < kKnockoutEchoWindowMs;
}
} // namespace

State NoteStanding(State state)
{
    state.countedThisEpisode = false;
    state.seenStanding = true;
    return state;
}

Verdict Judge(const State& state, Sighting sighting, unsigned int nowMs)
{
    // The same knockout echoing: the hook fires twice a second or so apart, and
    // the standing read before each is not to be trusted that close to a count.
    if (InsideEchoWindow(state, nowMs))
    {
        return Verdict_AlreadyCounted;
    }
    // The hook read them on their feet a moment before the native knockout ran:
    // that is the fall itself, whatever was counted before.
    if (sighting == Sighting_KnockoutFromStanding)
    {
        return Verdict_CountNewEpisode;
    }
    if (state.countedThisEpisode)
    {
        return Verdict_AlreadyCounted;
    }
    return state.seenStanding ? Verdict_CountAfterStanding : Verdict_CountFirstSeenDown;
}

bool Counts(Verdict verdict)
{
    return verdict != Verdict_AlreadyCounted;
}

bool CountsForPlayer(DefeatCredit credit, Verdict verdict)
{
    return credit == DefeatCredit_Counts && Counts(verdict);
}

State AfterCount(State state, unsigned int nowMs)
{
    state.countedThisEpisode = true;
    state.seenStanding = false;
    state.everCounted = true;
    state.lastCountMs = nowMs;
    return state;
}

const char* VerdictName(Verdict verdict)
{
    switch (verdict)
    {
    case Verdict_CountNewEpisode:    return "knocked down from standing";
    case Verdict_CountAfterStanding: return "down after being seen standing";
    case Verdict_CountFirstSeenDown: return "first seen already down";
    case Verdict_AlreadyCounted:     return "already counted this knockout";
    }
    return "?";
}
} // namespace DefeatEpisode
} // namespace totalrecruit
