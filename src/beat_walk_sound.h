#pragma once

// The hit sound when Golpear lands (beat-feedback T5), engine side. A port of
// Loot-Scoot-Execute's execute sound (Loot-Scoot-Execute.cpp
// TryPlayCharacterAudioEvent / TryPlayExecuteKillSound): Character::audioEvent(
// name, SOUNDRANGE_ALWAYS), which returns whether the engine accepted the event.
// The names come from beat_walk_rules.h BeatHitSoundCandidates (the configured
// submissionBeatHitSound first, then Loot-Scoot-Execute's list), tried on the
// prisoner first -- the blow is heard where it lands -- and then on the actor, as
// LSE falls back between its two characters. The first one accepted is the sound.
//
// Log (probeOnClick): "[cage] beat hit sound <name> on prisoner|actor", or
// "[cage] beat hit sound: no event accepted" (always: it means the configured
// name is wrong and nothing in the list works on this install).

#include <string>

class Character;

namespace totalrecruit
{
// Plays the hit sound; false when no candidate was accepted on either character.
bool PlayBeatHitSound(Character* prisoner, Character* actor, const std::string& configured);
} // namespace totalrecruit
