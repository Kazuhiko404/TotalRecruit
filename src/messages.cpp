#include "common.h"
#include "messages.h"
#include "log.h"

#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <set>

namespace totalrecruit
{
namespace
{
// Flattened: "faction.aliado.greeting" -> one or more alternatives. Flattening
// at load time keeps every lookup a single map hit, which matters because these
// run inside the right-click path.
std::map<std::string, std::vector<std::string> > g_lines;

// Keys the code asked for and the file did not have. A set, and reported once,
// because Say() runs inside the right-click path and per-call logging there
// would drown the log it is meant to help.
std::set<std::string> g_missing;

void NoteMissingKey(const std::string& path)
{
    g_missing.insert(path);
}

std::string Slurp(const std::string& path)
{
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file.is_open())
    {
        return std::string();
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Reads one JSON string starting at the opening quote. Handles \" and \\ so a
// line can contain a quotation mark; everything else passes through as written,
// which is what a translator would expect.
bool ReadString(const std::string& text, size_t* pos, std::string* out)
{
    size_t i = *pos;
    if (i >= text.size() || text[i] != '"')
    {
        return false;
    }
    ++i;

    std::string value;
    while (i < text.size() && text[i] != '"')
    {
        if (text[i] == '\\' && (i + 1) < text.size())
        {
            const char next = text[i + 1];
            if (next == 'n')      { value += '\n'; }
            else if (next == 't') { value += '\t'; }
            else                  { value += next; }
            i += 2;
            continue;
        }
        value += text[i];
        ++i;
    }
    if (i >= text.size())
    {
        return false;
    }

    *pos = i + 1;
    *out = value;
    return true;
}

void SkipSpace(const std::string& text, size_t* pos)
{
    while (*pos < text.size() && isspace((unsigned char)text[*pos]))
    {
        ++(*pos);
    }
}

// A small recursive walker rather than a general JSON library: the shape here
// is only objects, strings and arrays of strings, and a dependency for that
// would be a poor trade in a plugin that has to stay loadable.
bool ParseObject(const std::string& text, size_t* pos, const std::string& prefix);

bool ParseValue(const std::string& text, size_t* pos, const std::string& path)
{
    SkipSpace(text, pos);
    if (*pos >= text.size())
    {
        return false;
    }

    const char c = text[*pos];

    if (c == '"')
    {
        std::string value;
        if (!ReadString(text, pos, &value))
        {
            return false;
        }
        g_lines[path].push_back(value);
        return true;
    }

    if (c == '[')
    {
        ++(*pos);
        for (;;)
        {
            SkipSpace(text, pos);
            if (*pos >= text.size())
            {
                return false;
            }
            if (text[*pos] == ']')
            {
                ++(*pos);
                return true;
            }
            if (text[*pos] == ',')
            {
                ++(*pos);
                continue;
            }
            std::string value;
            if (!ReadString(text, pos, &value))
            {
                return false;
            }
            g_lines[path].push_back(value);
        }
    }

    if (c == '{')
    {
        return ParseObject(text, pos, path + ".");
    }

    // Numbers, booleans and null are not used by this file; skipping them keeps
    // the parser from choking on a key someone adds by hand.
    while (*pos < text.size() && text[*pos] != ',' && text[*pos] != '}' &&
           text[*pos] != ']')
    {
        ++(*pos);
    }
    return true;
}

bool ParseObject(const std::string& text, size_t* pos, const std::string& prefix)
{
    SkipSpace(text, pos);
    if (*pos >= text.size() || text[*pos] != '{')
    {
        return false;
    }
    ++(*pos);

    for (;;)
    {
        SkipSpace(text, pos);
        if (*pos >= text.size())
        {
            return false;
        }
        if (text[*pos] == '}')
        {
            ++(*pos);
            return true;
        }
        if (text[*pos] == ',')
        {
            ++(*pos);
            continue;
        }

        std::string key;
        if (!ReadString(text, pos, &key))
        {
            return false;
        }

        SkipSpace(text, pos);
        if (*pos >= text.size() || text[*pos] != ':')
        {
            return false;
        }
        ++(*pos);

        if (!ParseValue(text, pos, prefix + key))
        {
            return false;
        }
    }
}

// A stable index for this character, so a list entry is picked once and stays
// picked. Using the pointer keeps it free; it changes across saves, which only
// means an NPC may greet you differently in a new session.
size_t StableIndex(Character* who, size_t count)
{
    if (count <= 1 || who == NULL)
    {
        return 0;
    }
    const size_t bits = (size_t)((uintptr_t)who >> 4);
    return bits % count;
}
} // namespace

void LoadMessages(const std::string& modDirectory)
{
    g_lines.clear();

    const std::string path = modDirectory + "\\messages.json";
    const std::string text = Slurp(path);
    if (text.empty())
    {
        LogInfo("No messages.json; using the built-in lines.");
        return;
    }

    size_t pos = 0;
    if (!ParseObject(text, &pos, ""))
    {
        // Whatever parsed before the damage is kept: a translator who breaks
        // one line should not lose the other fifty.
        LogError("messages.json is malformed; keeping whatever parsed and "
                 "falling back to the built-in lines for the rest.");
    }

    char buf[64] = { 0 };
    _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%u", (unsigned)g_lines.size());
    LogInfo("Loaded " + std::string(buf) + " message keys from messages.json.");
}

std::string Say(const std::string& path, const std::string& fallback,
                Character* who)
{
    std::map<std::string, std::vector<std::string> >::const_iterator found =
        g_lines.find(path);
    if (found == g_lines.end() || found->second.empty())
    {
        // The fallback keeps the game playable, which is right for a player and
        // wrong for us: as more logic lands, a key nobody wrote would just
        // quietly serve the built-in line forever. Name it once, in the log, so
        // it can be added to messages.json.
        NoteMissingKey(path);
        return fallback;
    }
    return found->second[StableIndex(who, found->second.size())];
}

void ReportMissingKeys()
{
    if (g_missing.empty())
    {
        return;
    }

    std::string list;
    for (std::set<std::string>::const_iterator it = g_missing.begin();
         it != g_missing.end(); ++it)
    {
        if (!list.empty())
        {
            list += ", ";
        }
        list += *it;
    }
    LogInfo("messages.json has no entry for: " + list +
            " (the built-in text was used instead).");
}

std::string Fill(const std::string& text, const std::string& name,
                 int cats, int hours)
{
    std::string out = text;

    char number[32] = { 0 };
    _snprintf_s(number, sizeof(number), _TRUNCATE, "%d", cats);
    const std::string catsText = number;
    _snprintf_s(number, sizeof(number), _TRUNCATE, "%d", hours);
    const std::string hoursText = number;

    const char* keys[3] = { "{name}", "{cats}", "{hours}" };
    const std::string values[3] = { name, catsText, hoursText };

    for (int i = 0; i < 3; ++i)
    {
        for (;;)
        {
            const size_t at = out.find(keys[i]);
            if (at == std::string::npos)
            {
                break;
            }
            out = out.substr(0, at) + values[i] +
                  out.substr(at + strlen(keys[i]));
        }
    }
    return out;
}
} // namespace totalrecruit
