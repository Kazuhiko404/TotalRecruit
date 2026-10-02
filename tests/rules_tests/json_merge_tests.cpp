// Tests for src/json_merge.cpp: the text merge behind SaveConfig. The contract is
// "replace known values in place, append missing known keys, touch nothing else".

#include "test_support.h"
#include "json_merge.h"

#include <cstdlib>
#include <string>
#include <vector>

using namespace totalrecruit;

namespace
{
std::vector<JsonUpdate> One(const char* key, const std::string& value)
{
    std::vector<JsonUpdate> updates;
    updates.push_back(JsonUpdate(key, value));
    return updates;
}

bool Has(const std::string& text, const char* needle)
{
    return text.find(needle) != std::string::npos;
}

size_t Count(const std::string& text, const char* needle)
{
    size_t n = 0;
    size_t at = 0;
    const std::string pattern(needle);
    while ((at = text.find(pattern, at)) != std::string::npos)
    {
        ++n;
        at += pattern.size();
    }
    return n;
}

void ReplacesAKnownValueInPlace()
{
    const char* name = "json_merge: a known value is replaced where it stands";
    const std::string before =
        "{\n"
        "  \"enabled\": true,\n"
        "  \"catsMinimum\":   200,\n"
        "  \"preset\": 1\n"
        "}\n";
    const MergeResult r = MergeJsonKeys(before, One("catsMinimum", "350"));
    CHECK(r.ok);
    CHECK(r.text ==
          "{\n"
          "  \"enabled\": true,\n"
          "  \"catsMinimum\":   350,\n"
          "  \"preset\": 1\n"
          "}\n");
    CHECK(r.replaced == 1 && r.appended == 0 && r.kept == 0);
}

void UnknownKeysAndCommentsSurvive()
{
    const char* name = "json_merge: unknown keys, comments and order are kept";
    const std::string before =
        "{\n"
        "  // my notes: do not touch\n"
        "  \"myOwnKey\": [1, 2, 3],\n"
        "  \"enabled\": true, /* inline */\n"
        "  \"zzz\": \"last\"\n"
        "}\n";
    const MergeResult r = MergeJsonKeys(before, One("enabled", "false"));
    CHECK(r.ok);
    CHECK(Has(r.text, "// my notes: do not touch"));
    CHECK(Has(r.text, "\"myOwnKey\": [1, 2, 3]"));
    CHECK(Has(r.text, "/* inline */"));
    CHECK(Has(r.text, "\"enabled\": false,"));
    CHECK(r.text.find("myOwnKey") < r.text.find("enabled"));
    CHECK(r.text.find("enabled") < r.text.find("zzz"));
}

void EqualValuesKeepTheirOriginalText()
{
    const char* name = "json_merge: a value that already matches is left exactly as written";
    const std::string before =
        "{\n"
        "  \"easyHoursCaged\": 6.0,\n"
        "  \"fearfulMultiplier\": 0.60,\n"
        "  \"probeOnClick\": false,\n"
        "  \"recruitLabel\": \"Reclutar\",\n"
        "  \"factions\": [ \"a\",  \"b\" ]\n"
        "}\n";
    std::vector<JsonUpdate> updates;
    updates.push_back(JsonUpdate("easyHoursCaged", "6"));
    updates.push_back(JsonUpdate("fearfulMultiplier", "0.6"));
    updates.push_back(JsonUpdate("probeOnClick", "false"));
    updates.push_back(JsonUpdate("recruitLabel", "\"Reclutar\""));
    updates.push_back(JsonUpdate("factions", "[\"a\", \"b\"]"));
    const MergeResult r = MergeJsonKeys(before, updates);
    CHECK(r.ok);
    CHECK(r.text == before);
    CHECK(r.kept == 5 && r.replaced == 0 && r.appended == 0);
}

void MissingKeysAreAppended()
{
    const char* name = "json_merge: missing known keys are appended to the top-level object";
    const std::string before =
        "{\n"
        "  \"enabled\": true\n"
        "}\n";
    std::vector<JsonUpdate> updates;
    updates.push_back(JsonUpdate("catsHardCap", "150000"));
    updates.push_back(JsonUpdate("talentMaxMultiplier", "2"));
    const MergeResult r = MergeJsonKeys(before, updates);
    CHECK(r.ok);
    CHECK(r.text ==
          "{\n"
          "  \"enabled\": true,\n"
          "  \"catsHardCap\": 150000,\n"
          "  \"talentMaxMultiplier\": 2\n"
          "}\n");
    CHECK(r.appended == 2);
}

void AppendingAfterATrailingCommaStaysValid()
{
    const char* name = "json_merge: an existing trailing comma is not doubled";
    const std::string before = "{\n  \"a\": 1,\n}\n";
    const MergeResult r = MergeJsonKeys(before, One("b", "2"));
    CHECK(r.ok);
    CHECK(Count(r.text, ",,") == 0);
    CHECK(Has(r.text, "\"b\": 2"));
    CHECK(Has(r.text, "\"a\": 1,"));
}

void StringsDoNotConfuseTheScanner()
{
    const char* name = "json_merge: braces, commas and key names inside strings are inert";
    const std::string before =
        "{\n"
        "  \"recruitLabel\": \"enabled, }{ \\\"quoted\\\" \",\n"
        "  \"enabled\": true\n"
        "}\n";
    const MergeResult r = MergeJsonKeys(before, One("enabled", "false"));
    CHECK(r.ok);
    CHECK(Has(r.text, "\"recruitLabel\": \"enabled, }{ \\\"quoted\\\" \","));
    CHECK(Has(r.text, "\"enabled\": false"));
    CHECK(r.replaced == 1);
}

void ACommentedOutKeyIsNotTheKey()
{
    const char* name = "json_merge: a key inside a comment does not count";
    const std::string before =
        "{\n"
        "  // \"preset\": 2,\n"
        "  \"preset\": 1\n"
        "}\n";
    const MergeResult r = MergeJsonKeys(before, One("preset", "0"));
    CHECK(r.ok);
    CHECK(Has(r.text, "// \"preset\": 2,"));
    CHECK(Has(r.text, "\"preset\": 0"));
}

void KeyNamesMatchWholly()
{
    const char* name = "json_merge: 'enabled' is not 'rescueEnabled'";
    const std::string before = "{\n  \"rescueEnabled\": true\n}\n";
    const MergeResult r = MergeJsonKeys(before, One("enabled", "false"));
    CHECK(r.ok);
    CHECK(Has(r.text, "\"rescueEnabled\": true"));
    CHECK(Has(r.text, "\"enabled\": false"));
    CHECK(r.appended == 1 && r.replaced == 0);
}

void NestedKeysAreUpdatedWhereTheyAre()
{
    const char* name = "json_merge: a key inside a nested object is replaced there";
    const std::string before =
        "{\n"
        "  \"exclude\": {\n"
        "    \"dead\": true,\n"
        "    \"factions\": [\"x\"]\n"
        "  },\n"
        "  \"enabled\": true\n"
        "}\n";
    std::vector<JsonUpdate> updates;
    updates.push_back(JsonUpdate("dead", "false"));
    updates.push_back(JsonUpdate("factions", "[\"x\", \"y\"]"));
    const MergeResult r = MergeJsonKeys(before, updates);
    CHECK(r.ok);
    CHECK(Has(r.text, "\"dead\": false,"));
    CHECK(Has(r.text, "\"factions\": [\"x\", \"y\"]"));
    CHECK(Count(r.text, "\"dead\"") == 1);
    CHECK(r.appended == 0);
}

void EmptyAndMissingFilesProduceAFreshDocument()
{
    const char* name = "json_merge: no file yet is a fresh document";
    std::vector<JsonUpdate> updates;
    updates.push_back(JsonUpdate("enabled", "true"));
    updates.push_back(JsonUpdate("preset", "1"));

    const MergeResult blank = MergeJsonKeys("", updates);
    CHECK(blank.ok);
    CHECK(blank.text == "{\n  \"enabled\": true,\n  \"preset\": 1\n}\n");

    const MergeResult spaces = MergeJsonKeys("  \n\t ", updates);
    CHECK(spaces.ok);
    CHECK(spaces.text == blank.text);

    const MergeResult emptyObject = MergeJsonKeys("{}", updates);
    CHECK(emptyObject.ok);
    CHECK(emptyObject.text == "{\n  \"enabled\": true,\n  \"preset\": 1\n}");
}

void BrokenFilesAreNotGuessedAt()
{
    const char* name = "json_merge: a file that does not parse is reported, not rewritten";
    const MergeResult truncated = MergeJsonKeys("{\n  \"enabled\": tr", One("enabled", "true"));
    CHECK(!truncated.ok);
    CHECK(truncated.text.empty());

    const MergeResult notAnObject = MergeJsonKeys("[1, 2, 3]", One("enabled", "true"));
    CHECK(!notAnObject.ok);

    const MergeResult unbalanced = MergeJsonKeys("{ \"a\": { \"b\": 1 }", One("a", "2"));
    CHECK(!unbalanced.ok);

    const MergeResult noColon = MergeJsonKeys("{ \"a\" 1 }", One("a", "2"));
    CHECK(!noColon.ok);
}

void LineEndingsAndBomSurvive()
{
    const char* name = "json_merge: CRLF files stay CRLF and a BOM stays put";
    const std::string before = "\xEF\xBB\xBF{\r\n  \"enabled\": true\r\n}\r\n";
    const MergeResult r = MergeJsonKeys(before, One("preset", "1"));
    CHECK(r.ok);
    CHECK(r.text == "\xEF\xBB\xBF{\r\n  \"enabled\": true,\r\n  \"preset\": 1\r\n}\r\n");
}

void IndentFollowsTheFile()
{
    const char* name = "json_merge: appended keys use the file's own indent";
    const std::string before = "{\n\t\"enabled\": true\n}\n";
    const MergeResult r = MergeJsonKeys(before, One("preset", "1"));
    CHECK(r.ok);
    CHECK(Has(r.text, "\n\t\"preset\": 1\n}"));
}

void MergingTwiceChangesNothing()
{
    const char* name = "json_merge: merging the same updates again is a no-op";
    std::vector<JsonUpdate> updates;
    updates.push_back(JsonUpdate("enabled", "false"));
    updates.push_back(JsonUpdate("preset", "2"));
    updates.push_back(JsonUpdate("recruitLabel", JsonString("A \"b\"")));
    const MergeResult once = MergeJsonKeys("{\n  \"enabled\": true,\n  \"other\": 5\n}\n", updates);
    CHECK(once.ok);
    const MergeResult twice = MergeJsonKeys(once.text, updates);
    CHECK(twice.ok);
    CHECK(twice.text == once.text);
    CHECK(twice.replaced == 0 && twice.appended == 0);
    CHECK(twice.kept == 3);
}

void SingleLineObjectsStayValid()
{
    const char* name = "json_merge: a one-line object gets a valid appended key";
    const MergeResult r = MergeJsonKeys("{\"a\": 1}", One("b", "2"));
    CHECK(r.ok);
    CHECK(Has(r.text, "\"a\": 1,"));
    CHECK(Has(r.text, "\"b\": 2"));
    CHECK(Count(r.text, "{") == 1 && Count(r.text, "}") == 1);
}

void EncodersProduceReadableJson()
{
    const char* name = "json_merge: value encoders";
    CHECK(JsonBool(true) == "true");
    CHECK(JsonBool(false) == "false");
    CHECK(JsonInt(-42) == "-42");
    CHECK(JsonInt(150000) == "150000");
    CHECK(JsonFloat(0.6f) == "0.6");
    CHECK(JsonFloat(6.0f) == "6");
    CHECK(JsonFloat(0.09f) == "0.09");
    CHECK(JsonFloat(31.0f) == "31");
    CHECK(JsonFloat(0.0f) == "0");
    CHECK((float)atof(JsonFloat(0.1234567f).c_str()) == 0.1234567f);   // reads back exactly
    CHECK(JsonString("plain") == "\"plain\"");
    CHECK(JsonString("a\"b\\c") == "\"a\\\"b\\\\c\"");
    CHECK(JsonString("line\nbreak\ttab") == "\"line\\nbreak\\ttab\"");
    std::vector<std::string> names;
    CHECK(JsonStringArray(names) == "[]");
    names.push_back("x");
    names.push_back("y\"z");
    CHECK(JsonStringArray(names) == "[\"x\", \"y\\\"z\"]");
}
} // namespace

void RunJsonMergeTests()
{
    ReplacesAKnownValueInPlace();
    UnknownKeysAndCommentsSurvive();
    EqualValuesKeepTheirOriginalText();
    MissingKeysAreAppended();
    AppendingAfterATrailingCommaStaysValid();
    StringsDoNotConfuseTheScanner();
    ACommentedOutKeyIsNotTheKey();
    KeyNamesMatchWholly();
    NestedKeysAreUpdatedWhereTheyAre();
    EmptyAndMissingFilesProduceAFreshDocument();
    BrokenFilesAreNotGuessedAt();
    LineEndingsAndBomSurvive();
    IndentFollowsTheFile();
    MergingTwiceChangesNothing();
    SingleLineObjectsStayValid();
    EncodersProduceReadableJson();
}
