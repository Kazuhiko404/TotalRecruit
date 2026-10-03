#pragma once

// PURE: how many times the player has put each character down.
//
// The count lives in the character's own save state (character_identity.cpp
// writes it next to the persisted id as idata "total_recruit.player_defeats",
// one row of the saved_ints.h table),
// so a reload restores exactly what that save knew -- not a floor, not a guess,
// and nothing the engine wrote on its own. This ledger is the in-memory side of
// it: cleared at load begin (load_boundary.cpp), refilled by each character's
// load hook, added to by every counted defeat (defeats.cpp), read back by the
// serialise hook.
//
// Keyed by CharKey: the persisted id whenever the character has one, the engine
// handle otherwise (a handle-keyed count works for the session but never reaches
// a save, because only an id is written there). No engine include -- tested in
// tests/rules_tests/defeat_ledger_tests.cpp.

#include "char_key.h"

#include <map>

namespace totalrecruit
{
// The most defeats a save may claim. A value past this in a save state is
// corruption or another mod's key, never a real tally.
extern const int kMaxSavedDefeats;

// A count read back from a save, made safe to use: a negative value is 0 and an
// absurd one is capped at kMaxSavedDefeats.
int SanitiseSavedDefeats(int raw);

class DefeatLedger
{
public:
    // 0 for a character never counted.
    int Get(const CharKey& key) const;

    // Replaces the count (a load restoring a save). A count of 0 or less forgets
    // the character; anything else is sanitised first.
    void Set(const CharKey& key, int count);

    // One more defeat; returns the new count (never past kMaxSavedDefeats).
    int Add(const CharKey& key);

    // The whole world was abandoned (load begin).
    void Clear();

    // How many characters have a count, for logs and tests.
    int Size() const;

private:
    typedef std::map<CharKey, int> Counts;
    Counts counts_;
};
} // namespace totalrecruit
