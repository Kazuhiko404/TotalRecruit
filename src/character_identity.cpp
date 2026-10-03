#include "common.h"
#include "character_identity.h"
#include "identity_registry.h"
#include "defeats.h"
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
// The player-defeat count (defeats.h / defeat_ledger.h), saved next to the id so
// a load restores exactly what that save knew. Always written with the id, 0
// included: a state that once held a count must not keep a stale one.
const std::string kDefeatsKey("total_recruit.player_defeats");
const std::string kEmptyCategory;

identity::Registry g_registry;
bool g_ready = false;
bool g_attempted = false;
bool g_writeFailureLogged = false;
bool g_defeatWriteFailureLogged = false;
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

void ReadIdUnguarded(GameData* state, char* buffer, std::size_t size)
{
    typedef boost::unordered::unordered_map<std::string, std::string,
        boost::hash<std::string>, std::equal_to<std::string>,
        Ogre::STLAllocator<std::pair<std::string const, std::string>, Ogre::GeneralAllocPolicy> > Strings;
    const Strings::const_iterator found = state->sdata.find(kIdentityKey);
    if (found != state->sdata.end())
        strncpy_s(buffer, size, found->second.c_str(), _TRUNCATE);
}

bool SafeReadId(GameData* state, char* buffer, std::size_t size)
{
    buffer[0] = 0;
    if (state == NULL) return false;
    TR_TRY { ReadIdUnguarded(state, buffer, size); }
    TR_CATCH { buffer[0] = 0; return false; }
    return buffer[0] != 0;
}

void WriteIdUnguarded(GameData* state, const char* id)
{
    state->addString(kIdentityKey, std::string(id), kEmptyCategory, false);
}

bool SafeWriteId(GameData* state, const char* id)
{
    if (state == NULL) return false;
    TR_TRY { WriteIdUnguarded(state, id); }
    TR_CATCH { return false; }
    return true;
}

void ReadDefeatsUnguarded(GameData* state, int* count, bool* found)
{
    typedef boost::unordered::unordered_map<std::string, int,
        boost::hash<std::string>, std::equal_to<std::string>,
        Ogre::STLAllocator<std::pair<std::string const, int>, Ogre::GeneralAllocPolicy> > Ints;
    const Ints::const_iterator it = state->idata.find(kDefeatsKey);
    if (it != state->idata.end())
    {
        *count = it->second;
        *found = true;
    }
}

// True only when the state holds the key; `count` is 0 otherwise.
bool SafeReadDefeats(GameData* state, int* count)
{
    *count = 0;
    if (state == NULL) return false;
    bool found = false;
    TR_TRY { ReadDefeatsUnguarded(state, count, &found); }
    TR_CATCH { *count = 0; return false; }
    return found;
}

void WriteDefeatsUnguarded(GameData* state, int count)
{
    state->add(kDefeatsKey, count, kEmptyCategory, false);
}

bool SafeWriteDefeats(GameData* state, int count)
{
    if (state == NULL) return false;
    TR_TRY { WriteDefeatsUnguarded(state, count); }
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

// The defeat count stored in a save state, in the same order as the id. False
// when neither state holds it (a character saved before this field existed, or
// never touched by the mod).
bool ReadSavedDefeats(GameSaveState* save, int* count)
{
    if (SafeReadDefeats(SafeState(save, false), count)) return true;
    return SafeReadDefeats(SafeState(save, true), count);
}

// Writes the count into both states the id went into; true when at least one
// of them reads it back.
bool WriteSavedDefeats(GameSaveState* save, int count)
{
    bool stored = false;
    for (int pass = 0; pass < 2; ++pass)
    {
        GameData* state = SafeState(save, pass == 1);
        int check = 0;
        if (SafeWriteDefeats(state, count) && SafeReadDefeats(state, &check) && check == count)
            stored = true;
    }
    return stored;
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
    // Outside the registry lock: the ledger has its own.
    const int defeats = SavedDefeatCount(id);
    const bool defeatsStored = WriteSavedDefeats(out, defeats);
    bool first = false;
    bool defeatFailureFirst = false;
    {
        Guard lock;
        first = (g_writtenToSave++ == 0);
        if (!defeatsStored && !g_defeatWriteFailureLogged)
        {
            g_defeatWriteFailureLogged = true;
            defeatFailureFirst = true;
        }
    }
    if (first)
        Log("first character id written into a save state (" + id + ").");
    if (defeatFailureFirst)
        Log("could not write the player-defeat count into a save state; that character's defeats "
            "restart at 0 after a reload.");
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
        int defeats = 0;
        const bool hasDefeats = ReadSavedDefeats(save, &defeats);
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
        // The ledger was cleared at load begin (load_boundary); this puts back
        // exactly what the loaded save held for this character.
        if (hasDefeats && !id.empty())
            RestoreSavedDefeatCount(id, defeats);
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
