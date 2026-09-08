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
std::string g_modDirectory;

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
void ClampFloat(float* value, float low, float high)
{
    if (*value < low) *value = low;
    if (*value > high) *value = high;
}

void ClampInt(int* value, int low, int high)
{
    if (*value < low) *value = low;
    if (*value > high) *value = high;
}

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
    ReadBool(text, "resolveCageOccupant", &g_config.resolveCageOccupant);
    ReadBool(text, "hailWhenReady", &g_config.hailWhenReady);
    ReadFloat(text, "hailFirstHours", &g_config.hailFirstHours);
    ReadFloat(text, "hailRepeatHours", &g_config.hailRepeatHours);
    ReadInt(text, "hailRepeats", &g_config.hailRepeats);

    ReadFloat(text, "friendlyRelation", &g_config.friendlyRelation);
    ReadBool(text, "cheatNeedsCage", &g_config.cheatNeedsCage);
    ReadFloat(text, "cheatHoursCaged", &g_config.cheatHoursCaged);
    ReadInt(text, "rescueBaseBonus", &g_config.rescueBaseBonus);
    ReadFloat(text, "rescueHealWeight", &g_config.rescueHealWeight);

    ReadInt(text, "insistenceAttempts", &g_config.insistence.attemptsBeforeIgnored);
    ReadFloat(text, "insistenceBaseHours", &g_config.insistence.baseHours);
    ReadFloat(text, "insistenceMaxHours", &g_config.insistence.maxHours);
    ReadFloat(text, "insistenceForgetHours", &g_config.insistence.forgetAfterHours);

    ReadInt(text, "preset", &g_config.preset);
    ReadBool(text, "rescueEnabled", &g_config.rescueEnabled);
    ReadBool(text, "rescuePlayerOnly", &g_config.rescuePlayerOnly);
    ReadBool(text, "captivityEnabled", &g_config.captivityEnabled);
    ReadBool(text, "surrenderEnabled", &g_config.surrenderEnabled);
    ReadBool(text, "animalsEnabled", &g_config.animalsEnabled);

    ReadFloat(text, "easyHoursCaged", &g_config.prices.easyHoursCaged);
    ReadInt(text, "catsPerLevel", &g_config.prices.catsPerLevel);
    ReadInt(text, "catsCeiling", &g_config.prices.catsCeiling);
    ReadFloat(text, "priceCurveSteepness", &g_config.prices.priceCurveSteepness);
    ReadFloat(text, "priceCurveMidpoint", &g_config.prices.priceCurveMidpoint);
    ReadInt(text, "catsMinimum", &g_config.prices.catsMinimum);
    ReadFloat(text, "hungerThreshold", &g_config.prices.hungerThreshold);
    ReadFloat(text, "woundedThreshold", &g_config.prices.woundedThreshold);

    ReadInt(text, "chanceLevelBase", &g_config.chance.levelBase);
    ReadFloat(text, "chanceLevelGapPenalty", &g_config.chance.levelGapPenalty);
    ReadInt(text, "chanceLevelGapFloor", &g_config.chance.levelGapFloor);
    ReadFloat(text, "chanceLevelEdgeBonus", &g_config.chance.levelEdgeBonus);
    ReadInt(text, "chanceLevelEdgeCap", &g_config.chance.levelEdgeCap);
    ReadInt(text, "chanceFloor", &g_config.chance.floor);
    ReadInt(text, "chanceCeiling", &g_config.chance.ceiling);
    ReadInt(text, "chanceIntimidationBonus", &g_config.chance.intimidationBonus);
    ReadInt(text, "chanceGrudgePenalty", &g_config.chance.grudgePenalty);
    ReadInt(text, "chanceUniquePenalty", &g_config.chance.uniquePenalty);
    ReadBool(text, "chanceGrudgeIsAbsolute", &g_config.chance.grudgeIsAbsolute);

    ReadBool(text, "submissionCountDefeats", &g_config.submission.countDefeats);
    ReadFloat(text, "submissionDefeatDebounceHours", &g_config.submission.defeatDebounceHours);
    ReadInt(text, "submissionPersistedFloorCount", &g_config.submission.persistedFloorCount);
    ReadInt(text, "submissionChancePerDefeat", &g_config.submission.chancePerDefeat);
    ReadInt(text, "submissionMaxDefeatChance", &g_config.submission.maxDefeatChance);
    ReadBool(text, "submissionBegWhenNearDeath", &g_config.submission.begWhenNearDeath);
    ReadFloat(text, "submissionNearDeathBlood", &g_config.submission.nearDeathBlood);
    ReadFloat(text, "submissionFearfulResolve", &g_config.submission.fearfulResolve);
    ReadFloat(text, "submissionBraveResolve", &g_config.submission.braveResolve);
    ReadFloat(text, "submissionHonourableResolve", &g_config.submission.honourableResolve);
    ReadInt(text, "submissionDefeatsToBreak", &g_config.submission.defeatsToBreak);
    ReadInt(text, "submissionFieldYieldBonus", &g_config.submission.fieldYieldBonus);
    ReadFloat(text, "submissionStressFromHunger", &g_config.submission.stressFromHunger);
    ReadFloat(text, "submissionStressFromWounds", &g_config.submission.stressFromWounds);
    ReadFloat(text, "submissionStressFromExhaustion", &g_config.submission.stressFromExhaustion);
    ReadInt(text, "submissionMaxStressChance", &g_config.submission.maxStressChance);
    ReadBool(text, "submissionAllowBeating", &g_config.submission.allowBeating);
    ReadFloat(text, "submissionBeatingCooldownHours", &g_config.submission.beatingCooldownHours);
    ReadFloat(text, "submissionBeatingKnockoutSeconds", &g_config.submission.beatingKnockoutSeconds);

    ReadFloat(text, "animalCubMaturity", &g_config.animals.cubMaturity);
    ReadInt(text, "animalWeakLevel", &g_config.animals.weakLevel);
    ReadFloat(text, "animalCubFullness", &g_config.animals.cubFullness);
    ReadFloat(text, "animalBaseFullness", &g_config.animals.baseFullness);
    ReadFloat(text, "animalMaturityWeight", &g_config.animals.maturityWeight);
    ReadInt(text, "animalStrongLevel", &g_config.animals.strongLevel);
    ReadFloat(text, "animalStrongPenalty", &g_config.animals.strongPenalty);
    ReadFloat(text, "animalEdgeDiscount", &g_config.animals.edgeDiscount);
    ReadFloat(text, "animalBigEdgeDiscount", &g_config.animals.bigEdgeDiscount);
    ReadBool(text, "animalOwnedNeedCage", &g_config.animals.ownedNeedCage);
    ReadFloat(text, "animalNutritionPerMeal", &g_config.animals.nutritionPerMeal);
    ReadBool(text, "animalPersonalityMatters", &g_config.animals.personalityMatters);
    ReadFloat(text, "hoursCagedForProud", &g_config.prices.hoursCagedForProud);
    ReadFloat(text, "fearfulMultiplier", &g_config.prices.fearfulMultiplier);
    ReadFloat(text, "braveMultiplier", &g_config.prices.braveMultiplier);
    ReadFloat(text, "uniqueExtraHours", &g_config.prices.uniqueExtraHours);
    ReadFloat(text, "factionWeight", &g_config.prices.factionWeight);
    ReadFloat(text, "talentWeight", &g_config.prices.talentWeight);
    ReadFloat(text, "uniqueMultiplier", &g_config.prices.uniqueMultiplier);

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

    // Clamped after reading rather than rejected: a nonsensical value should
    // pull back to a sane one, not disable the feature or drop the whole file.
    ClampFloat(&g_config.hailFirstHours, kHailFirstHoursMin, kHailFirstHoursMax);
    ClampFloat(&g_config.hailRepeatHours, kHailRepeatHoursMin, kHailRepeatHoursMax);
    ClampInt(&g_config.hailRepeats, kHailRepeatsMin, kHailRepeatsMax);

    g_modDirectory = modDirectory;
    LogInfo("Loaded config from " + path);
}

std::string GetModDirectory()
{
    return g_modDirectory;
}

bool SaveConfig(const std::string& modDirectory)
{
    std::string dir = modDirectory.empty() ? g_modDirectory : modDirectory;
    if (dir.empty())
    {
        return false;
    }

    const std::string path = dir + "\\config.json";
    std::ofstream file(path.c_str(), std::ios::trunc);
    if (!file.is_open())
    {
        LogError("Failed to open " + path + " for writing config.");
        return false;
    }

    file << "{\n";
    file << "  \"enabled\": " << (g_config.enabled ? "true" : "false") << ",\n";
    file << "  \"showRecruitButton\": " << (g_config.showRecruitButton ? "true" : "false") << ",\n";
    file << "  \"autoRecruit\": " << (g_config.autoRecruit ? "true" : "false") << ",\n";
    file << "  \"recruitLabel\": \"" << g_config.recruitLabel << "\",\n";
    file << "  \"maxSpawnedCharacters\": " << g_config.maxSpawnedCharacters << ",\n";
    file << "  \"panelToggleKey\": " << g_config.panelToggleKey << ",\n";
    file << "  \"probeOnClick\": " << (g_config.probeOnClick ? "true" : "false") << ",\n";
    file << "  \"showTalkCursor\": " << (g_config.showTalkCursor ? "true" : "false") << ",\n";
    file << "  \"resolveCageOccupant\": " << (g_config.resolveCageOccupant ? "true" : "false") << ",\n";
    file << "  \"hailWhenReady\": " << (g_config.hailWhenReady ? "true" : "false") << ",\n";
    file << "  \"hailFirstHours\": " << g_config.hailFirstHours << ",\n";
    file << "  \"hailRepeatHours\": " << g_config.hailRepeatHours << ",\n";
    file << "  \"hailRepeats\": " << g_config.hailRepeats << ",\n\n";

    file << "  \"preset\": " << g_config.preset << ",\n\n";

    file << "  \"rescueEnabled\": " << (g_config.rescueEnabled ? "true" : "false") << ",\n";
    file << "  \"rescuePlayerOnly\": " << (g_config.rescuePlayerOnly ? "true" : "false") << ",\n";
    file << "  \"captivityEnabled\": " << (g_config.captivityEnabled ? "true" : "false") << ",\n";
    file << "  \"surrenderEnabled\": " << (g_config.surrenderEnabled ? "true" : "false") << ",\n";
    file << "  \"animalsEnabled\": " << (g_config.animalsEnabled ? "true" : "false") << ",\n\n";

    file << "  \"easyHoursCaged\": " << g_config.prices.easyHoursCaged << ",\n";
    file << "  \"catsPerLevel\": " << g_config.prices.catsPerLevel << ",\n";
    file << "  \"catsMinimum\": " << g_config.prices.catsMinimum << ",\n";
    file << "  \"hungerThreshold\": " << g_config.prices.hungerThreshold << ",\n";
    file << "  \"woundedThreshold\": " << g_config.prices.woundedThreshold << ",\n";

    file << "  \"friendlyRelation\": " << g_config.friendlyRelation << ",\n";
    file << "  \"cheatNeedsCage\": " << (g_config.cheatNeedsCage ? "true" : "false") << ",\n";
    file << "  \"cheatHoursCaged\": " << g_config.cheatHoursCaged << ",\n";
    file << "  \"rescueBaseBonus\": " << g_config.rescueBaseBonus << ",\n";
    file << "  \"rescueHealWeight\": " << g_config.rescueHealWeight << ",\n";
    file << "  \"insistenceAttempts\": " << g_config.insistence.attemptsBeforeIgnored << ",\n";
    file << "  \"insistenceBaseHours\": " << g_config.insistence.baseHours << ",\n";
    file << "  \"insistenceMaxHours\": " << g_config.insistence.maxHours << ",\n";
    file << "  \"insistenceForgetHours\": " << g_config.insistence.forgetAfterHours << ",\n";

    file << "  \"chanceLevelBase\": " << g_config.chance.levelBase << ",\n";
    file << "  \"chanceLevelGapPenalty\": " << g_config.chance.levelGapPenalty << ",\n";
    file << "  \"chanceLevelGapFloor\": " << g_config.chance.levelGapFloor << ",\n";
    file << "  \"chanceLevelEdgeBonus\": " << g_config.chance.levelEdgeBonus << ",\n";
    file << "  \"chanceLevelEdgeCap\": " << g_config.chance.levelEdgeCap << ",\n";
    file << "  \"chanceFloor\": " << g_config.chance.floor << ",\n";
    file << "  \"chanceCeiling\": " << g_config.chance.ceiling << ",\n";

    file << "  \"submissionCountDefeats\": " << (g_config.submission.countDefeats ? "true" : "false") << ",\n";
    file << "  \"submissionDefeatDebounceHours\": " << g_config.submission.defeatDebounceHours << ",\n";
    file << "  \"submissionPersistedFloorCount\": " << g_config.submission.persistedFloorCount << ",\n";
    file << "  \"submissionChancePerDefeat\": " << g_config.submission.chancePerDefeat << ",\n";
    file << "  \"submissionMaxDefeatChance\": " << g_config.submission.maxDefeatChance << ",\n";
    file << "  \"submissionBegWhenNearDeath\": " << (g_config.submission.begWhenNearDeath ? "true" : "false") << ",\n";
    file << "  \"submissionNearDeathBlood\": " << g_config.submission.nearDeathBlood << ",\n";
    file << "  \"submissionFearfulResolve\": " << g_config.submission.fearfulResolve << ",\n";
    file << "  \"submissionBraveResolve\": " << g_config.submission.braveResolve << ",\n";
    file << "  \"submissionHonourableResolve\": " << g_config.submission.honourableResolve << ",\n";
    file << "  \"submissionDefeatsToBreak\": " << g_config.submission.defeatsToBreak << ",\n";
    file << "  \"submissionFieldYieldBonus\": " << g_config.submission.fieldYieldBonus << ",\n";
    file << "  \"submissionStressFromHunger\": " << g_config.submission.stressFromHunger << ",\n";
    file << "  \"submissionStressFromWounds\": " << g_config.submission.stressFromWounds << ",\n";
    file << "  \"submissionStressFromExhaustion\": " << g_config.submission.stressFromExhaustion << ",\n";
    file << "  \"submissionMaxStressChance\": " << g_config.submission.maxStressChance << ",\n";
    file << "  \"submissionAllowBeating\": " << (g_config.submission.allowBeating ? "true" : "false") << ",\n";
    file << "  \"submissionBeatingCooldownHours\": " << g_config.submission.beatingCooldownHours << ",\n";
    file << "  \"submissionBeatingKnockoutSeconds\": " << g_config.submission.beatingKnockoutSeconds << ",\n";

    file << "  \"animalCubMaturity\": " << g_config.animals.cubMaturity << ",\n";
    file << "  \"animalWeakLevel\": " << g_config.animals.weakLevel << ",\n";
    file << "  \"animalCubFullness\": " << g_config.animals.cubFullness << ",\n";
    file << "  \"animalBaseFullness\": " << g_config.animals.baseFullness << ",\n";
    file << "  \"animalMaturityWeight\": " << g_config.animals.maturityWeight << ",\n";
    file << "  \"animalStrongLevel\": " << g_config.animals.strongLevel << ",\n";
    file << "  \"animalStrongPenalty\": " << g_config.animals.strongPenalty << ",\n";
    file << "  \"animalEdgeDiscount\": " << g_config.animals.edgeDiscount << ",\n";
    file << "  \"animalBigEdgeDiscount\": " << g_config.animals.bigEdgeDiscount << ",\n";
    file << "  \"hoursCagedForProud\": " << g_config.prices.hoursCagedForProud << ",\n";
    file << "  \"uniqueExtraHours\": " << g_config.prices.uniqueExtraHours << ",\n";
    file << "  \"fearfulMultiplier\": " << g_config.prices.fearfulMultiplier << ",\n";
    file << "  \"braveMultiplier\": " << g_config.prices.braveMultiplier << ",\n";
    file << "  \"factionWeight\": " << g_config.prices.factionWeight << ",\n\n";

    file << "  \"dialoguePackage\": \"" << g_config.dialogue.package << "\",\n";
    file << "  \"dialogueRescue\": \"" << g_config.dialogue.rescue << "\",\n";
    file << "  \"dialogueCaptive\": \"" << g_config.dialogue.captive << "\",\n";
    file << "  \"dialogueAcceptKey\": \"" << g_config.dialogue.acceptKey << "\",\n\n";

    file << "  \"exclude\": {\n";
    file << "    \"dead\": " << (g_config.exclude.dead ? "true" : "false") << ",\n";
    file << "    \"unconscious\": " << (g_config.exclude.unconscious ? "true" : "false") << ",\n";
    file << "    \"animals\": " << (g_config.exclude.animals ? "true" : "false") << ",\n";
    file << "    \"sameFaction\": " << (g_config.exclude.sameFaction ? "true" : "false") << ",\n";
    file << "    \"factions\": [";
    for (size_t i = 0; i < g_config.exclude.factions.size(); ++i)
    {
        if (i > 0) file << ", ";
        file << "\"" << g_config.exclude.factions[i] << "\"";
    }
    file << "],\n";
    file << "    \"characterNames\": [";
    for (size_t i = 0; i < g_config.exclude.characterNames.size(); ++i)
    {
        if (i > 0) file << ", ";
        file << "\"" << g_config.exclude.characterNames[i] << "\"";
    }
    file << "]\n";
    file << "  }\n";
    file << "}\n";

    file.close();
    LogInfo("Saved config to " + path);
    return true;
}

Config& GetConfig()
{
    return g_config;
}
} // namespace totalrecruit
