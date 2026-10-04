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

// Whether the game is paused (GameWorld::isPaused). The submission event's clock
// stops while it is.
bool SafeIsPaused(bool* out);

// Every character the player controls (PlayerInterface::playerCharacters), up to
// `max`, into `out`. Returns how many were written; 0 before there is a player.
int SafePlayerCharacters(Character** out, int max);

bool SafeIsPlayerCharacter(Character* character, bool* out);
bool SafeIsAnimal(Character* character, bool* out);
bool SafeIsDead(Character* character, bool* out);
bool SafeIsDown(Character* character, bool* out);

// Out cold, the way TotalSparring's FighterDownState reads it: the engine's own
// "literally unconscious, not pretending" or the KO prone state. isDown alone is
// true for anyone who cannot stand, a wide-awake cripple included.
bool SafeIsKnockedOut(Character* character, bool* out);

// The task the character is on right now (its body's current action), as the
// engine's TaskType in an int; NULL_TASK when idle.
bool SafeCurrentTask(Character* character, int* out);

// The engine's own per-character memory ("this one remembers that the player
// ..."). It is saved with the game, which is why the mod uses it at all.
bool SafeMemoryTag(Character* character, Character* about,
                   CharacterPerceptionTags_LongTerm tag, bool* out);
bool SafeRememberTag(Character* character, Character* about,
                     CharacterPerceptionTags_LongTerm tag);

// PS_PLAYING_DEAD is a real prone state, so "unconscious" alone would let the player
// farm recruits off NPCs that are only pretending.
bool SafeIsPretending(Character* character, bool* out);

// True when this character is dying rather than merely knocked out, and is not
// faking it (PS_PLAYING_DEAD exists).
bool IsGenuinelyDying(Character* character);

// In a cage of anyone's, shackled, or carried off as a prisoner. Not the same
// as IsInPlayerCage: this does not say whose.
bool SafeInPrison(Character* character, bool* out);

// True when the player's faction owns the cage this character is locked in.
// There is no getCaptor(); this is derived from inSomething and inWhat.
bool IsInPlayerCage(Character* character);

// Every character within `radius` of `centre`, as the engine lists them (the world
// query returns any object; non-characters are dropped). Reads only.
bool CharactersNear(const Ogre::Vector3& centre, float radius, std::vector<Character*>* out);

// Whether the one who last put this character down (Character::lastGuyWhoDefeatedMe,
// the engine's own answer) is one of the player's characters. False when there is
// no readable winner. The defeat count (defeats.cpp) and a temporary member's
// knockout strike (temp_member.cpp) both ask it.
bool SafeDefeatedByPlayer(Character* character, bool* out);
// Whether any of the player's characters (other than this one) is attacking this
// character right now (Character::getAllAttackers).
bool SafeAttackedByPlayer(Character* character, bool* out);
// The raw hunger value (MedicalSystem::hunger: about 3 when fed, see
// chance_rules HungerToFullness01) and the engine's own "starving" (isReallyHungry
// or isHungerKO).
bool SafeHunger(Character* character, float* rawHunger, bool* starving);
// The string id of the faction this character belongs to (its GameData sid, the
// key FactionManager::getFactionByStringID takes); "" when it cannot be read.
std::string FactionStringId(Character* character);
// The character the player has selected (PlayerInterface::selectedObject), the
// player's own included. NULL when nothing, or not a character, is selected.
Character* SafeSelectedCharacter();
// The character `who` is attacking right now (Character::getAttackTarget), or false
// when there is none or it cannot be read. The free helper fights it (helper.cpp).
bool SafeAttackTargetOf(Character* who, Character** out);
// The raw-pointer answer to "is this object a character?" for one RootObject.
bool SafeAsCharacter(RootObject* object, Character** out);

// Display name, or "<unnamed>" when it cannot be read.
std::string CharacterName(Character* character);
} // namespace totalrecruit
