#include "common.h"
#include "beating_stress.h"
#include "beating_daily_rules.h"
#include "char_key_reader.h"
#include "config.h"
#include "engine_reads.h"
#include "log.h"
#include "chance_rules.h"
#include "npc_level.h"

namespace totalrecruit
{
namespace
{
// The bars. The serialise hook reads them from wherever the engine saves, so the
// ledger sits behind a lock (the defeats.cpp pattern).
BeatingStressLedger g_ledger;
// Balanced only: each prisoner's beatings of the current game day, for the daily
// diminishing and cap (beating_daily_rules.h). Behind the same lock.
BeatingDayLedger g_days;

struct LedgerMutex
{
    CRITICAL_SECTION value;
    LedgerMutex() { InitializeCriticalSection(&value); }
    ~LedgerMutex() { DeleteCriticalSection(&value); }
} g_mutex;

struct LedgerGuard
{
    LedgerGuard() { EnterCriticalSection(&g_mutex.value); }
    ~LedgerGuard() { LeaveCriticalSection(&g_mutex.value); }
private:
    LedgerGuard(const LedgerGuard&);
    LedgerGuard& operator=(const LedgerGuard&);
};

// The levels and the temperament the gain is weighed by. Unreadable levels read
// as 1 (an even match) and an unreadable temperament as normal.
struct GainFacts
{
    int playerLevel;
    int npcLevel;
    Temperament temperament;
};

GainFacts ReadGainFacts(Character* target)
{
    GainFacts facts;
    facts.playerLevel = 1;
    facts.npcLevel = 1;
    facts.temperament = Temperament_Normal;
    SafeNpcLevel(target, &facts.npcLevel);
    // The player's level is the squad leader's: the one JoinChance compares.
    Character* leader = NULL;
    if (SafePlayerLeader(&leader) && leader != NULL)
    {
        SafeNpcLevel(leader, &facts.playerLevel);
    }
    SafeTemperament(target, &facts.temperament);
    return facts;
}

// `day` is the Balanced day record after this blow (dayIndex -1 in Cheat: no
// daily limits, no day part in the line).
void LogGain(Character* target, BeatingBlow blow, const GainFacts& facts, float gain, float bar,
             const BeatingDay& day)
{
    if (!GetConfig().probeOnClick)
    {
        return;
    }
    char line[160];
    _snprintf_s(line, sizeof(line), _TRUNCATE,
                " beating stress +%d%% (%s, player %d vs npc %d, resolve %.2f) -> %d%%",
                (int)(gain * 100.0f + 0.5f), blow == BeatingBlow_Knockout ? "knockout" : "blow",
                facts.playerLevel, facts.npcLevel,
                ResolveMultiplier(facts.temperament, GetConfig().submission),
                (int)(bar * 100.0f + 0.5f));
    std::string text = line;
    if (day.dayIndex >= 0)
    {
        const Submission& sub = GetConfig().submission;
        char dayPart[128];
        _snprintf_s(dayPart, sizeof(dayPart), _TRUNCATE,
                    " [day %d blow %d, x%.2f, today %d%% of cap %d%%]",
                    day.dayIndex, day.blows,
                    BeatingDiminishMultiplier(day.blows - 1, sub.beatingDiminishFactor),
                    (int)(day.gained * 100.0f + 0.5f),
                    (int)(sub.beatingDailyStressCap * 100.0f + 0.5f));
        text += dayPart;
    }
    LogInfo("[stress] " + CharacterName(target) + text);
}
} // namespace

float AddBeatingStress(Character* target, BeatingBlow blow)
{
    if (target == NULL)
    {
        return 0.0f;
    }
    CharKey key;
    double now = 0.0;
    if (!SafeCharKey(target, &key) || !SafeGameHours(&now))
    {
        LogError("[stress] " + CharacterName(target) +
                 ": beating stress not added (key or game clock unreadable)");
        return 0.0f;
    }
    const Config& config = GetConfig();
    const Submission& sub = config.submission;
    const GainFacts facts = ReadGainFacts(target);
    // Balanced weighs the blow by the prisoner's game day so far (diminishing and
    // daily cap, beating_daily_rules.h); Cheat keeps the plain per-blow gain.
    const bool daily = config.preset == Preset_Balanced;
    float gain = 0.0f;
    float bar = 0.0f;
    BeatingDay day;
    {
        LedgerGuard lock;
        if (daily)
        {
            const BeatingDay today = g_days.Today(key, DayIndexFromHours(now));
            gain = DailyBeatingGain(blow, facts.playerLevel, facts.npcLevel,
                                    facts.temperament, today, sub);
            day = RecordBeating(today, gain);
            g_days.Record(key, day);
        }
        else
        {
            gain = BeatingStressGain(blow, facts.playerLevel, facts.npcLevel,
                                     facts.temperament, sub);
        }
        bar = g_ledger.Add(key, gain, now, sub.beatingStressFadeHours);
    }
    LogGain(target, blow, facts, gain, bar, day);
    return gain;
}

float BeatingStressNow(Character* character)
{
    CharKey key;
    double now = 0.0;
    if (character == NULL || !SafeFindCharKey(character, &key) || !SafeGameHours(&now))
    {
        return 0.0f;
    }
    LedgerGuard lock;
    return g_ledger.Get(key, now, GetConfig().submission.beatingStressFadeHours);
}

int SavedBeatingStress(const std::string& persistedId)
{
    if (persistedId.empty())
    {
        return 0;
    }
    LedgerGuard lock;
    return g_ledger.SavedValue(CharKey::FromId(persistedId));
}

void RestoreSavedBeatingStress(const std::string& persistedId, int value)
{
    if (persistedId.empty())
    {
        return;
    }
    LedgerGuard lock;
    g_ledger.RestoreValue(CharKey::FromId(persistedId), value);
}

int SavedBeatingStressHour(const std::string& persistedId)
{
    if (persistedId.empty())
    {
        return 0;
    }
    LedgerGuard lock;
    return g_ledger.SavedHourTenths(CharKey::FromId(persistedId));
}

void RestoreSavedBeatingStressHour(const std::string& persistedId, int tenths)
{
    if (persistedId.empty())
    {
        return;
    }
    LedgerGuard lock;
    g_ledger.RestoreHourTenths(CharKey::FromId(persistedId), tenths);
}

int SavedBeatingDay(const std::string& persistedId)
{
    if (persistedId.empty())
    {
        return 0;
    }
    LedgerGuard lock;
    return g_days.SavedDay(CharKey::FromId(persistedId));
}

void RestoreSavedBeatingDay(const std::string& persistedId, int value)
{
    if (persistedId.empty())
    {
        return;
    }
    LedgerGuard lock;
    g_days.RestoreDay(CharKey::FromId(persistedId), value);
}

int SavedBeatingDayBlows(const std::string& persistedId)
{
    if (persistedId.empty())
    {
        return 0;
    }
    LedgerGuard lock;
    return g_days.SavedBlows(CharKey::FromId(persistedId));
}

void RestoreSavedBeatingDayBlows(const std::string& persistedId, int value)
{
    if (persistedId.empty())
    {
        return;
    }
    LedgerGuard lock;
    g_days.RestoreBlows(CharKey::FromId(persistedId), value);
}

int SavedBeatingDayGained(const std::string& persistedId)
{
    if (persistedId.empty())
    {
        return 0;
    }
    LedgerGuard lock;
    return g_days.SavedGained(CharKey::FromId(persistedId));
}

void RestoreSavedBeatingDayGained(const std::string& persistedId, int value)
{
    if (persistedId.empty())
    {
        return;
    }
    LedgerGuard lock;
    g_days.RestoreGained(CharKey::FromId(persistedId), value);
}

void ForgetAllBeatingStress()
{
    LedgerGuard lock;
    g_ledger.Clear();
    g_days.Clear();
}
} // namespace totalrecruit
