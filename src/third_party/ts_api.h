/*
 * ts_api.h -- TotalSparring public API, version 1.
 *
 * The ONE header another RE_Kenshi plugin copies to talk to TotalSparring.
 * Pure C ABI: POD structs, no std types, __cdecl function pointers, results
 * through out-pointers, int status codes. Append-only: a later version only
 * adds fields at the END of a struct and functions at the END of a table,
 * and every struct carries its own `size` so both sides can tell what the
 * other one knows.
 *
 * How to get it (no import library, nothing to link):
 *
 *     HMODULE ts = GetModuleHandleA("TotalSparring.dll");
 *     TS_GetApiFn getApi = ts ? (TS_GetApiFn)GetProcAddress(ts, "TS_GetApi") : NULL;
 *     const TS_ApiV1* api = NULL;
 *     if (getApi != NULL && getApi(TS_API_VERSION, &api) == TS_OK) { ... }
 *
 * Threading and lifetime:
 *   - Call every function on the GAME thread only (a hook, a frame tick, a
 *     MyGUI event). The API does not lock the engine.
 *   - Every Character* is only valid on the game thread for the duration of
 *     the call (or of the callback that received it). Never store one.
 *   - Strings in a TS_BoutEvent are valid only during the callback: copy them.
 *   - The table returned by TS_GetApi is static and lives as long as
 *     TotalSparring.dll is loaded. Keep the pointer, not a copy.
 *   - Nothing crosses this boundary as an exception: every entry point
 *     returns a status code, including after an internal fault.
 *
 * Both sides build against KenshiLib, so `Character` is the engine's own
 * class (global scope), passed as an opaque pointer.
 */
#ifndef TOTALSPARRING_TS_API_H
#define TOTALSPARRING_TS_API_H

#ifdef __cplusplus
class Character;
extern "C" {
#else
typedef struct Character Character;
#endif

/* The version this header describes. TS_GetApi hands out any version
 * 1..TS_API_VERSION; ask for the one you were built against. */
#define TS_API_VERSION 1u

/* Status codes. TS_OK is the only success. */
#define TS_OK 0
#define TS_ERR_NOT_READY (-1)       /* plugin not running, or no save loaded */
#define TS_ERR_BAD_ARG (-2)         /* NULL / too-small struct / same character twice */
#define TS_ERR_UNKNOWN_FIGHTER (-3) /* a character has no readable history key */
#define TS_ERR_BUSY (-4)            /* the duel popup is already open */
#define TS_ERR_FULL (-5)            /* no free bout-listener slot */
#define TS_ERR_NOT_FOUND (-6)       /* unregistering a listener that is not registered */
#define TS_ERR_VERSION (-7)         /* requested API version not provided */
#define TS_ERR_REFUSED (-8)         /* the duel popup could not be shown */
#define TS_ERR_INTERNAL (-9)        /* an internal fault was caught; nothing changed */

/* TS_BoutEvent::outcome */
#define TS_OUTCOME_UNDECIDED (-1) /* cancelled, interrupted, carried off, void... */
#define TS_OUTCOME_A_WON 0
#define TS_OUTCOME_B_WON 1
#define TS_OUTCOME_DRAW 2

/* TS_BoutEvent::duelType (meaningful only when isDuel != 0) */
#define TS_DUEL_FRIENDLY 0
#define TS_DUEL_TO_THE_DEATH 1

/* Head-to-head duel record of `a` against `b`, from a's point of view:
 * friendly and to-the-death duels only (spars are not counted), finished
 * or cut-short records (a series still in progress is not counted), both
 * orientations, a fighter's merged legacy keys included. The same rows the
 * Fight Log shows.
 * The CALLER sets `size` = sizeof(TS_DuelStats) before the call. */
typedef struct TS_DuelStats
{
    unsigned int size;
    int wins;
    int losses;
    int draws;
} TS_DuelStats;

/* One finished bout (spar or duel), sent once, after TotalSparring has torn
 * the pair down, paid any wager and recorded the history. Side A is always
 * the player's squad member. A best-of-N SPAR series reports nothing per
 * round in v1 (duels are never repeated).
 * Any pointer may be NULL when it could not be resolved. Everything is only
 * valid during the callback. */
typedef struct TS_BoutEvent
{
    unsigned int size;
    Character* a;
    Character* b;
    int isDuel;        /* 0 = spar, 1 = duel */
    int duelType;      /* TS_DUEL_*; only meaningful when isDuel */
    int outcome;       /* TS_OUTCOME_* */
    const char* reason; /* the end tag, e.g. "ko-a", "ko-b", "ko-draw", "cancelled", "interrupted" */
    const char* aUid;  /* history key of a (the id GetDuelStats uses) */
    const char* bUid;  /* history key of b */
    int stake;         /* the duel wager in cats, 0 when none */
} TS_BoutEvent;

/* Called on the game thread from inside TotalSparring's bout teardown.
 * Keep it short; do not start a bout or open a window from inside it
 * (defer that to your next frame). A fault inside it is caught and logged,
 * and the other listeners still run. */
typedef void(__cdecl* TS_BoutCallback)(void* user, const TS_BoutEvent* ev);

typedef struct TS_ApiV1
{
    unsigned int size;    /* sizeof(TS_ApiV1) as built by TotalSparring */
    unsigned int version; /* TS_API_VERSION of TotalSparring's build */

    /* Fills `out` with a's head-to-head duel record against b.
     * TS_OK (zeros when they never duelled), TS_ERR_BAD_ARG,
     * TS_ERR_NOT_READY (no save loaded), TS_ERR_UNKNOWN_FIGHTER,
     * TS_ERR_INTERNAL. Reads the history database: call it on an event
     * (selection change, window open), not every frame. */
    int(__cdecl* GetDuelStats)(Character* a, Character* b, TS_DuelStats* out);

    /* At most 8 listeners in total. Registering the same (cb, user) twice
     * is TS_OK and keeps one entry. TS_ERR_BAD_ARG (NULL cb), TS_ERR_FULL. */
    int(__cdecl* RegisterBoutListener)(TS_BoutCallback cb, void* user);

    /* TS_OK, TS_ERR_BAD_ARG, TS_ERR_NOT_FOUND. Safe from inside a callback. */
    int(__cdecl* UnregisterBoutListener)(TS_BoutCallback cb, void* user);

    /* Opens TotalSparring's own "Dueling" window for `npc` with `challenger`
     * pre-selected -- exactly what the player's Dueling button does (it
     * shows its own refusal when the NPC cannot be challenged).
     * `challenger` may be NULL. TS_OK, TS_ERR_BAD_ARG (NULL npc),
     * TS_ERR_NOT_READY, TS_ERR_BUSY (already open), TS_ERR_REFUSED,
     * TS_ERR_INTERNAL. */
    int(__cdecl* OpenDuelPopup)(Character* npc, Character* challenger);

    /* 1 while TotalSparring is running (fully started, not stopped), else 0. */
    int(__cdecl* IsReady)(void);
} TS_ApiV1;

/* The one exported entry point. TS_OK and *out = the static table when
 * 1 <= requestedVersion <= TS_API_VERSION; otherwise *out = NULL and
 * TS_ERR_VERSION (TS_ERR_BAD_ARG when out is NULL). */
typedef int(__cdecl* TS_GetApiFn)(unsigned int requestedVersion, const TS_ApiV1** out);

#ifdef TS_API_BUILDING_PLUGIN
__declspec(dllexport) int __cdecl TS_GetApi(unsigned int requestedVersion, const TS_ApiV1** out);
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* TOTALSPARRING_TS_API_H */
