#include "common.h"
#include "beat_walk_feedback.h"
#include "beat_walk.h"
#include "beat_walk_rules.h"
#include "cage_commands.h"
#include "dialogue_authoring.h"
#include "engine_reads.h"
#include "messages.h"
#include "recruit_core.h"

namespace totalrecruit
{
namespace
{
// Fill replaces {name}; the actor's {actor} is this line's own.
std::string WithActor(const std::string& text, const std::string& actorName)
{
    std::string out = text;
    const std::string token = "{actor}";
    size_t at = out.find(token);
    while (at != std::string::npos)
    {
        out.replace(at, token.size(), actorName);
        at = out.find(token, at + actorName.size());
    }
    return out;
}

std::string Line(const char* key, const char* fallback, const BeatOutcome& outcome)
{
    return WithActor(Fill(Say(key, fallback, NULL), outcome.targetName, 0, 0), outcome.actorName);
}

void Complain(Character* prisoner)
{
    if (prisoner == NULL)
    {
        return;
    }
    bool animal = false;
    if (!SafeIsAnimal(prisoner, &animal) || !BeatComplaintSpoken(animal))
    {
        return;
    }
    SpeakAloud(prisoner, Say("cage.beatOuch", "\xc2\xa1Ugh!", prisoner));
}

// The cage's own line for the aborts that are the cage's, else "".
std::string CageLine(BeatWalkAbort reason, Character* target)
{
    switch (reason)
    {
    case BeatAbort_NotAllowed: return CageCommandProblem(CageCommand_NotAllowed, target);
    case BeatAbort_Resting:    return CageCommandProblem(CageCommand_BeatResting, target);
    case BeatAbort_Failed:     return CageCommandProblem(CageCommand_Failed, target);
    default:                   return std::string();
    }
}
} // namespace

void TellBeatOutcome(const BeatOutcome& outcome)
{
    if (outcome.kind == BeatOutcome_Landed)
    {
        Complain(outcome.target);
        TellPlayer(Line("cage.beatLanded", "{actor} golpea a {name}", outcome), false);
        return;
    }
    if (!BeatAbortTellsPlayer(outcome.reason))
    {
        return;
    }
    std::string line = CageLine(outcome.reason, outcome.target);
    if (line.empty())
    {
        line = Line(BeatAbortMessageKey(outcome.reason), BeatAbortFallback(outcome.reason), outcome);
    }
    if (!line.empty())
    {
        TellPlayer(line, true);
    }
}
} // namespace totalrecruit
