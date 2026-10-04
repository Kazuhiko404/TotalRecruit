#include "common.h"
#include "beating_stress.h"
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

void LogGain(Character* target, BeatingBlow blow, const GainFacts& facts, float gain, float bar)
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
    LogInfo("[stress] " + CharacterName(target) + line);
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
    const Submission& sub = GetConfig().submission;
    const GainFacts facts = ReadGainFacts(target);
    const float gain = BeatingStressGain(blow, facts.playerLevel, facts.npcLevel,
                                         facts.temperament, sub);
    float bar = 0.0f;
    {
        LedgerGuard lock;
        bar = g_ledger.Add(key, gain, now, sub.beatingStressFadeHours);
    }
    LogGain(target, blow, facts, gain, bar);
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

void ForgetAllBeatingStress()
{
    LedgerGuard lock;
    g_ledger.Clear();
}
} // namespace totalrecruit
