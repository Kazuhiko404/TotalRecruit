#pragma once

class Character;

namespace totalrecruit
{
// Stops hostility, clears temp enemies, and resets AI action so recruited or rescued
// NPCs do not immediately turn and attack the player.
void SafePacify(Character* character, Character* playerLeader = 0);

// The submission event's calm (submission_director.cpp), per character and never a
// faction change: the NPC ends its combat, drops its attack target, and for each of
// the `count` squad members clears its temporary-enemy mark and overwrites its
// short-term memory of them with ST_NONE (TotalSparring fight_starter.cpp
// DisengageFighterEndCombatGuarded and spar_tick.cpp SafeForgetShortTerm); each
// member's own temporary-enemy mark on the NPC is cleared too. No re-think: the
// caller re-issues its own order right after, and a re-think would drop it.
void SafeCalmToward(Character* npc, Character* const* squad, int count);

// Whether the NPC is fighting, or holds a temporary-enemy mark toward any of the
// `count` squad members: what "hostile before the event" means when it is
// restored. False when it cannot be read.
bool SafeHostileToward(Character* npc, Character* const* squad, int count);

// The NPC remembers the player leader as a temporary enemy again
// (rememberCharacter ST_TEMPORARY_ENEMY, TotalSparring fight_starter.cpp
// MarkTempEnemyGuarded). The faction relation is not touched.
void SafeMarkHostile(Character* npc, Character* playerLeader);

// The free helper's peace with the squad (helper.cpp): for each of the `count`
// squad members, the NPC's temporary-enemy mark on them and theirs on the NPC are
// cleared. Nothing else: no endCombatMode (the helper may be fighting for the
// player) and no rememberCharacter (its memory of the squad stays its own).
void SafeClearTempEnemies(Character* npc, Character* const* squad, int count);

// Real combat right now (Character::isInCombatMode, melee or ranged).
bool SafeInCombat(Character* character);
} // namespace totalrecruit
