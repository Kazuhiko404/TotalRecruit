#include "common.h"
#include "config.h"
#include "log.h"

#include <cstdlib>
#include <fstream>
#include <sstream>

namespace totalrecruit
{
namespace
{
Config g_config;

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

// The config schema is flat and fully known, so keys are located directly
// instead of building a general JSON parser.
bool FindKey(const std::string& text, const std::string& key, size_t* outValueStart)
{
    const std::string quoted = "\"" + key + "\"";
    const size_t keyPos = text.find(quoted);
    if (keyPos == std::string::npos)
    {
        return false;
    }

    const size_t colon = text.find(':', keyPos + quoted.size());
    if (colon == std::string::npos)
    {
        return false;
    }

    size_t valueStart = colon + 1;
    while (valueStart < text.size() && isspace((unsigned char)text[valueStart]))
    {
        ++valueStart;
    }
    if (valueStart >= text.size())
    {
        return false;
    }

    *outValueStart = valueStart;
    return true;
}

void ReadBool(const std::string& text, const std::string& key, bool* target)
{
    size_t valueStart = 0;
    if (!FindKey(text, key, &valueStart))
    {
        return;
    }

    if (text.compare(valueStart, 4, "true") == 0)
    {
        *target = true;
    }
    else if (text.compare(valueStart, 5, "false") == 0)
    {
        *target = false;
    }
}

void ReadInt(const std::string& text, const std::string& key, int* target)
{
    size_t valueStart = 0;
    if (!FindKey(text, key, &valueStart))
    {
        return;
    }

    const int parsed = atoi(text.c_str() + valueStart);
    if (parsed != 0 || text[valueStart] == '0')
    {
        *target = parsed;
    }
}

// atof returns 0.0 both for "0" and for garbage, so the value is only taken
// when the text actually starts like a number.
void ReadFloat(const std::string& text, const std::string& key, float* target)
{
    size_t valueStart = 0;
    if (!FindKey(text, key, &valueStart))
    {
        return;
    }

    const char first = text[valueStart];
    const bool looksNumeric = (first >= '0' && first <= '9') ||
                              first == '-' || first == '+' || first == '.';
    if (looksNumeric)
    {
        *target = (float)atof(text.c_str() + valueStart);
    }
}

void ReadString(const std::string& text, const std::string& key, std::string* target)
{
    size_t valueStart = 0;
    if (!FindKey(text, key, &valueStart) || text[valueStart] != '"')
    {
        return;
    }

    const size_t end = text.find('"', valueStart + 1);
    if (end == std::string::npos)
    {
        return;
    }
    *target = text.substr(valueStart + 1, end - valueStart - 1);
}

void ReadStringArray(const std::string& text, const std::string& key,
                     std::vector<std::string>* target)
{
    size_t valueStart = 0;
    if (!FindKey(text, key, &valueStart) || text[valueStart] != '[')
    {
        return;
    }

    const size_t close = text.find(']', valueStart);
    if (close == std::string::npos)
    {
        return;
    }

    target->clear();
    size_t cursor = valueStart + 1;
    while (cursor < close)
    {
        const size_t open = text.find('"', cursor);
        if (open == std::string::npos || open > close)
        {
            break;
        }
        const size_t end = text.find('"', open + 1);
        if (end == std::string::npos || end > close)
        {
            break;
        }
        target->push_back(text.substr(open + 1, end - open - 1));
        cursor = end + 1;
    }
}
} // namespace

void LoadConfig(const std::string& modDirectory)
{
    g_config = Config();

    const std::string path = modDirectory + "\\config.json";
    const std::string text = Slurp(path);
    if (text.empty())
    {
        LogInfo("No config.json found; using defaults (no restrictions).");
        return;
    }

    ReadBool(text, "enabled", &g_config.enabled);
    ReadBool(text, "showRecruitButton", &g_config.showRecruitButton);
    ReadBool(text, "autoRecruit", &g_config.autoRecruit);
    ReadString(text, "recruitLabel", &g_config.recruitLabel);
    ReadInt(text, "maxSpawnedCharacters", &g_config.maxSpawnedCharacters);
    ReadInt(text, "panelToggleKey", &g_config.panelToggleKey);
    ReadBool(text, "probeOnClick", &g_config.probeOnClick);
    ReadBool(text, "showTalkCursor", &g_config.showTalkCursor);

    ReadInt(text, "preset", &g_config.preset);
    ReadBool(text, "rescueEnabled", &g_config.rescueEnabled);
    ReadBool(text, "rescuePlayerOnly", &g_config.rescuePlayerOnly);
    ReadBool(text, "captivityEnabled", &g_config.captivityEnabled);
    ReadBool(text, "surrenderEnabled", &g_config.surrenderEnabled);
    ReadBool(text, "animalsEnabled", &g_config.animalsEnabled);

    ReadInt(text, "catsPerLevel", &g_config.prices.catsPerLevel);
    ReadInt(text, "catsMinimum", &g_config.prices.catsMinimum);
    ReadFloat(text, "hungerThreshold", &g_config.prices.hungerThreshold);
    ReadFloat(text, "woundedThreshold", &g_config.prices.woundedThreshold);
    ReadFloat(text, "hoursCagedForProud", &g_config.prices.hoursCagedForProud);
    ReadFloat(text, "fearfulMultiplier", &g_config.prices.fearfulMultiplier);
    ReadFloat(text, "braveMultiplier", &g_config.prices.braveMultiplier);
    ReadFloat(text, "uniqueExtraHours", &g_config.prices.uniqueExtraHours);
    ReadFloat(text, "factionWeight", &g_config.prices.factionWeight);

    ReadString(text, "dialoguePackage", &g_config.dialogue.package);
    ReadString(text, "dialogueRescue", &g_config.dialogue.rescue);
    ReadString(text, "dialogueCaptive", &g_config.dialogue.captive);
    ReadString(text, "dialogueAcceptKey", &g_config.dialogue.acceptKey);

    ReadBool(text, "dead", &g_config.exclude.dead);
    ReadBool(text, "unconscious", &g_config.exclude.unconscious);
    ReadBool(text, "animals", &g_config.exclude.animals);
    ReadBool(text, "sameFaction", &g_config.exclude.sameFaction);
    ReadStringArray(text, "factions", &g_config.exclude.factions);
    ReadStringArray(text, "characterNames", &g_config.exclude.characterNames);

    LogInfo("Loaded config from " + path);
}

Config& GetConfig()
{
    return g_config;
}
} // namespace totalrecruit
