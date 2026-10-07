#include "common.h"
#include "beat_walk_sound.h"
#include "beat_hit_sound_rules.h"
#include "config.h"
#include "engine_reads.h"
#include "log.h"
#include "wwise_bridge.h"

#include <cstdio>
#include <vector>

namespace totalrecruit
{
namespace
{
// Loot-Scoot-Execute.cpp TryPlayCharacterAudioEvent: a POD-only frame, the names
// as plain pointers (C2712). The fallback path: the engine's own wrappers.
bool SafeAudioEvent(Character* character, const char* eventName)
{
    if (character == NULL || eventName == NULL || eventName[0] == '\0')
    {
        return false;
    }
    TR_TRY { return character->audioEvent(eventName, SOUNDRANGE_ALWAYS); }
    TR_CATCH { return false; }
}

bool SafeAudioSwitch(Character* character, const char* group, const char* value)
{
    if (character == NULL || group == NULL || value == NULL)
    {
        return false;
    }
    TR_TRY
    {
        character->audioValue(group, value);
        return true;
    }
    TR_CATCH { return false; }
}

std::string Describe(const BeatSoundCue& cue)
{
    if (cue.switchGroup.empty())
    {
        return cue.eventName;
    }
    return cue.eventName + " (" + cue.switchGroup + "=" + cue.switchValue + ")";
}

void LogPlayed(const std::string& what)
{
    if (GetConfig().probeOnClick)
    {
        LogInfo("[cage] beat hit sound " + what);
    }
}

// Through the game's Wwise: true when PostEvent returned a playing id.
bool PlayDirect(const BeatSoundCue& cue, Character* prisoner, unsigned __int64 gameObject)
{
    if (!cue.switchGroup.empty() && !WwiseSetSwitchAvailable())
    {
        SafeAudioSwitch(prisoner, cue.switchGroup.c_str(), cue.switchValue.c_str());
    }
    else if (!cue.switchGroup.empty() &&
             !WwiseSetSwitch(cue.switchGroup.c_str(), cue.switchValue.c_str(), gameObject))
    {
        LogError("[cage] beat hit sound: SetSwitch " + cue.switchGroup + "=" + cue.switchValue +
                 " failed (the impact plays with the switch it had)");
    }
    const unsigned long playing = WwisePostEvent(cue.eventName.c_str(), gameObject);
    if (playing == 0)
    {
        LogError("[cage] beat hit sound " + Describe(cue) + " failed: PostEvent returned 0");
        return false;
    }
    char id[32];
    _snprintf_s(id, sizeof(id), _TRUNCATE, "%lu", playing);
    LogPlayed(Describe(cue) + " played id " + id);
    return true;
}

// Through Character::audioValue / audioEvent: unconfirmed, audioEvent says true
// for unknown events too.
bool PlayFallback(const BeatSoundCue& cue, Character* prisoner)
{
    if (!cue.switchGroup.empty())
    {
        SafeAudioSwitch(prisoner, cue.switchGroup.c_str(), cue.switchValue.c_str());
    }
    if (!SafeAudioEvent(prisoner, cue.eventName.c_str()))
    {
        LogError("[cage] beat hit sound " + Describe(cue) + " failed: audioEvent refused it");
        return false;
    }
    LogPlayed(Describe(cue) + " via audioEvent (unconfirmed)");
    return true;
}
} // namespace

bool PlayBeatHitSound(Character* prisoner, const std::string& impactKind)
{
    if (prisoner == NULL)
    {
        return false;
    }
    bool animal = false;
    SafeIsAnimal(prisoner, &animal);
    const std::vector<BeatSoundCue> cues = BeatHitSoundCues(impactKind, animal);

    unsigned __int64 gameObject = 0;
    const bool direct = WwisePostEventAvailable() && SafeAudioObject(prisoner, &gameObject);
    bool any = false;
    for (size_t i = 0; i < cues.size(); ++i)
    {
        const bool played = direct ? PlayDirect(cues[i], prisoner, gameObject) : PlayFallback(cues[i], prisoner);
        any = any || played;
    }
    return any;
}
} // namespace totalrecruit
