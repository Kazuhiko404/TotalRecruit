// Tests for src/readout_catalog.cpp: one short line per reason, and they fit the card.

#include "test_support.h"
#include "readout_catalog.h"

#include <cstring>
#include <set>
#include <string>

using namespace totalrecruit;

namespace
{
// The reasons for which the card stays hidden: the question was not about a person.
bool IsHidden(int id)
{
    return id == Reason_None || id == Reason_Nobody || id == Reason_AnimalsDisabled ||
           id == Reason_Pretending || id == Reason_Dead;
}

void EveryReasonHasARowPlusTheWait()
{
    const char* name = "catalog: one row per ReasonKey plus the cage wait";
    CHECK(ReadoutLineCount() == (int)Reason_Count + 1);
    CHECK(kLineWaitCage == (int)Reason_Count);
}

void VisibleReasonsHaveShortAsciiText()
{
    const char* name = "catalog: shown reasons have text that fits the card and is plain ASCII";
    for (int id = 0; id < ReadoutLineCount(); ++id)
    {
        const char* text = ReadoutLineDefaultText(id);
        if (IsHidden(id))
        {
            CHECK(std::strlen(text) == 0);
            continue;
        }
        // Expanded card: about 40 glyphs at the body font. Placeholders are counted
        // at their widest fill (a 6 digit price, a 3 digit percent, "59min").
        std::string widest = text;
        const char* tokens[][2] = { { "{cats}", "150.000" }, { "{purse}", "100.000" },
                                    { "{hours}", "59min" }, { "{food}", "100" }, { "{need}", "100" } };
        for (int t = 0; t < 5; ++t)
        {
            std::string::size_type at;
            while ((at = widest.find(tokens[t][0])) != std::string::npos)
            {
                widest.replace(at, std::strlen(tokens[t][0]), tokens[t][1]);
            }
        }
        CHECK(std::strlen(text) > 0);
        CHECK(widest.size() <= 40);
        bool ascii = true;
        for (std::string::size_type i = 0; i < widest.size(); ++i)
        {
            if ((unsigned char)widest[i] > 126) ascii = false;
        }
        CHECK(ascii);
    }
}

void KeysAreUniqueAndNamespaced()
{
    const char* name = "catalog: message keys are unique and live under readout.line";
    std::set<std::string> seen;
    for (int id = 0; id < ReadoutLineCount(); ++id)
    {
        if (IsHidden(id))
        {
            continue;
        }
        const std::string key = ReadoutLineMessageKey(id);
        CHECK(key.compare(0, 13, "readout.line.") == 0);
        CHECK(seen.insert(key).second);
    }
}

void OutOfRangeIsEmpty()
{
    const char* name = "catalog: an id outside the table is empty, not a crash";
    CHECK(std::strlen(ReadoutLineDefaultText(-1)) == 0);
    CHECK(std::strlen(ReadoutLineDefaultText(9999)) == 0);
    CHECK(std::strlen(ReadoutLineMessageKey(9999)) == 0);
}
} // namespace

void RunReadoutCatalogTests()
{
    EveryReasonHasARowPlusTheWait();
    VisibleReasonsHaveShortAsciiText();
    KeysAreUniqueAndNamespaced();
    OutOfRangeIsEmpty();
}
