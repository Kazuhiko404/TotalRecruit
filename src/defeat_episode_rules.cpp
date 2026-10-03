#include "defeat_episode_rules.h"

namespace totalrecruit
{
namespace DefeatEpisode
{
State NoteStanding(State state)
{
    state.countedThisEpisode = false;
    state.seenStanding = true;
    return state;
}

Verdict Judge(const State& state, Sighting sighting)
{
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

State AfterCount(State state)
{
    state.countedThisEpisode = true;
    state.seenStanding = false;
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
