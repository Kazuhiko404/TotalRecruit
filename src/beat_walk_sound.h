#pragma once

// The hit sound when Golpear lands, engine side (beat-feedback T5, reworked in
// priority-one S1). The cues come from beat_hit_sound_rules.h BeatHitSoundCues:
// "Impact" with the "Impacts" switch set to submissionBeatHitSound, then the
// voice ("VO_Get_Hit", or "VO_Creature_Take_Damage" for an animal). All of them
// are posted on the prisoner, where the blow lands.
//
// Each cue goes through the game's Wwise directly (wwise_bridge.h): SetSwitch,
// then PostEvent, whose playing id tells whether it really played. When the
// bridge cannot be used (export missing, no audio object), it falls back to
// Character::audioValue + audioEvent, which cannot tell a failure.
//
// Log (probeOnClick): "[cage] beat hit sound Impact (Impacts=HtH_Impacts) played
// id 123" or "... via audioEvent (unconfirmed)"; always: "[cage] beat hit sound
// <event> failed: PostEvent returned 0" (the event or the switch value is not in
// the loaded banks).

#include <string>

class Character;

namespace totalrecruit
{
// Plays the blow's cues on the prisoner; false when none of them is known to play.
bool PlayBeatHitSound(Character* prisoner, const std::string& impactKind);
} // namespace totalrecruit
