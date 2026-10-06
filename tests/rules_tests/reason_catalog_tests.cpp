// Tests for src/reason_catalog.cpp: the table of reasons and the tone a refusal
// takes.
//
// The tone block is a characterization test against the thing it replaced.
// SpeakOpeningLine used to pick a refusing NPC's line by searching the reason's
// finished text for words; the oracle below is that search, verbatim, and the
// test feeds it every built-in reason text. Wherever the old search had a
// meaningful answer the new key-based one must give the same, and the T8 block
// states what the old search could not: rewording a message cannot change the tone.

#include "test_support.h"
#include "reason_catalog.h"

#include <cstring>
#include <set>
#include <string>

using namespace totalrecruit;

namespace
{
bool Has(const std::string& text, const char* needle)
{
    return text.find(needle) != std::string::npos;
}

// The pre-T8 matcher from rules.cpp, unchanged, with the lines it chose turned
// into the tone names the new code uses.
RefusalTone OldSubstringTone(const std::string& reason)
{
    if (Has(reason, "harto") || Has(reason, "pesado"))
    {
        return RefusalTone_Insistence;
    }
    if (Has(reason, "d\xc3\xa9" "bil") || Has(reason, "fuerte"))
    {
        return RefusalTone_TooWeak;
    }
    if (Has(reason, "honor"))
    {
        return RefusalTone_Honour;
    }
    if (Has(reason, "lealtad") || Has(reason, "reputaci") || Has(reason, "respeto"))
    {
        return RefusalTone_Loyalty;
    }
    if (Has(reason, "mataste"))
    {
        return RefusalTone_Grudge;
    }
    return RefusalTone_Default;
}

void TheTableHasOneRowPerKeyInEnumOrder()
{
    const char* name = "catalog: one row per ReasonKey, in enum order";
    CHECK(ReasonCatalogSize() == (int)Reason_Count);
    for (int i = 0; i < ReasonCatalogSize(); ++i)
    {
        CHECK(ReasonCatalogKeyAt(i) == (ReasonKey)i);
    }
}

void EveryReasonHasAKeyAndAText()
{
    const char* name = "catalog: every reason has a messages.json key and a built-in text";
    CHECK(ReasonMessageKey(Reason_None, "neutral").empty());
    CHECK(std::strlen(ReasonDefaultText(Reason_None)) == 0);
    for (int i = 1; i < (int)Reason_Count; ++i)
    {
        const ReasonKey key = (ReasonKey)i;
        CHECK(!ReasonMessageKey(key, "neutral").empty());
        CHECK(std::strlen(ReasonDefaultText(key)) > 0);
    }
}

void KeysAreUniqueExceptTheSharedCheatJoin()
{
    const char* name = "catalog: message keys are unique, except cheat.joins which two reasons share";
    std::set<std::string> seen;
    int duplicates = 0;
    for (int i = 1; i < (int)Reason_Count; ++i)
    {
        if (!seen.insert(ReasonMessageKey((ReasonKey)i, "neutral")).second)
        {
            ++duplicates;
        }
    }
    CHECK(duplicates == 1);
    CHECK(ReasonMessageKey(Reason_CheatJoins, "x") == ReasonMessageKey(Reason_CheatJoinsFreely, "x"));
}

void TheBandIsFilledIntoFactionKeys()
{
    const char* name = "catalog: faction reasons carry the band, the others ignore it";
    CHECK(ReasonMessageKey(Reason_AllyJoins, "aliado") == "faction.aliado.reason");
    CHECK(ReasonMessageKey(Reason_AllyMaybe, "amistoso") == "faction.amistoso.reasonMaybe");
    CHECK(ReasonMessageKey(Reason_FactionHonour, "hostil") == "faction.hostil.reasonHonourable");
    CHECK(ReasonMessageKey(Reason_FactionGreedy, "hostil") == "faction.enemigo.reasonGreedy");
    CHECK(ReasonMessageKey(Reason_Insistence, "aliado") == "reason.insistence");
}

void ExistingKeysKeepTheirPaths()
{
    const char* name = "catalog: the reasons that already had a key keep it";
    CHECK(ReasonMessageKey(Reason_CheatNeedsCage, "") == "cheat.needsCage");
    CHECK(ReasonMessageKey(Reason_CheatWaiting, "") == "cheat.waiting");
    CHECK(ReasonMessageKey(Reason_FieldYieldJoins, "") == "submission.broken");
    CHECK(ReasonMessageKey(Reason_FieldYieldNotYet, "") == "submission.notYet");
    CHECK(ReasonMessageKey(Reason_GrudgeNever, "") == "grudge.never");
    CHECK(ReasonMessageKey(Reason_RescueJoins, "") == "rescue.joins");
    CHECK(ReasonMessageKey(Reason_RescueRepays, "") == "rescue.repays");
    CHECK(ReasonMessageKey(Reason_Intimidated, "") == "captive.intimidated");
    CHECK(ReasonMessageKey(Reason_UniqueZealot, "") == "unique.zealot");
    CHECK(ReasonMessageKey(Reason_AnimalOwned, "") == "animal.owned");
    CHECK(ReasonMessageKey(Reason_TameWounded, "") == "animal.tame.wounded");
    CHECK(ReasonMessageKey(Reason_TameReady, "") == "animal.tame.ready");
    CHECK(ReasonMessageKey(Reason_TameHungry, "") == "animal.tame.hungry");
}

void TheAnyCageAnswerHasItsOwnNeutralLines()
{
    const char* name = "catalog: cheat.needsAnyCage never says 'your cage'; cheat.needsCage is unchanged";
    CHECK(ReasonMessageKey(Reason_CheatNeedsAnyCage, "") == "cheat.needsAnyCage");
    CHECK(ReasonDefaultText(Reason_CheatNeedsAnyCage) == std::string("metelo en una jaula y hablara enseguida"));
    CHECK(!Has(ReasonDefaultText(Reason_CheatNeedsAnyCage), "tus jaulas"));
    CHECK(!Has(ReasonDefaultText(Reason_CheatNeedsAnyCage), "tuya"));
    CHECK(ReasonDefaultText(Reason_CheatNeedsCage) == std::string("metelo en una de tus jaulas y hablara enseguida"));
    CHECK(RefusalToneFor(Reason_CheatNeedsAnyCage) == RefusalToneFor(Reason_CheatNeedsCage));
}

void PricesAndStomachCarryTheirPlaceholders()
{
    const char* name = "catalog: the price and stomach texts keep {cats} and {purse}";
    CHECK(Has(ReasonDefaultText(Reason_CatsAsk), "{cats}") && Has(ReasonDefaultText(Reason_CatsAsk), "{purse}"));
    CHECK(Has(ReasonDefaultText(Reason_CatsJoin), "{cats}"));
    CHECK(Has(ReasonDefaultText(Reason_TameHungry), "{cats}"));
}

// The reasons a refusing NPC can have come from, with the tone the substring
// matcher gave their built-in text.
void ToneMatchesWhatTheOldSearchDecided()
{
    const char* name = "tone: the key gives the tone the old text search gave the built-in text";
    const ReasonKey refusals[] = {
        Reason_Insistence, Reason_AllyMaybe, Reason_FactionHonour, Reason_FactionGreedy,
        Reason_HonourRefusesBribe, Reason_MilitaryLoyalty, Reason_GrudgeNever,
        Reason_CheatNeedsCage, Reason_CheatWaiting, Reason_RescueRepays
    };
    for (size_t i = 0; i < sizeof(refusals) / sizeof(refusals[0]); ++i)
    {
        CHECK(RefusalToneFor(refusals[i]) == OldSubstringTone(ReasonDefaultText(refusals[i])));
    }
}

void EachToneHasItsReasons()
{
    const char* name = "tone: the five distinct tones are each reachable";
    CHECK(RefusalToneFor(Reason_Insistence) == RefusalTone_Insistence);
    CHECK(RefusalToneFor(Reason_AllyMaybe) == RefusalTone_TooWeak);
    CHECK(RefusalToneFor(Reason_FactionHonour) == RefusalTone_Honour);
    CHECK(RefusalToneFor(Reason_HonourRefusesBribe) == RefusalTone_Honour);
    CHECK(RefusalToneFor(Reason_MilitaryLoyalty) == RefusalTone_Loyalty);
    CHECK(RefusalToneFor(Reason_GrudgeNever) == RefusalTone_Grudge);
    CHECK(RefusalToneFor(Reason_FactionGreedy) == RefusalTone_Default);
    CHECK(RefusalToneFor(Reason_RescueRepays) == RefusalTone_Default);
    CHECK(RefusalToneFor(Reason_None) == RefusalTone_Default);
}

// --- T8: what the old search could not do.

void RewordingAMessageCannotChangeTheTone()
{
    const char* name = "tone: a reworded or translated message keeps the tone (the key decides)";
    // The same grudge, said in another way a translator might choose: the old
    // search finds no "mataste" in it and falls to the default line.
    const std::string reworded = "no olvida a los que perdi\xc3\xb3 por tu mano";
    CHECK(OldSubstringTone(reworded) == RefusalTone_Default);
    // The key never looked at the text, so it cannot be fooled by it.
    CHECK(RefusalToneFor(Reason_GrudgeNever) == RefusalTone_Grudge);
    // And the converse: a text that merely contains a trigger word is not a tone.
    const std::string wordy = "tiene honor de sobra, y aun asi se une";
    CHECK(OldSubstringTone(wordy) == RefusalTone_Honour);
}

void NoReasonThatIsNotARefusalCarriesATone()
{
    const char* name = "tone: reasons that never refuse are all default";
    const ReasonKey joins[] = {
        Reason_CheatJoins, Reason_RescueJoins, Reason_FieldYieldJoins, Reason_AllyJoins,
        Reason_CatsJoin, Reason_Intimidated, Reason_TameReady,
        Reason_CaredFor, Reason_BrokenNoPride, Reason_FreedFollows
    };
    for (size_t i = 0; i < sizeof(joins) / sizeof(joins[0]); ++i)
    {
        CHECK(RefusalToneFor(joins[i]) == RefusalTone_Default);
    }
}
} // namespace

void RunReasonCatalogTests()
{
    TheTableHasOneRowPerKeyInEnumOrder();
    EveryReasonHasAKeyAndAText();
    KeysAreUniqueExceptTheSharedCheatJoin();
    TheBandIsFilledIntoFactionKeys();
    ExistingKeysKeepTheirPaths();
    TheAnyCageAnswerHasItsOwnNeutralLines();
    PricesAndStomachCarryTheirPlaceholders();
    ToneMatchesWhatTheOldSearchDecided();
    EachToneHasItsReasons();
    RewordingAMessageCannotChangeTheTone();
    NoReasonThatIsNotARefusalCarriesATone();
}
