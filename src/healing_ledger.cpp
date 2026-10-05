#include "healing_ledger.h"

#include <set>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

namespace totalrecruit
{
namespace
{
typedef std::set<CharKey> Healed;
Healed g_healed;

struct HealedMutex
{
    CRITICAL_SECTION value;
    HealedMutex() { InitializeCriticalSection(&value); }
    ~HealedMutex() { DeleteCriticalSection(&value); }
} g_mutex;

struct Guard
{
    Guard() { EnterCriticalSection(&g_mutex.value); }
    ~Guard() { LeaveCriticalSection(&g_mutex.value); }
private:
    Guard(const Guard&);
    Guard& operator=(const Guard&);
};
} // namespace

bool CountsAsPlayerHealing(bool healerIsPlayers, bool patientIsAnimal, bool patientIsPlayers)
{
    return healerIsPlayers && patientIsAnimal && !patientIsPlayers;
}

bool RecordPlayerHealing(const CharKey& key)
{
    Guard lock;
    return g_healed.insert(key).second;
}

bool HealedByPlayer(const CharKey& key)
{
    Guard lock;
    return g_healed.find(key) != g_healed.end();
}

int SavedPlayerHealing(const std::string& persistedId)
{
    if (persistedId.empty())
    {
        return 0;
    }
    return HealedByPlayer(CharKey::FromId(persistedId)) ? 1 : 0;
}

void RestoreSavedPlayerHealing(const std::string& persistedId, int value)
{
    if (persistedId.empty())
    {
        return;
    }
    const CharKey key = CharKey::FromId(persistedId);
    Guard lock;
    if (value > 0)
    {
        g_healed.insert(key);
        return;
    }
    g_healed.erase(key);
}

void ForgetAllPlayerHealing()
{
    Guard lock;
    g_healed.clear();
}
} // namespace totalrecruit
