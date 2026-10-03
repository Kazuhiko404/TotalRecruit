#include "common.h"
#include "character_identity.h"
#include "identity_registry.h"
#include "saved_ints.h"
#include "saved_strings.h"
#include "log.h"

#include <rpc.h>

#pragma warning(push)
#pragma warning(disable: 4091)
#include <kenshi/GameData.h>
#include <kenshi/GameSaveState.h>
#include <kenshi/Character.h>
#pragma warning(pop)

#include <cstring>

#pragma comment(lib, "Rpcrt4.lib")

#ifndef NULL
#define NULL 0
#endif

namespace totalrecruit
{
namespace
{
using identity::Lifetime;

// The save key. Namespace-scope so a guarded frame can pass it by reference
// without materialising a std::string there (C2712).
const std::string kIdentityKey("total_recruit.character_id");
// The per-character ints and strings saved next to the id (saved_ints.h: the
// defeat count, the meals the player gave, ...; saved_strings.h: a temporary
// member's home faction) go through the same two hooks; their keys live in those
// tables.
const std::string kEmptyCategory;

identity::Registry g_registry;
bool g_ready = false;
bool g_attempted = false;
bool g_writeFailureLogged = false;
// Room for the saved_ints.h rows: one "could not write" flag each, and the
// values a load captures (more rows than this would share the last flag and
// lose the overflow on load; the table has five).
const int kMaxSavedIntFields = 8;
bool g_fieldWriteFailureLogged[kMaxSavedIntFields] = { false };
// The same for the saved_strings.h rows (the table has one).
const int kMaxSavedStringFields = 4;
bool g_stringWriteFailureLogged[kMaxSavedStringFields] = { false };
// A saved string's longest value, terminator included (ids and faction string
// ids are well under it).
const std::size_t kSavedStringSize = 128;
// Diagnostics (the in-game relaunch test reads them): ids read back from the
// save, written into it and created since the last world reset.
int g_readFromSave = 0;
int g_writtenToSave = 0;
int g_created = 0;

struct RegistryMutex
{
    CRITICAL_SECTION value;
    RegistryMutex() { InitializeCriticalSection(&value); }
    ~RegistryMutex() { DeleteCriticalSection(&value); }
} g_mutex;

struct Guard
{
    Guard() { EnterCriticalSection(&g_mutex.value); }
    ~Guard() { LeaveCriticalSection(&g_mutex.value); }
private:
    Guard(const Guard&);
    Guard& operator=(const Guard&);
};

void Log(const std::string& message)
{
    LogInfo("[identity] " + message);
}

bool NewId(std::string& id)
{
    id.clear();
    UUID uuid;
    // RPC_S_UUID_LOCAL_ONLY cannot promise uniqueness across machines.
    if (UuidCreate(&uuid) != RPC_S_OK) return false;
    RPC_CSTR text = NULL;
    if (UuidToStringA(&uuid, &text) != RPC_S_OK) return false;
    id.assign(reinterpret_cast<const char*>(text));
    RpcStringFreeA(&text);
    return !id.empty();
}

// ---------------------------------------------------------------------------
// Save-state access. Every engine read is guarded (TR_TRY); every std::string
// lives in an Unguarded helper the guard only jumps through (C2712).

// The GAMESTATE_CHARACTER instance state, never baseData.
GameData* CharacterStateUnguarded(GameSaveState* save)
{
    GameData* state = save->getState(GAMESTATE_CHARACTER);
    return state && state != save->baseData && state->type == GAMESTATE_CHARACTER ? state : NULL;
}

// Kill Counter's location: the character's STATS record.
GameData* StatsStateUnguarded(GameSaveState* save)
{
    GameData* state = save->getState(STATS);
    return state && state != save->baseData ? state : NULL;
}

GameData* SafeState(GameSaveState* save, bool stats)
{
    if (save == NULL) return NULL;
    TR_TRY { return stats ? StatsStateUnguarded(save) : CharacterStateUnguarded(save); }
    TR_CATCH { return NULL; }
}

// One sdata string: the id and every saved_strings.h row share this bridge.
void ReadStringUnguarded(GameData* state, const std::string& key, char* buffer, std::size_t size,
                         bool* found)
{
    typedef boost::unordered::unordered_map<std::string, std::string,
        boost::hash<std::string>, std::equal_to<std::string>,
        Ogre::STLAllocator<std::pair<std::string const, std::string>, Ogre::GeneralAllocPolicy> > Strings;
    const Strings::const_iterator it = state->sdata.find(key);
    if (it != state->sdata.end())
    {
        strncpy_s(buffer, size, it->second.c_str(), _TRUNCATE);
        *found = true;
    }
}

// True only when the state holds the key (its value may be ""). The key is built
// by the caller: this guarded frame holds no C++ object (C2712).
bool SafeReadString(GameData* state, const std::string& key, char* buffer, std::size_t size)
{
    buffer[0] = 0;
    if (state == NULL) return false;
    bool found = false;
    TR_TRY { ReadStringUnguarded(state, key, buffer, size, &found); }
    TR_CATCH { buffer[0] = 0; return false; }
    return found;
}

void WriteStringUnguarded(GameData* state, const std::string& key, const char* value)
{
    state->addString(key, std::string(value), kEmptyCategory, false);
}

bool SafeWriteString(GameData* state, const std::string& key, const char* value)
{
    if (state == NULL) return false;
    TR_TRY { WriteStringUnguarded(state, key, value); }
    TR_CATCH { return false; }
    return true;
}

// The id is a saved string that is never empty.
bool SafeReadId(GameData* state, char* buffer, std::size_t size)
{
    return SafeReadString(state, kIdentityKey, buffer, size) && buffer[0] != 0;
}

bool SafeWriteId(GameData* state, const char* id)
{
    return SafeWriteString(state, kIdentityKey, id);
}

void ReadIntUnguarded(GameData* state, const std::string& key, int* value, bool* found)
{
    typedef boost::unordered::unordered_map<std::string, int,
        boost::hash<std::string>, std::equal_to<std::string>,
        Ogre::STLAllocator<std::pair<std::string const, int>, Ogre::GeneralAllocPolicy> > Ints;
    const Ints::const_iterator it = state->idata.find(key);
    if (it != state->idata.end())
    {
        *value = it->second;
        *found = true;
    }
}

// True only when the state holds the key; `value` is 0 otherwise. The key is
// built by the caller: this guarded frame holds no C++ object (C2712).
bool SafeReadInt(GameData* state, const std::string& key, int* value)
{
    *value = 0;
    if (state == NULL) return false;
    bool found = false;
    TR_TRY { ReadIntUnguarded(state, key, value, &found); }
    TR_CATCH { *value = 0; return false; }
    return found;
}

void WriteIntUnguarded(GameData* state, const std::string& key, int value)
{
    state->add(key, value, kEmptyCategory, false);
}

bool SafeWriteInt(GameData* state, const std::string& key, int value)
{
    if (state == NULL) return false;
    TR_TRY { WriteIntUnguarded(state, key, value); }
    TR_CATCH { return false; }
    return true;
}

// The id stored in a save state: the character instance state first, then the
// STATS record.
std::string ReadSavedId(GameSaveState* save)
{
    char buffer[128];
    if (SafeReadId(SafeState(save, false), buffer, sizeof(buffer)))
        return std::string(buffer);
    if (SafeReadId(SafeState(save, true), buffer, sizeof(buffer)))
        return std::string(buffer);
    return std::string();
}

// Writes the id into both states and reads it back; true when at least one of
// them holds it afterwards.
bool WriteSavedId(GameSaveState* save, const std::string& id)
{
    bool stored = false;
    for (int pass = 0; pass < 2; ++pass)
    {
        GameData* state = SafeState(save, pass == 1);
        char check[128];
        if (SafeWriteId(state, id.c_str()) && SafeReadId(state, check, sizeof(check)) && id == check)
            stored = true;
    }
    return stored;
}

// A saved int (saved_ints.h) stored in a save state, in the same order as the
// id. False when neither state holds it (a character saved before that field
// existed, or never touched by the mod).
bool ReadSavedInt(GameSaveState* save, const std::string& key, int* value)
{
    if (SafeReadInt(SafeState(save, false), key, value)) return true;
    return SafeReadInt(SafeState(save, true), key, value);
}

// Writes the value into both states the id went into; true when at least one
// of them reads it back.
bool WriteSavedInt(GameSaveState* save, const std::string& key, int value)
{
    bool stored = false;
    for (int pass = 0; pass < 2; ++pass)
    {
        GameData* state = SafeState(save, pass == 1);
        int check = 0;
        if (SafeWriteInt(state, key, value) && SafeReadInt(state, key, &check) && check == value)
            stored = true;
    }
    return stored;
}

// Every saved_ints.h field of the character carrying `id`, into the save state.
// Outside the registry lock: each module's table has its own. A failed write is
// logged once per field.
void WriteSavedInts(GameSaveState* save, const std::string& id)
{
    for (int i = 0; i < SavedIntFieldCount(); ++i)
    {
        const SavedIntField& field = SavedIntFieldAt(i);
        const std::string key(field.key);
        if (WriteSavedInt(save, key, field.saved(id)))
            continue;
        const int flag = (i < kMaxSavedIntFields) ? i : kMaxSavedIntFields - 1;
        bool firstFailure = false;
        {
            Guard lock;
            if (!g_fieldWriteFailureLogged[flag])
            {
                g_fieldWriteFailureLogged[flag] = true;
                firstFailure = true;
            }
        }
        if (firstFailure)
            Log("could not write " + key + " into a save state; that character's value restarts at 0 "
                "after a reload.");
    }
}

// A saved string (saved_strings.h) stored in a save state, in the same order as
// the id. False when neither state holds it.
bool ReadSavedString(GameSaveState* save, const std::string& key, std::string* value)
{
    char buffer[kSavedStringSize];
    if (SafeReadString(SafeState(save, false), key, buffer, sizeof(buffer)) ||
        SafeReadString(SafeState(save, true), key, buffer, sizeof(buffer)))
    {
        *value = buffer;
        return true;
    }
    return false;
}

// Writes the value into both states the id went into; true when at least one of
// them reads it back.
bool WriteSavedString(GameSaveState* save, const std::string& key, const std::string& value)
{
    bool stored = false;
    for (int pass = 0; pass < 2; ++pass)
    {
        GameData* state = SafeState(save, pass == 1);
        char check[kSavedStringSize];
        if (SafeWriteString(state, key, value.c_str()) &&
            SafeReadString(state, key, check, sizeof(check)) && value == check)
            stored = true;
    }
    return stored;
}

// Every saved_strings.h field of the character carrying `id`, into the save
// state; a failed write is logged once per field.
void WriteSavedStrings(GameSaveState* save, const std::string& id)
{
    for (int i = 0; i < SavedStringFieldCount(); ++i)
    {
        const SavedStringField& field = SavedStringFieldAt(i);
        const std::string key(field.key);
        if (WriteSavedString(save, key, field.saved(id)))
            continue;
        const int flag = (i < kMaxSavedStringFields) ? i : kMaxSavedStringFields - 1;
        bool firstFailure = false;
        {
            Guard lock;
            if (!g_stringWriteFailureLogged[flag])
            {
                g_stringWriteFailureLogged[flag] = true;
                firstFailure = true;
            }
        }
        if (firstFailure)
            Log("could not write " + key + " into a save state; that character's value is lost "
                "after a reload.");
    }
}

// Captures every saved_strings.h field the loading save state holds, before the
// native loader runs.
struct LoadedString
{
    int field;
    std::string value;
};

int ReadSavedStrings(GameSaveState* save, LoadedString* out, int capacity)
{
    int found = 0;
    for (int i = 0; i < SavedStringFieldCount() && found < capacity; ++i)
    {
        std::string value;
        if (ReadSavedString(save, std::string(SavedStringFieldAt(i).key), &value))
        {
            out[found].field = i;
            out[found].value = value;
            ++found;
        }
    }
    return found;
}

// Captures every saved_ints.h field the loading save state holds, before the
// native loader runs (it may destroy the state).
struct LoadedInt
{
    int field;
    int value;
};

int ReadSavedInts(GameSaveState* save, LoadedInt* out, int capacity)
{
    int found = 0;
    for (int i = 0; i < SavedIntFieldCount() && found < capacity; ++i)
    {
        int value = 0;
        if (ReadSavedInt(save, std::string(SavedIntFieldAt(i).key), &value))
        {
            out[found].field = i;
            out[found].value = value;
            ++found;
        }
    }
    return found;
}

// ---------------------------------------------------------------------------
// Hooks. KenshiLib::AddHook chains: every mod that hooks the same function gets
// the previous detour as its "original" and must call it exactly once, with the
// arguments untouched, which each hook below does (TotalSparring, Proving
// Grounds and Kill Counter hook these same three functions).

// Installed x64 member ABI: RCX=this, RDX=hidden result, R8=container,
// R9=refs, stack+0x28=offset; RAX=result. Kill Counter notes the order of the
// first two has differed across builds, so both are forwarded untouched and
// `this` is the one that is not the returned slot.
typedef GameSaveState* (*SerialiseFn)(void*, void*, GameDataContainer*, GameData*, PosRotPair*);
SerialiseFn g_serialiseOriginal = NULL;
void (*g_loadOriginal)(Character*, GameSaveState*) = NULL;
void (*g_destroyOriginal)(Character*) = NULL;

GameSaveState* Serialise(void* arg1, void* arg2, GameDataContainer* container,
                         GameData* refs, PosRotPair* offset)
{
    GameSaveState* out = g_serialiseOriginal(arg1, arg2, container, refs, offset);
    if (out == NULL) return out;
    const void* self = (out == static_cast<GameSaveState*>(arg1)) ? arg2 : arg1;

    std::string id;
    {
        Guard lock;
        // Native callbacks may reenter load/destruction: never hold the lock
        // across the original (taken after it returned).
        if (!g_ready || !g_registry.SavedId(self, g_registry.Current(self), id)) return out;
    }
    if (!WriteSavedId(out, id))
    {
        if (!g_writeFailureLogged)
        {
            g_writeFailureLogged = true;
            Log("could not write the character id into a save state; that character will get a new id "
                "after a relaunch (its state restarts).");
        }
        return out;
    }
    WriteSavedInts(out, id);
    WriteSavedStrings(out, id);
    bool first = false;
    {
        Guard lock;
        first = (g_writtenToSave++ == 0);
    }
    if (first)
        Log("first character id written into a save state (" + id + ").");
    return out;
}

void Load(Character* self, GameSaveState* save)
{
    bool active;
    { Guard lock; active = g_ready; }
    if (active)
    {
        // The native loader may destroy save; capture and bind beforehand.
        const std::string id = ReadSavedId(save);
        LoadedInt loaded[kMaxSavedIntFields];
        const int loadedCount = ReadSavedInts(save, loaded, kMaxSavedIntFields);
        LoadedString loadedStrings[kMaxSavedStringFields];
        const int loadedStringCount = ReadSavedStrings(save, loadedStrings, kMaxSavedStringFields);
        bool conflict = false;
        bool first = false;
        {
            Guard lock;
            if (g_ready)
            {
                g_registry.Load(self, save != NULL, id, conflict);
                if (save != NULL && !id.empty())
                    first = (g_readFromSave++ == 0);
            }
        }
        // Every saved-int and saved-string table was cleared at load begin
        // (load_boundary); this puts back exactly what the loaded save held for
        // this character.
        if (!id.empty())
        {
            for (int i = 0; i < loadedCount; ++i)
                SavedIntFieldAt(loaded[i].field).restore(id, loaded[i].value);
            for (int i = 0; i < loadedStringCount; ++i)
                SavedStringFieldAt(loadedStrings[i].field).restore(id, loadedStrings[i].value);
        }
        if (first)
            Log("first character id read back from the save (" + id + ").");
        if (conflict)
            Log("duplicate live character id " + id + "; both fall back to their handle key until a reload.");
    }
    g_loadOriginal(self, save);
}

void Destroy(Character* self)
{
    {
        Guard lock;
        // Non-deleting Character::_DESTRUCTOR. Erase before native destruction.
        g_registry.End(self, g_registry.Current(self));
    }
    g_destroyOriginal(self);
}
} // namespace

namespace CharacterIdentity
{
bool InstallHooks()
{
    {
        Guard lock;
        if (g_attempted) return g_ready;
        g_attempted = true;
    }
    bool ok = KenshiLib::SUCCESS == KenshiLib::AddHook(
        KenshiLib::GetRealAddress(&Character::_DESTRUCTOR), Destroy, &g_destroyOriginal);
    if (ok) ok = g_destroyOriginal && KenshiLib::SUCCESS == KenshiLib::AddHook(
        KenshiLib::GetRealAddress(&Character::_NV_loadFromSerialise), Load, &g_loadOriginal);
    if (ok) ok = g_loadOriginal && KenshiLib::SUCCESS == KenshiLib::AddHook(
        KenshiLib::GetRealAddress(&Character::_NV_serialise), Serialise, &g_serialiseOriginal);
    ok = ok && g_serialiseOriginal != NULL;
    { Guard lock; g_ready = ok; }
    if (ok)
        Log("Hooked Character::_DESTRUCTOR, _NV_loadFromSerialise, _NV_serialise (persisted character id).");
    else
        LogError("[identity] hook installation failed; per-character state stays keyed by handle "
                 "(it changes on a game relaunch).");
    return ok;
}

bool IsReady()
{
    Guard lock;
    return g_ready;
}

bool Lookup(Character* character, std::string& id)
{
    id.clear();
    Guard lock;
    if (!g_ready || !character) return false;
    const Lifetime lifetime = g_registry.Current(character);
    return lifetime != 0 && g_registry.Find(character, lifetime, id);
}

bool GetOrCreate(Character* character, std::string& id)
{
    id.clear();
    Lifetime lifetime;
    {
        Guard lock;
        if (!g_ready || !character) return false;
        lifetime = g_registry.Current(character);
        if (!lifetime) lifetime = g_registry.Begin(character);
        std::string existing;
        if (g_registry.SavedId(character, lifetime, existing))
            return g_registry.Find(character, lifetime, id);
    }
    std::string generated;
    if (!NewId(generated)) { Log("Windows UUID generation failed"); return false; }
    bool conflict = false;
    bool found = false;
    bool created = false;
    {
        Guard lock;
        if (!g_ready || g_registry.Current(character) != lifetime) return false;
        std::string existing;
        if (!g_registry.SavedId(character, lifetime, existing))
        {
            g_registry.Bind(character, lifetime, generated, conflict);
            created = (g_created++ == 0);
        }
        found = g_registry.Find(character, lifetime, id);
    }
    if (conflict) Log("duplicate generated character id " + generated);
    if (created) Log("first character id created this session (" + generated + "); it reaches the save at the next save.");
    return found;
}

void AbandonWorldState()
{
    int readCount;
    int writtenCount;
    int createdCount;
    {
        Guard lock;
        g_registry.Clear();
        readCount = g_readFromSave;
        writtenCount = g_writtenToSave;
        createdCount = g_created;
        g_readFromSave = 0;
        g_writtenToSave = 0;
        g_created = 0;
    }
    char line[200];
    _snprintf_s(line, sizeof(line), _TRUNCATE,
        "world reset at load begin; the previous world read %d id(s) from its save, created %d and wrote %d.",
        readCount, createdCount, writtenCount);
    Log(line);
}
} // namespace CharacterIdentity
} // namespace totalrecruit
