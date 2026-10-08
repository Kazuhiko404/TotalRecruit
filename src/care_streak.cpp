#include "common.h"
#include "care_streak.h"
#include "care_streak_rules.h"
#include "character_facts.h"
#include "char_key_reader.h"
#include "chance_rules.h"
#include "config.h"
#include "engine_reads.h"
#include "log.h"

#include <cstdio>

namespace totalrecruit
{
namespace
{
// The records. The serialise hook reads them from wherever the engine saves, so the
// ledger sits behind a lock (the beating_stress.cpp pattern).
CareStreakLedger g_ledger;

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

// Records one sighting; returns the streak before and after it, for the log.
void Observe(const CharKey& key, int dayIndex, bool caredFor, int* before, int* after)
{
    LedgerGuard lock;
    *before = g_ledger.StreakDays(key, dayIndex);
    g_ledger.Observe(key, dayIndex, caredFor);
    *after = g_ledger.StreakDays(key, dayIndex);
}

CharKey KeyOf(const std::string& persistedId)
{
    return CharKey::FromId(persistedId);
}
} // namespace

void NoteCareState(Character* character)
{
    const Config& config = GetConfig();
    if (config.preset != Preset_Balanced)
    {
        return;
    }

    double now = 0.0;
    CharKey key;
    bool caredFor = false;
    if (!SafeGameHours(&now) || !SafeCharKey(character, &key) ||
        !ReadFullyCaredFor(character, &caredFor))
    {
        return;
    }

    const int day = DayIndexFromHours(now);
    int before = 0;
    int after = 0;
    Observe(key, day, caredFor, &before, &after);
    if (config.probeOnClick && before != after)
    {
        char buf[128] = { 0 };
        _snprintf_s(buf, sizeof(buf), _TRUNCATE, " [care] streak %d -> %d days (day %d, %s)",
                    before, after, day, caredFor ? "cared for" : "not cared for: reset");
        LogInfo(CharacterName(character) + buf);
    }
}

int CareStreakDaysFor(const CharKey& key, int dayIndex, bool observe, bool caredFor)
{
    if (observe)
    {
        int before = 0;
        int after = 0;
        Observe(key, dayIndex, caredFor, &before, &after);
        return after;
    }
    LedgerGuard lock;
    return g_ledger.StreakDays(key, dayIndex);
}

int SavedCareStreakDay(const std::string& persistedId)
{
    if (persistedId.empty())
    {
        return 0;
    }
    LedgerGuard lock;
    return g_ledger.SavedDay(KeyOf(persistedId));
}

void RestoreSavedCareStreakDay(const std::string& persistedId, int value)
{
    if (persistedId.empty())
    {
        return;
    }
    LedgerGuard lock;
    g_ledger.RestoreDay(KeyOf(persistedId), value);
}

int SavedCareStreakDays(const std::string& persistedId)
{
    if (persistedId.empty())
    {
        return 0;
    }
    LedgerGuard lock;
    return g_ledger.SavedStreak(KeyOf(persistedId));
}

void RestoreSavedCareStreakDays(const std::string& persistedId, int value)
{
    if (persistedId.empty())
    {
        return;
    }
    LedgerGuard lock;
    g_ledger.RestoreStreak(KeyOf(persistedId), value);
}

int SavedCareStreakKept(const std::string& persistedId)
{
    if (persistedId.empty())
    {
        return 0;
    }
    LedgerGuard lock;
    return g_ledger.SavedKept(KeyOf(persistedId));
}

void RestoreSavedCareStreakKept(const std::string& persistedId, int value)
{
    if (persistedId.empty())
    {
        return;
    }
    LedgerGuard lock;
    g_ledger.RestoreKept(KeyOf(persistedId), value);
}

void ForgetAllCareStreaks()
{
    LedgerGuard lock;
    g_ledger.Clear();
}
} // namespace totalrecruit
