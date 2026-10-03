// Tests for src/defeat_ledger.cpp: the per-character player-defeat count that a
// save carries and a load restores.

#include "test_support.h"
#include "defeat_ledger.h"

using namespace totalrecruit;

namespace
{
const CharKey kGwyn = CharKey::FromId("6f1c2a9e-guard-gwynnever");
const CharKey kOther = CharKey::FromId("0b7d3e41-someone-else");

void NeverCountedIsZero()
{
    const char* name = "ledger: a character never counted has 0";
    DefeatLedger ledger;
    CHECK(ledger.Get(kGwyn) == 0);
    CHECK(ledger.Size() == 0);
}

void AddCountsPerCharacter()
{
    const char* name = "ledger: each defeat adds one, to that character only";
    DefeatLedger ledger;
    CHECK(ledger.Add(kGwyn) == 1);
    CHECK(ledger.Add(kGwyn) == 2);
    CHECK(ledger.Get(kGwyn) == 2);
    CHECK(ledger.Get(kOther) == 0);
    // A handle key is a different key from an id key.
    CHECK(ledger.Get(CharKey(7u, 1u)) == 0);
}

void SetRestoresAndZeroForgets()
{
    const char* name = "ledger: Set restores a saved count, 0 forgets the character";
    DefeatLedger ledger;
    ledger.Set(kGwyn, 3);
    CHECK(ledger.Get(kGwyn) == 3);
    CHECK(ledger.Size() == 1);
    ledger.Set(kGwyn, 0);
    CHECK(ledger.Get(kGwyn) == 0);
    CHECK(ledger.Size() == 0);
    ledger.Set(kGwyn, -4);
    CHECK(ledger.Size() == 0);
}

// Load an older save: the ledger is cleared at load begin and each character's
// load hook puts back what THAT save held.
void ALoadRestoresExactlyThatSave()
{
    const char* name = "ledger: a load restores the loaded save's counts, not the session's";
    DefeatLedger ledger;
    ledger.Add(kGwyn);
    ledger.Add(kGwyn);
    ledger.Add(kOther);

    ledger.Clear();
    CHECK(ledger.Get(kGwyn) == 0);
    CHECK(ledger.Get(kOther) == 0);

    ledger.Set(kGwyn, 1);    // the older save knew one defeat
    CHECK(ledger.Get(kGwyn) == 1);
    CHECK(ledger.Get(kOther) == 0);
    CHECK(ledger.Add(kGwyn) == 2);
}

void SavedCountsAreSanitised()
{
    const char* name = "ledger: a corrupt saved count is clamped, never trusted";
    CHECK(SanitiseSavedDefeats(-5) == 0);
    CHECK(SanitiseSavedDefeats(0) == 0);
    CHECK(SanitiseSavedDefeats(2) == 2);
    CHECK(SanitiseSavedDefeats(kMaxSavedDefeats + 1) == kMaxSavedDefeats);
    CHECK(SanitiseSavedDefeats(0x7fffffff) == kMaxSavedDefeats);

    DefeatLedger ledger;
    ledger.Set(kGwyn, 0x7fffffff);
    CHECK(ledger.Get(kGwyn) == kMaxSavedDefeats);
    CHECK(ledger.Add(kGwyn) == kMaxSavedDefeats);
}
} // namespace

void RunDefeatLedgerTests()
{
    NeverCountedIsZero();
    AddCountsPerCharacter();
    SetRestoresAndZeroForgets();
    ALoadRestoresExactlyThatSave();
    SavedCountsAreSanitised();
}
