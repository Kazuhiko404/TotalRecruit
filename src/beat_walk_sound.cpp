#include "common.h"
#include "beat_walk_sound.h"
#include "beat_walk_rules.h"
#include "config.h"
#include "log.h"

#include <vector>

namespace totalrecruit
{
namespace
{
// Loot-Scoot-Execute.cpp TryPlayCharacterAudioEvent: a POD-only frame, the name
// as a plain pointer (C2712).
bool SafeAudioEvent(Character* character, const char* eventName)
{
    if (character == NULL || eventName == NULL || eventName[0] == '\0')
    {
        return false;
    }
    TR_TRY { return character->audioEvent(eventName, SOUNDRANGE_ALWAYS); }
    TR_CATCH { return false; }
}

// The first accepted name on `character`, or NULL.
const char* PlayFirst(Character* character, const std::vector<std::string>& names)
{
    for (size_t i = 0; i < names.size(); ++i)
    {
        if (SafeAudioEvent(character, names[i].c_str()))
        {
            return names[i].c_str();
        }
    }
    return NULL;
}

void LogPlayed(const char* name, const char* emitter)
{
    if (GetConfig().probeOnClick)
    {
        LogInfo(std::string("[cage] beat hit sound ") + name + " on " + emitter);
    }
}
} // namespace

bool PlayBeatHitSound(Character* prisoner, Character* actor, const std::string& configured)
{
    const std::vector<std::string> names = BeatHitSoundCandidates(configured);
    const char* played = PlayFirst(prisoner, names);
    if (played != NULL)
    {
        LogPlayed(played, "prisoner");
        return true;
    }
    played = PlayFirst(actor, names);
    if (played != NULL)
    {
        LogPlayed(played, "actor");
        return true;
    }
    LogError("[cage] beat hit sound: no event accepted (submissionBeatHitSound \"" + configured + "\")");
    return false;
}
} // namespace totalrecruit
