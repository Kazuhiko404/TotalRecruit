#pragma once

// The engine reads every module needs, in one place.
//
// Engine pointers can already be freed, and reading one access-violates rather
// than returning null, so every dereference here is guarded. These used to be
// copied into rules, defeats, surrender, mercy, hail, rescue, recruit_core, probe
// and the dialogue bridge, and the copies had started to drift (one checked the
// active platoon, the others did not). One copy means one fix.
//
// Each guard keeps a POD-only frame: C2712 counts every C++ object in the whole
// function, not just the ones inside the __try block, so anything that builds a
// std::string (CharacterName) does it outside the guard.
//
// Anything that decides something from these reads belongs in a *_rules module;
// this file only reads.

#include "common.h"

#include <string>

namespace totalrecruit
{
// The player's faction, or false when there is no player yet.
bool SafePlayerFaction(Faction** out);

// The leader of the player's active squad. False before a squad exists.
bool SafePlayerLeader(Character** out);

// In-game hours, the one clock every timer in the mod runs on (it stops when the
// game is paused).
bool SafeGameHours(double* out);

bool SafeIsPlayerCharacter(Character* character, bool* out);
bool SafeIsAnimal(Character* character, bool* out);
bool SafeIsDead(Character* character, bool* out);
bool SafeIsDown(Character* character, bool* out);

// The engine's own per-character memory ("this one remembers that the player
// ..."). It is saved with the game, which is why the mod uses it at all.
bool SafeMemoryTag(Character* character, Character* about,
                   CharacterPerceptionTags_LongTerm tag, bool* out);
bool SafeRememberTag(Character* character, Character* about,
                     CharacterPerceptionTags_LongTerm tag);

// In a cage of anyone's, shackled, or carried off as a prisoner. Not the same
// as IsInPlayerCage: this does not say whose.
bool SafeInPrison(Character* character, bool* out);

// True when the player's faction owns the cage this character is locked in.
// There is no getCaptor(); this is derived from inSomething and inWhat.
bool IsInPlayerCage(Character* character);

// Display name, or "<unnamed>" when it cannot be read.
std::string CharacterName(Character* character);
} // namespace totalrecruit
