#pragma once

// Merging known keys into a hand-edited JSON file, without the engine.
//
// SaveConfig used to truncate config.json and rewrite only the keys the plugin
// knew about, which threw away every comment, every unknown key and the player's
// own ordering and formatting on the first Mod Hub click. This module is the text
// logic that replaces that: it reads the existing text, replaces the values of
// the keys it was given in place, appends the ones that are missing, and leaves
// every other byte alone. It is not a general JSON library -- it understands just
// enough structure (strings, nesting, // and /* */ comments, trailing commas) to
// know where a value starts and ends.

#include <string>
#include <vector>

namespace totalrecruit
{
struct JsonUpdate
{
    std::string key;
    std::string value;   // already encoded as JSON (use the Json* encoders below)

    JsonUpdate() {}
    JsonUpdate(const std::string& k, const std::string& v) : key(k), value(v) {}
};

struct MergeResult
{
    // False when `existing` is not something a merge can be trusted on (it does
    // not parse as one object). `text` is empty then and the caller decides what
    // to do -- SaveConfig keeps a backup and writes a fresh file.
    bool ok;
    std::string text;
    int replaced;   // values rewritten because they differed
    int appended;   // keys that were missing and were added
    int kept;       // values already equal to the update, left byte for byte

    MergeResult() : ok(false), replaced(0), appended(0), kept(0) {}
};

// Merges `updates` into `existing`. A key is matched by name at any depth, the
// first occurrence wins (the same rule the config reader uses). A value that is
// already equal to the update (0.60 vs 0.6, [ "a" ] vs ["a"]) is left exactly as
// the author wrote it. Missing keys are appended to the top-level object. An empty
// or whitespace-only `existing` yields a fresh document.
MergeResult MergeJsonKeys(const std::string& existing, const std::vector<JsonUpdate>& updates);

// Encoders for the values.
std::string JsonBool(bool value);
std::string JsonInt(int value);
// Shortest text that reads back as the same float.
std::string JsonFloat(float value);
// Quoted, with quotes, backslashes and control characters escaped.
std::string JsonString(const std::string& value);
std::string JsonStringArray(const std::vector<std::string>& values);
} // namespace totalrecruit
