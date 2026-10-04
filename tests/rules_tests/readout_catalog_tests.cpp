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

void EveryReasonHasARowPlusTheCardsOwnLines()
{
    const char* name = "catalog: one row per ReasonKey plus the wait, the open roll, beat-again and the ally prices";
    CHECK(ReadoutLineCount() == (int)Reason_Count + 9);
    CHECK(kLineWaitCage == (int)Reason_Count);
    CHECK(kLineRollOpen == (int)Reason_Count + 1);
    CHECK(kLineFieldYieldBeatAgain == (int)Reason_Count + 2);
    CHECK(kLineAllyPaidJoin == (int)Reason_Count + 3);
    CHECK(kLineAllyPaidAsk == (int)Reason_Count + 4);
    CHECK(kLineTempMemberDaysLeft == (int)Reason_Count + 5);
    CHECK(kLineHelperDaysLeft == (int)Reason_Count + 6);
    CHECK(kLineDuelRecord == (int)Reason_Count + 7);
    CHECK(kLineDuelRecordWithDraws == (int)Reason_Count + 8);
}

void TheDuelRecordLinesCarryTheCounts()
{
    const char* name = "catalog: the duel record reads wins-losses ({food}-{need}), draws in {chance}";
    CHECK(std::string(ReadoutLineDefaultText(kLineDuelRecord)) == "Duelos: {food}-{need}");
    CHECK(std::string(ReadoutLineDefaultText(kLineDuelRecordWithDraws)) == "Duelos: {food}-{need}-{chance}");
    CHECK(std::string(ReadoutLineMessageKey(kLineDuelRecord)) == "readout.line.duel.record");
    CHECK(std::string(ReadoutLineMessageKey(kLineDuelRecordWithDraws)) == "readout.line.duel.recordDraws");
}

void TheHelperLineCountsTheDays()
{
    const char* name = "catalog: a helper's line names the days left ({need})";
    const std::string days = ReadoutLineDefaultText(kLineHelperDaysLeft);
    CHECK(days.find("Ayudante") == 0 && days.find("{need}") != std::string::npos);
    CHECK(std::string(ReadoutLineMessageKey(kLineHelperDaysLeft)) == "readout.line.helper.daysLeft");
}

void TheTempMemberLineCountsTheDays()
{
    const char* name = "catalog: a temporary member's line names the days left ({need})";
    const std::string days = ReadoutLineDefaultText(kLineTempMemberDaysLeft);
    CHECK(days.find("Temporal") == 0 && days.find("{need}") != std::string::npos);
    CHECK(std::string(ReadoutLineMessageKey(kLineTempMemberDaysLeft)) == "readout.line.temp.daysLeft");
}

void TheAllyTierLinesNameTheFriendship()
{
    const char* name = "catalog: the ally price lines keep {cats} (and {purse} when short); the tier refusals have lines";
    const std::string join = ReadoutLineDefaultText(kLineAllyPaidJoin);
    const std::string ask = ReadoutLineDefaultText(kLineAllyPaidAsk);
    CHECK(join.find("Aliado") == 0 && join.find("{cats}") != std::string::npos);
    CHECK(ask.find("{cats}") != std::string::npos && ask.find("{purse}") != std::string::npos);
    CHECK(std::string(ReadoutLineMessageKey(kLineAllyPaidJoin)) == "readout.line.ally.paidJoin");
    CHECK(std::string(ReadoutLineMessageKey(kLineAllyPaidAsk)) == "readout.line.ally.paidAsk");
    CHECK(std::string(ReadoutLineMessageKey(Reason_AllyTemporary)) == "readout.line.ally.temporary");
    CHECK(std::strlen(ReadoutLineDefaultText(Reason_AllyLeaderTooWeak)) > 0);
    CHECK(std::strlen(ReadoutLineDefaultText(Reason_AllyCruel)) > 0);
    CHECK(std::strlen(ReadoutLineDefaultText(Reason_AllyGreedy)) > 0);
}

void TheCardsOwnLinesSayWhatTheyMean()
{
    const char* name = "catalog: the open roll states the odds, beat-again never says to starve them";
    CHECK(std::string(ReadoutLineMessageKey(kLineRollOpen)) == "readout.line.rollOpen");
    CHECK(std::string(ReadoutLineDefaultText(kLineRollOpen)).find("{chance}%") == 0);
    CHECK(std::string(ReadoutLineMessageKey(kLineFieldYieldBeatAgain)) ==
          "readout.line.fieldYield.beatAgain");
    const std::string beat = ReadoutLineDefaultText(kLineFieldYieldBeatAgain);
    CHECK(beat.find("sin comer") == std::string::npos);
    CHECK(beat.find("vencelo") != std::string::npos);
}

void EveryRouteHasALabel()
{
    const char* name = "catalog: each route the player is told has a readout.route key and a label";
    const ReadoutRoute routes[] = { ReadoutRoute_Free, ReadoutRoute_Captivity, ReadoutRoute_Rescue,
                                    ReadoutRoute_Surrender, ReadoutRoute_Mercy, ReadoutRoute_Animal,
                                    ReadoutRoute_FieldYield };
    std::set<std::string> keys;
    for (int i = 0; i < 7; ++i)
    {
        const std::string key = ReadoutRouteMessageKey(routes[i]);
        CHECK(key.compare(0, 14, "readout.route.") == 0);
        CHECK(keys.insert(key).second);
        CHECK(std::strlen(ReadoutRouteDefaultText(routes[i])) > 0);
    }
    CHECK(std::string(ReadoutRouteMessageKey(ReadoutRoute_FieldYield)) == "readout.route.fieldYield");
    CHECK(std::string(ReadoutRouteDefaultText(ReadoutRoute_FieldYield)) == "Sometido");
    CHECK(std::string(ReadoutRouteDefaultText(ReadoutRoute_Surrender)) == "Rendicion");
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
                                    { "{hours}", "59min" }, { "{food}", "100" }, { "{need}", "100" },
                                    { "{chance}", "100" } };
        for (int t = 0; t < 6; ++t)
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
    EveryReasonHasARowPlusTheCardsOwnLines();
    TheDuelRecordLinesCarryTheCounts();
    TheCardsOwnLinesSayWhatTheyMean();
    TheAllyTierLinesNameTheFriendship();
    TheTempMemberLineCountsTheDays();
    TheHelperLineCountsTheDays();
    EveryRouteHasALabel();
    VisibleReasonsHaveShortAsciiText();
    KeysAreUniqueAndNamespaced();
    OutOfRangeIsEmpty();
}
