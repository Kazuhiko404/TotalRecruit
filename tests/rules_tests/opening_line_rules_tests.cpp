// Tests for src/opening_line_rules: which messages.json line a person opens with
// (priority-one E). The refusal.* and currency.* keys of messages.json used to be
// dead (npc_speech.cpp hardcoded the same lines); now every branch names its key,
// and the built-in text is the line it always said.

#include "test_support.h"
#include "opening_line_rules.h"

#include <cstring>
#include <string>

using namespace totalrecruit;

namespace
{
OpeningFacts Person()
{
    OpeningFacts f;
    f.traitsRead = true;
    return f;
}

std::string KeyOf(const OpeningFacts& f)
{
    return OpeningLineFor(f).key;
}

void UnreadTraitsAskWhatTheyWant()
{
    const char* name = "opening: traits that cannot be read get the plain question";
    OpeningFacts f;
    f.traitsRead = false;
    f.refused = true;
    CHECK(KeyOf(f) == "currency.unread");
    CHECK(std::string(OpeningLineFor(f).fallback) == "\xc2\xbfY bien? Habla de una vez.");
}

void ARefusalSpeaksInItsTone()
{
    const char* name = "opening: a refusal takes the refusal.* line of its tone";
    OpeningFacts f = Person();
    f.refused = true;
    f.tone = RefusalTone_Default;     CHECK(KeyOf(f) == "refusal.fallback");
    f.tone = RefusalTone_Insistence;  CHECK(KeyOf(f) == "refusal.insistence");
    f.tone = RefusalTone_TooWeak;     CHECK(KeyOf(f) == "refusal.tooWeak");
    f.tone = RefusalTone_Honour;      CHECK(KeyOf(f) == "refusal.honour");
    f.tone = RefusalTone_Loyalty;     CHECK(KeyOf(f) == "refusal.loyalty");
    f.tone = RefusalTone_Grudge;      CHECK(KeyOf(f) == "refusal.grudge");
    f.tone = RefusalTone_Grudge;
    CHECK(std::string(OpeningLineFor(f).fallback) == "S\xc3\xa9 lo que hiciste. Fuera de mi vista.");
    // A refusal wins over a rescue.
    f.viaRescue = true;
    CHECK(KeyOf(f) == "refusal.grudge");
}

void ARescueSpeaksOfTheDebt()
{
    const char* name = "opening: a rescue line by temperament";
    OpeningFacts f = Person();
    f.viaRescue = true;
    f.currency = Currency_Cats;
    f.temperament = Temperament_Honorable;  CHECK(KeyOf(f) == "currency.rescueHonourable");
    f.temperament = Temperament_ColdCruel;  CHECK(KeyOf(f) == "currency.rescueCold");
    f.temperament = Temperament_Maniacal;   CHECK(KeyOf(f) == "currency.rescueCold");
    f.temperament = Temperament_Fearful;    CHECK(KeyOf(f) == "currency.rescueFearful");
    f.temperament = Temperament_Normal;     CHECK(KeyOf(f) == "currency.rescue");
}

void TheCurrencyNamesWhatTheyWant()
{
    const char* name = "opening: freedom, care and submission lines by temperament";
    OpeningFacts f = Person();
    f.currency = Currency_Freedom;           CHECK(KeyOf(f) == "currency.freedom");
    f.currency = Currency_Care;
    f.temperament = Temperament_Honorable;   CHECK(KeyOf(f) == "currency.careKind");
    f.temperament = Temperament_WarmKind;    CHECK(KeyOf(f) == "currency.careKind");
    f.temperament = Temperament_Fearful;     CHECK(KeyOf(f) == "currency.careFearful");
    f.temperament = Temperament_Brave;       CHECK(KeyOf(f) == "currency.care");
    f.currency = Currency_Submission;
    f.temperament = Temperament_Honorable;   CHECK(KeyOf(f) == "currency.submissionHonourable");
    f.temperament = Temperament_Maniacal;    CHECK(KeyOf(f) == "currency.submissionManiacal");
    f.temperament = Temperament_Fearful;     CHECK(KeyOf(f) == "currency.submissionFearful");
    f.temperament = Temperament_WarmKind;    CHECK(KeyOf(f) == "currency.submission");
}

void NothingOwedSaysReadyOrNothing()
{
    const char* name = "opening: nothing owed says it is ready only when it joins; a no says nothing";
    OpeningFacts f = Person();
    f.currency = Currency_None;
    f.willJoin = true;
    CHECK(KeyOf(f) == "currency.ready");
    f.willJoin = false;
    CHECK(KeyOf(f).empty());
    CHECK(OpeningLineFor(f).fallback[0] == '\0');
}

void CatsDependOnWhoAsks()
{
    const char* name = "opening: a price line by who they are, then by temperament";
    OpeningFacts f = Person();
    f.currency = Currency_Cats;
    f.speaker = OpeningSpeaker_Bandit;  CHECK(KeyOf(f) == "currency.catsBandit");
    f.speaker = OpeningSpeaker_Trader;  CHECK(KeyOf(f) == "currency.catsTrader");
    f.speaker = OpeningSpeaker_Slaver;  CHECK(KeyOf(f) == "currency.catsSlaver");
    f.speaker = OpeningSpeaker_Other;
    f.temperament = Temperament_Fearful;  CHECK(KeyOf(f) == "currency.catsFearful");
    f.temperament = Temperament_Normal;   CHECK(KeyOf(f) == "currency.cats");
}

void EveryLineHasItsBuiltInText()
{
    const char* name = "opening: every keyed line carries a non-empty built-in text";
    OpeningFacts f = Person();
    const Currency currencies[4] = { Currency_Cats, Currency_Care, Currency_Freedom, Currency_Submission };
    for (int c = 0; c < 4; ++c)
    {
        for (int t = 0; t < Temperament_Count; ++t)
        {
            f.currency = currencies[c];
            f.temperament = (Temperament)t;
            const OpeningLine line = OpeningLineFor(f);
            CHECK(line.key[0] != '\0' && line.fallback[0] != '\0');
        }
    }
}
} // namespace

void RunOpeningLineRulesTests()
{
    UnreadTraitsAskWhatTheyWant();
    ARefusalSpeaksInItsTone();
    ARescueSpeaksOfTheDebt();
    TheCurrencyNamesWhatTheyWant();
    NothingOwedSaysReadyOrNothing();
    CatsDependOnWhoAsks();
    EveryLineHasItsBuiltInText();
}
