#include "common.h"
#include "ts_bridge.h"
#include "char_key_reader.h"
#include "engine_reads.h"
#include "log.h"

#include <cstddef>
#include <cstdio>
#include <deque>

#include <tlhelp32.h>

#include "third_party/ts_api.h"

namespace totalrecruit
{
namespace
{
const char* const kTsExport = "TS_GetApi";
// How often the frame tick tries to attach, and how many tries before TotalSparring
// counts as absent for the session (every plugin is loaded long before the first
// frame, so a handful is plenty).
const DWORD kAttachRetryMs = 2000;
const int kAttachTries = 15;
// The queue between TotalSparring's callback and the next frame. A burst larger than
// this drops the oldest bout (logged).
const size_t kMaxQueuedBouts = 32;

enum AttachState
{
    Attach_Pending,
    Attach_Attached,
    Attach_Absent
};

// Game-thread state: written by TsBridgeTick / ShutdownTsBridge only.
AttachState g_state = Attach_Pending;
const TS_ApiV1* g_api = NULL;
bool g_listening = false;
int g_tries = 0;
DWORD g_lastTry = 0;

// ------------------------------------------------------------- Bout queue
struct QueueMutex
{
    CRITICAL_SECTION value;
    QueueMutex() { InitializeCriticalSection(&value); }
    ~QueueMutex() { DeleteCriticalSection(&value); }
} g_queueMutex;

// RAII: held for the scope, released on every path out of it. Nothing foreign runs
// while it is held.
struct QueueLock
{
    QueueLock() { EnterCriticalSection(&g_queueMutex.value); }
    ~QueueLock() { LeaveCriticalSection(&g_queueMutex.value); }

private:
    QueueLock(const QueueLock&);
    QueueLock& operator=(const QueueLock&);
};

std::deque<DuelBoutRecord> g_queue;
// Bouts dropped because the queue was full; reported by the frame tick.
int g_dropped = 0;

// ------------------------------------------------- Guarded calls into TS
// Each one is a plain frame (no C++ objects) around one call through the table, so
// a fault inside TotalSparring reads as TS_ERR_INTERNAL instead of taking the game
// down.
int GuardedGetApi(TS_GetApiFn getApi, const TS_ApiV1** out)
{
    __try
    {
        return getApi(TS_API_VERSION, out);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        *out = NULL;
        return TS_ERR_INTERNAL;
    }
}

int GuardedIsReady(const TS_ApiV1* api)
{
    __try
    {
        return api->IsReady();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }
}

int GuardedDuelStats(const TS_ApiV1* api, Character* a, Character* b, TS_DuelStats* out)
{
    __try
    {
        return api->GetDuelStats(a, b, out);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return TS_ERR_INTERNAL;
    }
}

int GuardedRegister(const TS_ApiV1* api, TS_BoutCallback cb)
{
    __try
    {
        return api->RegisterBoutListener(cb, NULL);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return TS_ERR_INTERNAL;
    }
}

int GuardedUnregister(const TS_ApiV1* api, TS_BoutCallback cb)
{
    __try
    {
        return api->UnregisterBoutListener(cb, NULL);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return TS_ERR_INTERNAL;
    }
}

int GuardedOpenDuel(const TS_ApiV1* api, Character* npc, Character* challenger)
{
    __try
    {
        return api->OpenDuelPopup(npc, challenger);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return TS_ERR_INTERNAL;
    }
}

std::string Code(int value)
{
    char buffer[16] = { 0 };
    _snprintf_s(buffer, sizeof(buffer), _TRUNCATE, "%d", value);
    return std::string(buffer);
}

// ------------------------------------------------------------- The callback
DuelBoutOutcome OutcomeOf(int outcome)
{
    switch (outcome)
    {
    case TS_OUTCOME_A_WON: return DuelBout_AWon;
    case TS_OUTCOME_B_WON: return DuelBout_BWon;
    case TS_OUTCOME_DRAW:  return DuelBout_Draw;
    default:               return DuelBout_Undecided;
    }
}

// The fields v1 promises, up to and including `outcome`.
const size_t kBoutEventMinSize = offsetof(TS_BoutEvent, outcome) + sizeof(int);

// Copies one event while its pointers are still valid: keys and "is side A mine" are
// read now, nothing else is done.
void CopyBoutUnguarded(const TS_BoutEvent* ev)
{
    if (ev == NULL || ev->size < kBoutEventMinSize)
    {
        return;
    }
    DuelBoutRecord record;
    record.isDuel = ev->isDuel != 0;
    record.outcome = OutcomeOf(ev->outcome);
    if (ev->a != NULL)
    {
        record.aKnown = SafeFindCharKey(ev->a, &record.a);
        bool mine = false;
        record.aIsSquadMember = SafeIsPlayerCharacter(ev->a, &mine) && mine;
    }
    if (ev->b != NULL)
    {
        record.bKnown = SafeFindCharKey(ev->b, &record.b);
    }

    QueueLock lock;
    if (g_queue.size() >= kMaxQueuedBouts)
    {
        g_queue.pop_front();
        ++g_dropped;
    }
    g_queue.push_back(record);
}

void GuardedCopyBout(const TS_BoutEvent* ev)
{
    __try
    {
        CopyBoutUnguarded(ev);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

void __cdecl OnBout(void* /*user*/, const TS_BoutEvent* ev)
{
    GuardedCopyBout(ev);
}

// --------------------------------------------------------------- Attaching
// TotalSparring by its module name, or any module exporting TS_GetApi (a renamed
// DLL), the way mod_hub_integration.cpp finds Mod Hub.
TS_GetApiFn FindGetApi()
{
    const char* names[] = { "TotalSparring.dll", "TotalSparring" };
    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i)
    {
        HMODULE module = GetModuleHandleA(names[i]);
        if (module == NULL)
        {
            continue;
        }
        FARPROC proc = GetProcAddress(module, kTsExport);
        if (proc != NULL)
        {
            return (TS_GetApiFn)proc;
        }
    }

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32,
                                           GetCurrentProcessId());
    if (snap == INVALID_HANDLE_VALUE)
    {
        return NULL;
    }
    TS_GetApiFn found = NULL;
    MODULEENTRY32 entry;
    entry.dwSize = sizeof(entry);
    if (Module32First(snap, &entry))
    {
        do
        {
            FARPROC proc = GetProcAddress(entry.hModule, kTsExport);
            if (proc != NULL)
            {
                found = (TS_GetApiFn)proc;
                break;
            }
        } while (Module32Next(snap, &entry));
    }
    CloseHandle(snap);
    return found;
}

bool TableUsable(const TS_ApiV1* api)
{
    return api != NULL && api->size >= sizeof(TS_ApiV1) && api->version >= 1u &&
           api->GetDuelStats != NULL && api->RegisterBoutListener != NULL &&
           api->UnregisterBoutListener != NULL && api->OpenDuelPopup != NULL &&
           api->IsReady != NULL;
}

void GiveUp(const std::string& why)
{
    g_state = Attach_Absent;
    g_api = NULL;
    LogInfo("[ts-bridge] TotalSparring duels unavailable: " + why);
}

void TryAttach()
{
    const TS_GetApiFn getApi = FindGetApi();
    if (getApi == NULL)
    {
        if (g_tries >= kAttachTries)
        {
            GiveUp("TotalSparring.dll is not loaded (or has no TS_GetApi export).");
        }
        return;
    }

    const TS_ApiV1* api = NULL;
    const int rc = GuardedGetApi(getApi, &api);
    if (rc != TS_OK || api == NULL)
    {
        GiveUp("TS_GetApi(" + Code((int)TS_API_VERSION) + ") returned " + Code(rc) + ".");
        return;
    }
    if (!TableUsable(api))
    {
        GiveUp("the API table is too small or incomplete.");
        return;
    }

    g_api = api;
    g_state = Attach_Attached;
    const int reg = GuardedRegister(api, &OnBout);
    g_listening = (reg == TS_OK);
    if (!g_listening)
    {
        LogError("[ts-bridge] TotalSparring refused the bout listener (" + Code(reg) +
                 "): duel results cannot be read, so the duel challenge stays off.");
        return;
    }
    LogInfo("[ts-bridge] attached to TotalSparring API v" + Code((int)api->version) +
            " and listening for bouts.");
}

// The listener lives in TotalSparring's module: unregistering only makes sense (and is
// only safe) while that module is still mapped.
bool TsModuleLoaded(const TS_ApiV1* api)
{
    if (api == NULL)
    {
        return false;
    }
    HMODULE owner = NULL;
    return GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                  GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                              (LPCSTR)api, &owner) != FALSE &&
           owner != NULL;
}

const TS_ApiV1* AttachedApi()
{
    return g_state == Attach_Attached ? g_api : NULL;
}
} // namespace

void TsBridgeTick()
{
    if (g_state != Attach_Pending)
    {
        return;
    }
    const DWORD now = GetTickCount();
    if (g_tries > 0 && now - g_lastTry < kAttachRetryMs)
    {
        return;
    }
    g_lastTry = now;
    ++g_tries;
    TryAttach();
}

bool TsAvailable()
{
    const TS_ApiV1* api = AttachedApi();
    return api != NULL && g_listening && GuardedIsReady(api) != 0;
}

bool TsDuelStats(Character* squadMember, Character* npc, int* wins, int* losses, int* draws)
{
    const TS_ApiV1* api = AttachedApi();
    if (api == NULL || squadMember == NULL || npc == NULL || wins == NULL ||
        losses == NULL || draws == NULL)
    {
        return false;
    }
    TS_DuelStats stats;
    stats.size = sizeof(TS_DuelStats);
    stats.wins = 0;
    stats.losses = 0;
    stats.draws = 0;
    const int rc = GuardedDuelStats(api, squadMember, npc, &stats);
    if (rc != TS_OK)
    {
        if (rc != TS_ERR_NOT_READY && rc != TS_ERR_UNKNOWN_FIGHTER)
        {
            LogError("[ts-bridge] GetDuelStats returned " + Code(rc) + ".");
        }
        return false;
    }
    *wins = stats.wins;
    *losses = stats.losses;
    *draws = stats.draws;
    return true;
}

TsOpenResult TsOpenDuel(Character* npc, Character* challenger)
{
    const TS_ApiV1* api = AttachedApi();
    if (api == NULL || npc == NULL)
    {
        return TsOpen_Absent;
    }
    const int rc = GuardedOpenDuel(api, npc, challenger);
    switch (rc)
    {
    case TS_OK:            return TsOpen_Opened;
    case TS_ERR_NOT_READY: return TsOpen_NotReady;
    case TS_ERR_BUSY:      return TsOpen_Busy;
    case TS_ERR_REFUSED:   return TsOpen_Refused;
    default:
        LogError("[ts-bridge] OpenDuelPopup returned " + Code(rc) + ".");
        return TsOpen_Failed;
    }
}

void TsTakeBoutEvents(std::vector<DuelBoutRecord>* out)
{
    if (out == NULL)
    {
        return;
    }
    int dropped = 0;
    {
        QueueLock lock;
        out->insert(out->end(), g_queue.begin(), g_queue.end());
        g_queue.clear();
        dropped = g_dropped;
        g_dropped = 0;
    }
    if (dropped > 0)
    {
        LogError("[ts-bridge] " + Code(dropped) + " bout(s) dropped: the queue was full.");
    }
}

void ForgetTsBoutEvents()
{
    QueueLock lock;
    g_queue.clear();
    g_dropped = 0;
}

void ShutdownTsBridge()
{
    const TS_ApiV1* api = AttachedApi();
    if (api != NULL && g_listening && TsModuleLoaded(api))
    {
        const int rc = GuardedUnregister(api, &OnBout);
        if (rc != TS_OK && rc != TS_ERR_NOT_FOUND)
        {
            LogError("[ts-bridge] UnregisterBoutListener returned " + Code(rc) + ".");
        }
    }
    g_listening = false;
    g_api = NULL;
    // Nothing talks to TotalSparring after this, not even a late frame tick.
    g_state = Attach_Absent;
    ForgetTsBoutEvents();
}
} // namespace totalrecruit
