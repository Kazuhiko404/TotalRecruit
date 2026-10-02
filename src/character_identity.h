#pragma once

// Persisted character identity (T7): a UUID per character, stored INSIDE the
// Kenshi save, so a character keeps the same key across a game relaunch. The
// engine handle (index + serial) that every per-character map used before is
// reassigned on a relaunch, so a prisoner's defeat count, hail clock, mercy
// flag and -- above all -- the seed of the stable join roll were all quietly
// reset (and the roll recast) every time the game was restarted.
//
// Port of TotalSparring's FighterIdentity (src/shared/game/fighter_identity.*),
// itself a port of Proving Grounds': the same three Character hooks
// (_NV_serialise writes the id into the save state, _NV_loadFromSerialise reads
// it back and binds it BEFORE the native loader runs, _DESTRUCTOR unbinds), the
// same pure registry (identity_registry.h) and UuidCreate ids. Save key
// "total_recruit.character_id" -- distinct from TotalSparring's
// "total_sparring.fighter_id" and Proving Grounds' "proving_grounds.fighter_id"
// so all three mods can coexist on one character.
//
// Deliberate behaviour (same as TotalSparring):
//   - NOT fail-closed. If a hook cannot be installed IsReady() is false and
//     char_key_reader falls back to the handle key, so the mod keeps working
//     with the pre-T7 relaunch limitation. A failed save-state write is logged
//     once and never turns identity off mid-session (switching keys mid-session
//     would split a character's state the way the relaunch did).
//   - The id is written into the GAMESTATE_CHARACTER instance state AND the
//     STATS state (where Kill Counter keeps per-character data and which it
//     proves is saved); loading reads the former first.
//   - The serialise hook recovers `this` the Kill Counter way: the one of its
//     first two arguments that is not the returned save-state slot.

#include <string>

class Character;

namespace totalrecruit
{
namespace CharacterIdentity
{
// Installs the three Character hooks once. False (and IsReady() false) when any
// of them failed; hooks already placed then stay inert passthroughs (KenshiLib
// has no rollback).
bool InstallHooks();

// True when every hook was installed.
bool IsReady();

// The character's persisted id, creating (UuidCreate) and binding one when it
// has none yet; it reaches the save at the next save. False when not ready, on a
// null character, on a UUID failure, or when the id is conflicted (two live
// characters loaded with the same id).
bool GetOrCreate(Character* character, std::string& id);

// The character's id if it already has one; never creates. Lets a read-only
// question ("is this prisoner being held?") avoid minting ids for every
// bystander.
bool Lookup(Character* character, std::string& id);

// Call at load/import BEGIN (load_boundary), before old objects are discarded
// or new characters load. Never at completion: loaded bindings must survive
// world activation. Dereferences no character.
void AbandonWorldState();
} // namespace CharacterIdentity
} // namespace totalrecruit
