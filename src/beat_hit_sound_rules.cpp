#include "beat_hit_sound_rules.h"

namespace totalrecruit
{
const char* const kBeatImpactKindDefault = "HtH_Impacts";

namespace
{
const char* const kImpactEvent = "Impact";
const char* const kImpactSwitchGroup = "Impacts";
const char* const kHumanVoiceEvent = "VO_Get_Hit";
const char* const kAnimalVoiceEvent = "VO_Creature_Take_Damage";

std::string Trimmed(const std::string& text)
{
    const char* const blanks = " \t\r\n";
    const std::string::size_type first = text.find_first_not_of(blanks);
    if (first == std::string::npos)
    {
        return std::string();
    }
    const std::string::size_type last = text.find_last_not_of(blanks);
    return text.substr(first, last - first + 1);
}

BeatSoundCue Cue(const char* eventName, const std::string& group, const std::string& value)
{
    BeatSoundCue cue;
    cue.eventName = eventName;
    cue.switchGroup = group;
    cue.switchValue = value;
    return cue;
}
} // namespace

std::vector<BeatSoundCue> BeatHitSoundCues(const std::string& impactKind, bool animal)
{
    std::vector<BeatSoundCue> cues;
    const std::string kind = Trimmed(impactKind);
    if (kind.empty())
    {
        cues.push_back(Cue(kImpactEvent, std::string(), std::string()));
    }
    else
    {
        cues.push_back(Cue(kImpactEvent, kImpactSwitchGroup, kind));
    }
    cues.push_back(Cue(animal ? kAnimalVoiceEvent : kHumanVoiceEvent, std::string(), std::string()));
    return cues;
}
} // namespace totalrecruit
