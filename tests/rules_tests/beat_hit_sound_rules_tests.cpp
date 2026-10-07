// Tests for src/beat_hit_sound_rules: which Wwise events Golpear posts when the
// blow lands (priority-one S1). The names are real Event objects in Kenshi's
// banks (FNV-1 hash vs HIRC type 4): "Impact" (SFX banks, a switch container on
// the "Impacts" group) and the voice, "VO_Get_Hit" for a person or
// "VO_Creature_Take_Damage" for an animal (VO_Bank.bnk).

#include "test_support.h"
#include "beat_hit_sound_rules.h"

#include <string>
#include <vector>

using namespace totalrecruit;

namespace
{
void APersonGetsTheBlowAndTheHumanVoice()
{
    const char* name = "beat hit sound: a person gets Impact with the impact kind, then VO_Get_Hit";
    const std::vector<BeatSoundCue> cues = BeatHitSoundCues("HtH_Impacts", false);
    CHECK(cues.size() == 2);
    if (cues.size() != 2) return;
    CHECK(cues[0].eventName == "Impact");
    CHECK(cues[0].switchGroup == "Impacts");
    CHECK(cues[0].switchValue == "HtH_Impacts");
    CHECK(cues[1].eventName == "VO_Get_Hit");
    CHECK(cues[1].switchGroup.empty());
    CHECK(cues[1].switchValue.empty());
}

void AnAnimalGetsTheCreatureVoice()
{
    const char* name = "beat hit sound: an animal gets VO_Creature_Take_Damage, never the human voice";
    const std::vector<BeatSoundCue> cues = BeatHitSoundCues("HtH_Impacts", true);
    CHECK(cues.size() == 2);
    if (cues.size() != 2) return;
    CHECK(cues[0].eventName == "Impact");
    CHECK(cues[1].eventName == "VO_Creature_Take_Damage");
}

void AnEmptyImpactKindLeavesTheBankDefault()
{
    const char* name = "beat hit sound: an empty or blank impact kind sets no switch (the bank's default plays)";
    const std::vector<BeatSoundCue> cues = BeatHitSoundCues("", false);
    CHECK(cues.size() == 2);
    if (cues.size() != 2) return;
    CHECK(cues[0].eventName == "Impact");
    CHECK(cues[0].switchGroup.empty());
    CHECK(cues[0].switchValue.empty());

    const std::vector<BeatSoundCue> blank = BeatHitSoundCues("  ", false);
    CHECK(blank.size() == 2 && blank[0].switchGroup.empty());
}

void TheImpactKindIsTrimmed()
{
    const char* name = "beat hit sound: the configured impact kind is trimmed, any other value passes as is";
    const std::vector<BeatSoundCue> cues = BeatHitSoundCues(" Heavy_Hit ", false);
    CHECK(cues.size() == 2 && cues[0].switchValue == "Heavy_Hit");
    CHECK(std::string(kBeatImpactKindDefault) == "HtH_Impacts");
}

void TheOldDefaultIsNotAnEvent()
{
    const char* name = "beat hit sound: no cue posts the old non-event names";
    const std::vector<BeatSoundCue> cues = BeatHitSoundCues("Heavy_Hit", false);
    for (size_t i = 0; i < cues.size(); ++i)
    {
        CHECK(cues[i].eventName != "Heavy_Hit");
        CHECK(cues[i].eventName.find("Play_") != 0);
    }
}
} // namespace

void RunBeatHitSoundRulesTests()
{
    APersonGetsTheBlowAndTheHumanVoice();
    AnAnimalGetsTheCreatureVoice();
    AnEmptyImpactKindLeavesTheBankDefault();
    TheImpactKindIsTrimmed();
    TheOldDefaultIsNotAnEvent();
}
