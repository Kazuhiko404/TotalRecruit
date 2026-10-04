#include "common.h"
#include "config.h"
#include "log.h"
#include "json_merge.h"
#include "atomic_file.h"
#include "head_notice_rules.h"

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

// The ally tiers (ally_tier_rules.h): the cut levels kept in order, so a hand
// edit cannot make a tier start before the one below it; a discount never takes
// the whole price; a stay of at least one day, the longest no shorter than the
// shortest.
void ClampAllyTiers(AllyTiers* t)
{
    ClampInt(&t->midFromLevel, 1, 500);
    ClampInt(&t->highFromLevel, t->midFromLevel, 500);
    ClampInt(&t->eliteFromLevel, t->highFromLevel, 500);
    ClampInt(&t->legendFromLevel, t->eliteFromLevel, 500);
    ClampFloat(&t->lowMaxDiscount, 0.0f, 0.9f);
    ClampFloat(&t->midMaxDiscount, 0.0f, 0.9f);
    ClampFloat(&t->highMaxDiscount, 0.0f, 0.9f);
    ClampFloat(&t->eliteMaxDiscount, 0.0f, 0.9f);
    ClampFloat(&t->discountScale, 0.0f, 5.0f);
    ClampInt(&t->closeLevelGap, 0, 200);
    ClampInt(&t->tempMemberMinDays, 1, 60);
    ClampInt(&t->tempMemberMaxDays, t->tempMemberMinDays, 120);
}

// A temporary member's stay (temp_member_rules.h): at least one strike to leave,
// thresholds inside their range, and no wait longer than a month (the rejoin
// cooldown rides the reject cooldown, itself at most 720 hours).
void ClampTempMembers(TempMembers* t)
{
    ClampInt(&t->maxStrikes, 1, 10);
    ClampFloat(&t->hungerThreshold, 0.0f, 1.0f);
    ClampFloat(&t->hungryHours, 0.0f, 168.0f);
    ClampFloat(&t->strikeCooldownHours, 0.0f, 168.0f);
    ClampFloat(&t->expiryWarningHours, 0.0f, 72.0f);
    ClampInt(&t->rejoinCooldownDays, 0, 30);
}

// The free helper (helper_rules.h): a chance, a level lead and a stay inside their
// ranges, the longest stay no shorter than the shortest, a virtual-key code or 0.
void ClampHelpers(Helpers* h)
{
    ClampFloat(&h->offerChancePerDay, 0.0f, 1.0f);
    ClampInt(&h->minLevelLead, 0, 200);
    ClampInt(&h->daysMin, 1, 30);
    ClampInt(&h->daysMax, h->daysMin, 60);
    ClampInt(&h->rejectCooldownDays, 0, 30);
    ClampFloat(&h->expiryWarningHours, 0.0f, 72.0f);
    ClampInt(&h->debugKey, 0, 255);
}

// The duel challenge (duel_challenge_rules.h): a cooldown of up to a month, and a
// challenge waits at least half an hour and at most a week for its duel.
void ClampDuelChallenges(DuelChallenges* d)
{
    ClampFloat(&d->cooldownHours, 0.0f, 720.0f);
    ClampFloat(&d->expiryHours, 0.5f, 168.0f);
    // The join chance after a win (duel_join_rules.h) and the win cooldown band, up to
    // a month. A max below the min is left as it is: the rule reads it as the min.
    ClampInt(&d->joinBaseChance, 0, 100);
    ClampInt(&d->joinChancePerWin, 0, 100);
    ClampFloat(&d->winCooldownMinHours, 0.0f, 720.0f);
    ClampFloat(&d->winCooldownMaxHoursUnder100, 0.0f, 720.0f);
    ClampFloat(&d->winCooldownMaxHours, 0.0f, 720.0f);
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

// One table describes every setting: the file key, its type and where it lives.
// LoadConfig and SaveConfig both walk it, so a key can no longer be read but not
// saved (which is how the price curve, the grudge penalty and half a dozen
// animal settings used to be dropped on the first Mod Hub click).
enum FieldType
{
    Field_Bool,
    Field_Int,
    Field_Float,
    Field_String,
    Field_StringList
};

struct Field
{
    const char* key;
    FieldType type;
    void* target;
};

void Add(std::vector<Field>& fields, const char* key, FieldType type, void* target)
{
    Field field;
    field.key = key;
    field.type = type;
    field.target = target;
    fields.push_back(field);
}

void AddBool(std::vector<Field>& f, const char* key, bool* p)         { Add(f, key, Field_Bool, p); }
void AddInt(std::vector<Field>& f, const char* key, int* p)           { Add(f, key, Field_Int, p); }
void AddFloat(std::vector<Field>& f, const char* key, float* p)       { Add(f, key, Field_Float, p); }
void AddString(std::vector<Field>& f, const char* key, std::string* p) { Add(f, key, Field_String, p); }
void AddList(std::vector<Field>& f, const char* key, std::vector<std::string>* p)
{
    Add(f, key, Field_StringList, p);
}

// The order here is the order missing keys are appended to a file.
std::vector<Field> ConfigFields(Config& c)
{
    std::vector<Field> f;

    AddBool(f, "enabled", &c.enabled);
    AddBool(f, "showRecruitButton", &c.showRecruitButton);
    AddBool(f, "autoRecruit", &c.autoRecruit);
    AddString(f, "recruitLabel", &c.recruitLabel);
    AddInt(f, "maxSpawnedCharacters", &c.maxSpawnedCharacters);
    AddInt(f, "panelToggleKey", &c.panelToggleKey);
    AddBool(f, "probeOnClick", &c.probeOnClick);
    AddBool(f, "showTalkCursor", &c.showTalkCursor);
    AddBool(f, "resolveCageOccupant", &c.resolveCageOccupant);
    AddBool(f, "hailWhenReady", &c.hailWhenReady);
    AddFloat(f, "hailFirstHours", &c.hailFirstHours);
    AddFloat(f, "hailRepeatHours", &c.hailRepeatHours);
    AddInt(f, "hailRepeats", &c.hailRepeats);
    AddBool(f, "readoutEnabled", &c.readoutEnabled);
    AddBool(f, "readoutCompact", &c.readoutCompact);
    AddBool(f, "readoutOnSelect", &c.readoutOnSelect);
    AddFloat(f, "headNoticeSeconds", &c.headNoticeSeconds);
    AddFloat(f, "headNoticeInfoSeconds", &c.headNoticeInfoSeconds);

    AddInt(f, "preset", &c.preset);
    AddBool(f, "rescueEnabled", &c.rescueEnabled);
    AddBool(f, "rescuePlayerOnly", &c.rescuePlayerOnly);
    AddBool(f, "captivityEnabled", &c.captivityEnabled);
    AddBool(f, "surrenderEnabled", &c.surrenderEnabled);
    AddBool(f, "animalsEnabled", &c.animalsEnabled);

    AddFloat(f, "easyHoursCaged", &c.prices.easyHoursCaged);
    AddInt(f, "catsPerLevel", &c.prices.catsPerLevel);
    AddInt(f, "catsCeiling", &c.prices.catsCeiling);
    AddFloat(f, "priceCurveSteepness", &c.prices.priceCurveSteepness);
    AddFloat(f, "priceCurveMidpoint", &c.prices.priceCurveMidpoint);
    AddInt(f, "catsMinimum", &c.prices.catsMinimum);
    AddInt(f, "catsHardCap", &c.prices.catsHardCap);
    AddFloat(f, "hungerThreshold", &c.prices.hungerThreshold);
    AddFloat(f, "woundedThreshold", &c.prices.woundedThreshold);
    AddFloat(f, "fearfulMultiplier", &c.prices.fearfulMultiplier);
    AddFloat(f, "braveMultiplier", &c.prices.braveMultiplier);
    AddFloat(f, "factionWeight", &c.prices.factionWeight);
    AddFloat(f, "talentWeight", &c.prices.talentWeight);
    AddFloat(f, "talentMaxMultiplier", &c.prices.talentMaxMultiplier);
    AddFloat(f, "premiumMaxMultiplier", &c.prices.premiumMaxMultiplier);
    AddFloat(f, "uniqueMultiplier", &c.prices.uniqueMultiplier);
    AddFloat(f, "talentCraftWeight", &c.prices.craftFamilyWeight);
    AddFloat(f, "talentShadowWeight", &c.prices.shadowFamilyWeight);
    AddBool(f, "haggleEnabled", &c.prices.haggleEnabled);
    AddFloat(f, "haggleMinFraction", &c.prices.haggleMinFraction);
    AddFloat(f, "haggleDiscountWeight", &c.prices.haggleDiscountWeight);
    AddFloat(f, "haggleLeverageWeight", &c.prices.haggleLeverageWeight);
    AddFloat(f, "haggleOffenceMarkup", &c.prices.haggleOffenceMarkup);

    AddFloat(f, "friendlyRelation", &c.friendlyRelation);
    AddBool(f, "allyTierEnabled", &c.allyTiers.enabled);
    AddInt(f, "allyTierMidFromLevel", &c.allyTiers.midFromLevel);
    AddInt(f, "allyTierHighFromLevel", &c.allyTiers.highFromLevel);
    AddInt(f, "allyTierEliteFromLevel", &c.allyTiers.eliteFromLevel);
    AddInt(f, "allyTierLegendFromLevel", &c.allyTiers.legendFromLevel);
    AddFloat(f, "allyTierLowMaxDiscount", &c.allyTiers.lowMaxDiscount);
    AddFloat(f, "allyTierMidMaxDiscount", &c.allyTiers.midMaxDiscount);
    AddFloat(f, "allyTierHighMaxDiscount", &c.allyTiers.highMaxDiscount);
    AddFloat(f, "allyTierEliteMaxDiscount", &c.allyTiers.eliteMaxDiscount);
    AddFloat(f, "allyTierDiscountScale", &c.allyTiers.discountScale);
    AddInt(f, "allyTierCloseLevelGap", &c.allyTiers.closeLevelGap);
    AddInt(f, "allyTierTempMinDays", &c.allyTiers.tempMemberMinDays);
    AddInt(f, "allyTierTempMaxDays", &c.allyTiers.tempMemberMaxDays);
    AddInt(f, "tempMemberMaxStrikes", &c.tempMembers.maxStrikes);
    AddFloat(f, "tempMemberHungerThreshold", &c.tempMembers.hungerThreshold);
    AddFloat(f, "tempMemberHungryHours", &c.tempMembers.hungryHours);
    AddFloat(f, "tempMemberStrikeCooldownHours", &c.tempMembers.strikeCooldownHours);
    AddFloat(f, "tempMemberExpiryWarningHours", &c.tempMembers.expiryWarningHours);
    AddInt(f, "tempMemberRejoinCooldownDays", &c.tempMembers.rejoinCooldownDays);
    AddBool(f, "tempMemberDailyNotice", &c.tempMembers.dailyNotice);
    AddBool(f, "helperEnabled", &c.helpers.enabled);
    AddFloat(f, "helperOfferChancePerDay", &c.helpers.offerChancePerDay);
    AddInt(f, "helperMinLevelLead", &c.helpers.minLevelLead);
    AddInt(f, "helperDaysMin", &c.helpers.daysMin);
    AddInt(f, "helperDaysMax", &c.helpers.daysMax);
    AddInt(f, "helperRejectCooldownDays", &c.helpers.rejectCooldownDays);
    AddFloat(f, "helperExpiryWarningHours", &c.helpers.expiryWarningHours);
    AddInt(f, "helperDebugKey", &c.helpers.debugKey);
    AddBool(f, "helperNativeBodyguard", &c.helpers.nativeBodyguard);
    AddBool(f, "duelChallengeEnabled", &c.duelChallenges.enabled);
    AddFloat(f, "duelChallengeCooldownHours", &c.duelChallenges.cooldownHours);
    AddFloat(f, "duelChallengeExpiryHours", &c.duelChallenges.expiryHours);
    AddInt(f, "duelJoinBaseChance", &c.duelChallenges.joinBaseChance);
    AddInt(f, "duelJoinChancePerWin", &c.duelChallenges.joinChancePerWin);
    AddFloat(f, "duelWinCooldownMinHours", &c.duelChallenges.winCooldownMinHours);
    AddFloat(f, "duelWinCooldownMaxHoursUnder100", &c.duelChallenges.winCooldownMaxHoursUnder100);
    AddFloat(f, "duelWinCooldownMaxHours", &c.duelChallenges.winCooldownMaxHours);
    AddFloat(f, "relationRefuseGoodNatured", &c.relationRefuseGoodNatured);
    AddFloat(f, "relationRefuseAll", &c.relationRefuseAll);
    AddBool(f, "cheatNeedsCage", &c.cheatNeedsCage);
    AddFloat(f, "cheatHoursCaged", &c.cheatHoursCaged);
    AddInt(f, "rescueBaseBonus", &c.rescueBaseBonus);
    AddFloat(f, "rescueHealWeight", &c.rescueHealWeight);

    AddInt(f, "insistenceAttempts", &c.insistence.attemptsBeforeIgnored);
    AddFloat(f, "insistenceBaseHours", &c.insistence.baseHours);
    AddFloat(f, "insistenceMaxHours", &c.insistence.maxHours);
    AddFloat(f, "insistenceForgetHours", &c.insistence.forgetAfterHours);

    AddInt(f, "chanceLevelBase", &c.chance.levelBase);
    AddFloat(f, "chanceLevelGapPenalty", &c.chance.levelGapPenalty);
    AddInt(f, "chanceLevelGapFloor", &c.chance.levelGapFloor);
    AddFloat(f, "chanceLevelEdgeBonus", &c.chance.levelEdgeBonus);
    AddInt(f, "chanceLevelEdgeCap", &c.chance.levelEdgeCap);
    AddInt(f, "chanceFloor", &c.chance.floor);
    AddInt(f, "chanceCeiling", &c.chance.ceiling);
    AddInt(f, "chanceIntimidationBonus", &c.chance.intimidationBonus);
    AddInt(f, "chanceGrudgePenalty", &c.chance.grudgePenalty);
    AddInt(f, "chanceUniquePenalty", &c.chance.uniquePenalty);
    AddBool(f, "chanceGrudgeIsAbsolute", &c.chance.grudgeIsAbsolute);
    AddFloat(f, "chanceRelationWeight", &c.chance.relationWeight);
    AddInt(f, "chanceAlliedBonus", &c.chance.alliedBonus);
    AddInt(f, "chancePersonalityFearful", &c.chance.personality[Temperament_Fearful]);
    AddInt(f, "chancePersonalityTraitorous", &c.chance.personality[Temperament_Traitorous]);
    AddInt(f, "chancePersonalityDumb", &c.chance.personality[Temperament_Dumb]);
    AddInt(f, "chancePersonalitySmart", &c.chance.personality[Temperament_Smart]);
    AddInt(f, "chancePersonalityBrave", &c.chance.personality[Temperament_Brave]);
    AddInt(f, "chancePersonalityHonorable", &c.chance.personality[Temperament_Honorable]);
    AddInt(f, "chancePersonalityManiacal", &c.chance.personality[Temperament_Maniacal]);
    AddInt(f, "chancePersonalityColdCruel", &c.chance.personality[Temperament_ColdCruel]);
    AddInt(f, "chancePersonalityWarmKind", &c.chance.personality[Temperament_WarmKind]);

    AddBool(f, "submissionCountDefeats", &c.submission.countDefeats);
    AddInt(f, "submissionChancePerDefeat", &c.submission.chancePerDefeat);
    AddInt(f, "submissionMaxDefeatChance", &c.submission.maxDefeatChance);
    AddBool(f, "submissionBegWhenNearDeath", &c.submission.begWhenNearDeath);
    AddFloat(f, "submissionNearDeathBlood", &c.submission.nearDeathBlood);
    AddFloat(f, "submissionFearfulResolve", &c.submission.fearfulResolve);
    AddFloat(f, "submissionBraveResolve", &c.submission.braveResolve);
    AddFloat(f, "submissionHonourableResolve", &c.submission.honourableResolve);
    AddInt(f, "submissionDefeatsToBreak", &c.submission.defeatsToBreak);
    AddInt(f, "submissionFieldYieldBonus", &c.submission.fieldYieldBonus);
    AddInt(f, "submissionFieldYieldLoyaltyPenalty", &c.submission.fieldYieldLoyaltyPenalty);
    AddFloat(f, "submissionStressFromHunger", &c.submission.stressFromHunger);
    AddFloat(f, "submissionStressFromWounds", &c.submission.stressFromWounds);
    AddFloat(f, "submissionStressFromExhaustion", &c.submission.stressFromExhaustion);
    AddInt(f, "submissionMaxStressChance", &c.submission.maxStressChance);
    AddBool(f, "submissionAllowBeating", &c.submission.allowBeating);
    AddFloat(f, "submissionBeatingCooldownHours", &c.submission.beatingCooldownHours);
    AddFloat(f, "submissionKnockoutCooldownHours", &c.submission.knockoutCooldownHours);
    AddFloat(f, "submissionBeatStressBase", &c.submission.beatStressBase);
    AddFloat(f, "submissionKnockoutStressBase", &c.submission.knockoutStressBase);
    AddFloat(f, "submissionBeatingStressMin", &c.submission.beatingStressMin);
    AddFloat(f, "submissionBeatingStressMax", &c.submission.beatingStressMax);
    AddFloat(f, "submissionBeatingStressLevelMin", &c.submission.beatingStressLevelMin);
    AddFloat(f, "submissionBeatingStressLevelMax", &c.submission.beatingStressLevelMax);
    AddFloat(f, "submissionBeatingStressFadeHours", &c.submission.beatingStressFadeHours);
    AddBool(f, "submissionEventEnabled", &c.submission.eventEnabled);
    AddFloat(f, "submissionApproachSeconds", &c.submission.approachSeconds);
    AddFloat(f, "submissionForgiveSeconds", &c.submission.forgiveSeconds);
    AddFloat(f, "submissionRejectCooldownHours", &c.submission.rejectCooldownHours);

    AddFloat(f, "animalCubMaturity", &c.animals.cubMaturity);
    AddInt(f, "animalWeakLevel", &c.animals.weakLevel);
    AddFloat(f, "animalCubFullness", &c.animals.cubFullness);
    AddFloat(f, "animalBaseFullness", &c.animals.baseFullness);
    AddFloat(f, "animalMaturityWeight", &c.animals.maturityWeight);
    AddInt(f, "animalStrongLevel", &c.animals.strongLevel);
    AddFloat(f, "animalStrongPenalty", &c.animals.strongPenalty);
    AddFloat(f, "animalEdgeDiscount", &c.animals.edgeDiscount);
    AddFloat(f, "animalBigEdgeDiscount", &c.animals.bigEdgeDiscount);
    AddBool(f, "animalOwnedNeedCage", &c.animals.ownedNeedCage);
    AddFloat(f, "animalNutritionPerMeal", &c.animals.nutritionPerMeal);
    AddBool(f, "animalPersonalityMatters", &c.animals.personalityMatters);
    AddInt(f, "animalEdgeLevel", &c.animals.edgeLevel);
    AddInt(f, "animalBigEdgeLevel", &c.animals.bigEdgeLevel);
    AddFloat(f, "animalTemperamentBase", &c.animals.temperamentBase);
    AddFloat(f, "animalTemperamentWeight", &c.animals.temperamentWeight);

    AddString(f, "dialoguePackage", &c.dialogue.package);
    AddString(f, "dialogueRescue", &c.dialogue.rescue);
    AddString(f, "dialogueCaptive", &c.dialogue.captive);
    AddString(f, "dialogueAcceptKey", &c.dialogue.acceptKey);

    AddBool(f, "dead", &c.exclude.dead);
    AddBool(f, "unconscious", &c.exclude.unconscious);
    AddBool(f, "animals", &c.exclude.animals);
    AddBool(f, "sameFaction", &c.exclude.sameFaction);
    AddList(f, "factions", &c.exclude.factions);
    AddList(f, "characterNames", &c.exclude.characterNames);

    return f;
}

void ReadField(const std::string& text, const Field& field)
{
    switch (field.type)
    {
    case Field_Bool:       ReadBool(text, field.key, (bool*)field.target); break;
    case Field_Int:        ReadInt(text, field.key, (int*)field.target); break;
    case Field_Float:      ReadFloat(text, field.key, (float*)field.target); break;
    case Field_String:     ReadString(text, field.key, (std::string*)field.target); break;
    case Field_StringList: ReadStringArray(text, field.key, (std::vector<std::string>*)field.target); break;
    }
}

JsonUpdate WriteField(const Field& field)
{
    switch (field.type)
    {
    case Field_Bool:       return JsonUpdate(field.key, JsonBool(*(bool*)field.target));
    case Field_Int:        return JsonUpdate(field.key, JsonInt(*(int*)field.target));
    case Field_Float:      return JsonUpdate(field.key, JsonFloat(*(float*)field.target));
    case Field_String:     return JsonUpdate(field.key, JsonString(*(std::string*)field.target));
    case Field_StringList: return JsonUpdate(field.key, JsonStringArray(*(std::vector<std::string>*)field.target));
    }
    return JsonUpdate(field.key, "null");
}

// What a brand new config.json starts from. The exclusion keys are read by name
// wherever they sit, but the documented layout nests them, so a file the plugin
// creates itself keeps that layout and the merge fills in the values.
const char* const kFreshConfig =
    "{\n"
    "  \"exclude\": {\n"
    "    \"dead\": true,\n"
    "    \"unconscious\": false,\n"
    "    \"animals\": false,\n"
    "    \"sameFaction\": true,\n"
    "    \"factions\": [],\n"
    "    \"characterNames\": []\n"
    "  }\n"
    "}\n";
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

    const std::vector<Field> fields = ConfigFields(g_config);
    for (size_t i = 0; i < fields.size(); ++i)
    {
        ReadField(text, fields[i]);
    }

    // Clamped after reading rather than rejected: a nonsensical value should
    // pull back to a sane one, not disable the feature or drop the whole file.
    ClampFloat(&g_config.hailFirstHours, kHailFirstHoursMin, kHailFirstHoursMax);
    ClampFloat(&g_config.hailRepeatHours, kHailRepeatHoursMin, kHailRepeatHoursMax);
    ClampInt(&g_config.hailRepeats, kHailRepeatsMin, kHailRepeatsMax);
    g_config.headNoticeSeconds = ClampHeadNoticeSeconds(g_config.headNoticeSeconds);
    g_config.headNoticeInfoSeconds = ClampHeadNoticeInfoSeconds(g_config.headNoticeInfoSeconds);

    // Money tunables. A negative or absurd value would otherwise reach the price
    // arithmetic: a negative minimum or ceiling, a curve that never leaves zero.
    ClampInt(&g_config.prices.catsPerLevel, 0, 1000);
    ClampInt(&g_config.prices.catsCeiling, 0, 1000000);
    ClampInt(&g_config.prices.catsMinimum, 0, 1000000);
    ClampInt(&g_config.prices.catsHardCap, 0, 100000000);
    ClampFloat(&g_config.prices.priceCurveSteepness, 0.01f, 1.0f);
    ClampFloat(&g_config.prices.priceCurveMidpoint, 1.0f, 200.0f);
    ClampFloat(&g_config.prices.fearfulMultiplier, 0.0f, 20.0f);
    ClampFloat(&g_config.prices.braveMultiplier, 0.0f, 20.0f);
    ClampFloat(&g_config.prices.factionWeight, 0.0f, 5.0f);
    ClampFloat(&g_config.prices.talentWeight, 0.0f, 10.0f);
    ClampFloat(&g_config.prices.uniqueMultiplier, 0.0f, 50.0f);
    ClampFloat(&g_config.prices.talentMaxMultiplier, 1.0f, 50.0f);
    ClampFloat(&g_config.prices.premiumMaxMultiplier, 1.0f, 50.0f);

    // The numbers that used to be literals in the rules. A hand-edited value that
    // would break the arithmetic (a negative weight, a table entry that swamps the
    // clamp) is pulled back into a range the rules were written for.
    ClampFloat(&g_config.prices.craftFamilyWeight, 0.0f, 10.0f);
    ClampFloat(&g_config.prices.shadowFamilyWeight, 0.0f, 10.0f);
    // Haggling: a floor above the ask or a negative weight would invert the slider
    // or the odds.
    ClampFloat(&g_config.prices.haggleMinFraction, 0.1f, 1.0f);
    ClampFloat(&g_config.prices.haggleDiscountWeight, 0.0f, 10.0f);
    ClampFloat(&g_config.prices.haggleLeverageWeight, 0.0f, 5.0f);
    ClampFloat(&g_config.prices.haggleOffenceMarkup, 0.0f, 2.0f);
    ClampFloat(&g_config.chance.relationWeight, 0.0f, 2.0f);
    ClampInt(&g_config.chance.alliedBonus, 0, 100);
    ClampInt(&g_config.submission.fieldYieldLoyaltyPenalty, 0, 100);
    // The submission event: a walk-up needs some seconds to happen at all, and
    // neither window nor the cooldown may run for a season.
    ClampFloat(&g_config.submission.approachSeconds, 10.0f, 600.0f);
    ClampFloat(&g_config.submission.forgiveSeconds, 0.0f, 1800.0f);
    ClampFloat(&g_config.submission.rejectCooldownHours, 0.0f, 720.0f);
    for (int i = 0; i < Temperament_Count; ++i)
    {
        ClampInt(&g_config.chance.personality[i], -100, 100);
    }
    ClampFloat(&g_config.relationRefuseGoodNatured, -100.0f, 100.0f);
    ClampFloat(&g_config.relationRefuseAll, -100.0f, 100.0f);
    ClampInt(&g_config.animals.edgeLevel, 1, 500);
    ClampInt(&g_config.animals.bigEdgeLevel, 1, 500);
    ClampFloat(&g_config.animals.temperamentBase, 0.0f, 2.0f);
    ClampFloat(&g_config.animals.temperamentWeight, 0.0f, 2.0f);
    ClampAllyTiers(&g_config.allyTiers);
    ClampTempMembers(&g_config.tempMembers);
    ClampHelpers(&g_config.helpers);
    ClampDuelChallenges(&g_config.duelChallenges);

    g_modDirectory = modDirectory;
    LogInfo("Loaded config from " + path);
}

std::string GetModDirectory()
{
    return g_modDirectory;
}

// Merges the current settings into the file that is there rather than rewriting
// it: values of known keys change in place, keys the file lacks are appended, and
// everything else -- comments, unknown keys, the player's own order and spacing --
// is left alone. The result goes through a temporary file, so a crash cannot leave
// a half-written config.json.
bool SaveConfig(const std::string& modDirectory)
{
    std::string dir = modDirectory.empty() ? g_modDirectory : modDirectory;
    if (dir.empty())
    {
        return false;
    }

    const std::string path = dir + "\\config.json";
    const std::string existing = Slurp(path);

    const std::vector<Field> fields = ConfigFields(g_config);
    std::vector<JsonUpdate> updates;
    for (size_t i = 0; i < fields.size(); ++i)
    {
        updates.push_back(WriteField(fields[i]));
    }

    const bool blank = existing.find_first_not_of(" \t\r\n") == std::string::npos;
    MergeResult merged = MergeJsonKeys(blank ? std::string(kFreshConfig) : existing, updates);
    if (!merged.ok)
    {
        // The file is there but is not one object we can edit safely. Keep what
        // the player wrote next to it and start again from a fresh document,
        // rather than either dropping their text silently or never saving.
        const std::string backup = path + ".bak";
        if (!CopyFileA(path.c_str(), backup.c_str(), FALSE))
        {
            LogError(path + " is not valid JSON and could not be backed up; not overwriting it.");
            return false;
        }
        LogError(path + " is not valid JSON; kept a copy as " + backup + " and wrote a fresh file.");
        merged = MergeJsonKeys(kFreshConfig, updates);
    }

    if (merged.ok && merged.text == existing)
    {
        return true;    // nothing differs, so there is nothing to write
    }

    std::string error;
    if (!merged.ok || !WriteFileAtomically(path, merged.text, &error))
    {
        LogError("Failed to write config to " + path + (error.empty() ? std::string() : ": " + error));
        return false;
    }

    char counts[96] = { 0 };
    _snprintf_s(counts, sizeof(counts), _TRUNCATE, " (%d changed, %d added)",
                merged.replaced, merged.appended);
    LogInfo("Saved config to " + path + counts);
    return true;
}

Config& GetConfig()
{
    return g_config;
}
} // namespace totalrecruit
