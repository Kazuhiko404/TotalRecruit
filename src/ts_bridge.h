#pragma once

// The bridge to TotalSparring's public API (src/third_party/ts_api.h, a verbatim copy
// of TotalSparring's own header; nothing is linked).
//
// TotalSparring owns duels: its own Dueling window decides friendly or to the death
// and fights it out. TotalRecruit only opens that window for an NPC and listens for
// how it ended (duel_challenge_rules.h says what a result means).
//
// Lifecycle:
//   - Attach is lazy and never in startPlugin (load order is not guaranteed): the
//     frame tick calls TsBridgeTick, which tries every kAttachRetryMs (2 s). It finds
//     TotalSparring.dll by name (GetModuleHandleA), or by scanning the process for
//     the TS_GetApi export (the Mod Hub pattern, mod_hub_integration.cpp), asks
//     TS_GetApi for version 1, checks the table's size and entry points, and registers
//     one bout listener. No TotalSparring after kAttachTries tries, an old one without
//     the export, or a refused version: absent for the session (logged once), and
//     every call below says so.
//   - The listener runs inside TotalSparring's bout teardown. It only copies the event:
//     both Character pointers are turned into CharKeys there and then (SafeFindCharKey;
//     they are only valid during the call), whether side A is the player's is read,
//     and the copy goes into a small locked queue. TsTakeBoutEvents hands the queue to
//     the frame tick, which acts on it -- never inside TotalSparring's teardown.
//   - ShutdownTsBridge (stopPlugin) unregisters the listener, only while
//     TotalSparring's module is still loaded.
//
// Every call into TotalSparring's table is SEH-guarded and its status code checked;
// a fault reads as TS_ERR_INTERNAL. All of this runs on the game thread, except the
// queue, which is locked anyway.

#include <vector>

#include "duel_challenge_rules.h"

class Character;

namespace totalrecruit
{
enum TsOpenResult
{
    TsOpen_Opened,
    // TotalSparring is not there (or not attached yet).
    TsOpen_Absent,
    // TS_ERR_NOT_READY: no save loaded, or TotalSparring not running.
    TsOpen_NotReady,
    // TS_ERR_BUSY: its duel window is already open.
    TsOpen_Busy,
    // TS_ERR_REFUSED: the window could not be shown (TotalSparring says why itself).
    TsOpen_Refused,
    // Any other code, a bad table or a caught fault.
    TsOpen_Failed
};

// The frame tick: attaches (retrying every 2 s) until attached or known absent.
void TsBridgeTick();

// Attached, listening for bouts, and TotalSparring says it is ready.
bool TsAvailable();

// TotalSparring's head-to-head duel record of `squadMember` against `npc` (friendly
// and to-the-death duels, from the squad member's side). Reads its history database:
// call it on an event (popup open, selection change), never every frame. False when
// TotalSparring is absent, not ready, or cannot name a fighter.
bool TsDuelStats(Character* squadMember, Character* npc, int* wins, int* losses, int* draws);

// Opens TotalSparring's own Dueling window for `npc` with `challenger` pre-selected.
TsOpenResult TsOpenDuel(Character* npc, Character* challenger);

// Moves every bout TotalSparring reported since the last call into `out`, oldest
// first. Game thread (the frame tick).
void TsTakeBoutEvents(std::vector<DuelBoutRecord>* out);

// Goes up by one for every bout TotalSparring reports (spar or duel), counted when
// the listener copies it: by then TotalSparring has written the bout to its history,
// so a duel record read after a change sees it. A reader keeps the value it read at
// and reads again when it differs (duel_record_cache_rules.h). Any thread.
unsigned TsBoutGeneration();

// Load begin: bouts of the abandoned world are dropped.
void ForgetTsBoutEvents();

// stopPlugin: unregisters the listener and stops talking to TotalSparring.
void ShutdownTsBridge();
} // namespace totalrecruit
