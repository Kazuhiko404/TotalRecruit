#pragma once

// The per-character strings this mod keeps inside the Kenshi save.
//
// The string twin of saved_ints.h: character_identity.cpp writes every row as an
// sdata string next to the persisted id (_NV_serialise) and reads it back on load
// (_NV_loadFromSerialise), one loop over this table, through the same read/write
// bridge the id itself uses. A module that needs a saved string adds one row here
// (its key and its two callbacks), nothing in character_identity.
//
//   total_recruit.temp_home_faction   temp_member.h   the faction a temporary
//                                                     member goes back to
//
// Contract for each row (the same as saved_ints.h):
//   saved(id)          the value to write for the character carrying `id`. Always
//                      written, "" included: a state that once held a value must
//                      not keep a stale one.
//   restore(id, value) the value a loading save state held; only called when the
//                      key is present. The module cleared its table at load begin
//                      (load_boundary.cpp).
// Both may run off the game thread (the serialise hook): they must be
// thread-safe.

#include <string>

namespace totalrecruit
{
struct SavedStringField
{
    const char* key;
    std::string (*saved)(const std::string& persistedId);
    void (*restore)(const std::string& persistedId, const std::string& value);
};

// How many rows there are, and one of them (0 <= index < count).
int SavedStringFieldCount();
const SavedStringField& SavedStringFieldAt(int index);
} // namespace totalrecruit
