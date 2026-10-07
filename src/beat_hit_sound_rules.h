#pragma once

// PURE: which Wwise events Golpear posts on the prisoner when the blow lands
// (priority-one S1). beat_walk_sound.cpp plays them.
//
// Why these names: the first build tried Loot-Scoot-Execute's list ("Heavy_Hit",
// "Light_Hit", "Play_*") and nothing was heard, because none of them is an Event
// object in Kenshi's banks (data/audio/*.bnk: FNV-1 lowercase hash of the name vs
// the HIRC type-4 ids). Character::audioEvent still returned true, so it could
// not tell. The events that exist, traced to their containers:
//
//   "Impact"                   SFX_Bank / SFX_General_Bank: a switch container on
//                              the "Impacts" group (default Light_Hit; values
//                              Heavy_Hit, Light_Hit, Parry, Metal, ..., HtH_Impacts,
//                              HtH_Impacts_Robot). So "Heavy_Hit" was a switch
//                              VALUE, not an event.
//   "VO_Get_Hit"               VO_Bank: switch on "VO_Races" (human_male, Hive,
//                              skeleton, ...), which the game sets per character.
//   "VO_Creature_Take_Damage"  VO_Bank: switch on "Creature_VO" (Dog, Beak, Bull,
//                              Goat, Spider, ...), set per animal by the game.
//
// So a blow is two cues: "Impact" with the "Impacts" switch set to the configured
// impact kind (submissionBeatHitSound, default "HtH_Impacts", the unarmed hit;
// blank = leave the switch as it is), then the voice for a person or an animal,
// whose switch the game already set on that character.
//
// No Kenshi / MyGUI include: tested in tests/rules_tests/beat_hit_sound_rules_tests.cpp.

#include <string>
#include <vector>

namespace totalrecruit
{
// submissionBeatHitSound's default: the "Impacts" switch value of an unarmed hit.
extern const char* const kBeatImpactKindDefault;

struct BeatSoundCue
{
    std::string eventName;    // the Wwise event to post
    std::string switchGroup;  // "" = set no switch before posting
    std::string switchValue;
};

// The cues for one blow, in order: the impact, then the voice.
std::vector<BeatSoundCue> BeatHitSoundCues(const std::string& impactKind, bool animal);
} // namespace totalrecruit
