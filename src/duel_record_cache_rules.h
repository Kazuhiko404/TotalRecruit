#pragma once

// When the readout card reads the duel record from TotalSparring again.
//
// Play-test (2026-10-04): after a duel the card still said "Duelos: 0-0" while the
// recruit popup showed the new record. The card read the record once per (NPC,
// selected squad member) pair, and a duel does not change the pair. Reading it on
// every 2 s refresh would hit TotalSparring's history database for nothing, so the
// card keeps its copy and also remembers the bout generation it read at
// (ts_bridge.h TsBoutGeneration: bumped on every bout TotalSparring reports). A new
// bout, or a different pair, makes the copy stale.
//
// Pointers are only compared, never dereferenced.

namespace totalrecruit
{
// What the card's copy of the record was read for.
struct DuelRecordStamp
{
    // A copy exists at all.
    bool valid;
    const void* npc;
    const void* challenger;
    // TsBoutGeneration when the copy was read.
    unsigned generation;

    DuelRecordStamp() : valid(false), npc(0), challenger(0), generation(0) {}
};

// True when the copy stamped `cached` cannot be shown for this pair at this bout
// generation: there is none, the NPC or the challenger changed, or TotalSparring
// reported a bout since it was read.
bool DuelRecordNeedsRead(const DuelRecordStamp& cached, const void* npc,
                         const void* challenger, unsigned boutGeneration);
} // namespace totalrecruit
