#include "common.h"
#include "hold_tracking.h"
#include "hold_clock.h"
#include "engine_reads.h"
#include "char_key_reader.h"
#include "config.h"
#include "route_decisions.h"
#include "log.h"

namespace totalrecruit
{
namespace
{
// The clocks. The serialise hook reads them from wherever the engine saves, so the
// clock sits behind a lock (the beating_stress.cpp pattern).
HoldClock g_holdClock;

struct HoldMutex
{
    CRITICAL_SECTION value;
    HoldMutex() { InitializeCriticalSection(&value); }
    ~HoldMutex() { DeleteCriticalSection(&value); }
} g_mutex;

struct HoldGuard
{
    HoldGuard() { EnterCriticalSection(&g_mutex.value); }
    ~HoldGuard() { LeaveCriticalSection(&g_mutex.value); }
private:
    HoldGuard(const HoldGuard&);
    HoldGuard& operator=(const HoldGuard&);
};
} // namespace

float HoursHeld(Character* character)
{
    double now = 0.0;
    if (!SafeGameHours(&now))
    {
        return 0.0f;
    }

    CharKey key;
    if (!SafeCharKey(character, &key))
    {
        return 0.0f;
    }

    HoldGuard lock;
    g_holdClock.Observe(key, true, now);
    return g_holdClock.HoursHeld(key, now);
}

bool PeekHoursHeld(Character* character, float* out)
{
    double now = 0.0;
    CharKey key;
    if (!SafeGameHours(&now) || !SafeFindCharKey(character, &key))
    {
        return false;
    }
    HoldGuard lock;
    if (!g_holdClock.IsTracked(key))
    {
        return false;
    }
    *out = g_holdClock.HoursHeld(key, now);
    return true;
}

bool PeekCountedHoursHeld(Character* character, float* out)
{
    const Config& config = GetConfig();
    const bool mine = IsInPlayerCage(character);
    bool any = false;
    if (!mine && HoldClockCountsAnyCage(config))
    {
        bool inPrison = false;
        any = SafeInPrison(character, &inPrison) && inPrison;
    }
    return HoldClockCounts(mine, any, config) && PeekHoursHeld(character, out);
}

bool NoteCageState(Character* character)
{
    const bool caged = IsInPlayerCage(character);

    // cheat-any-cage: in Cheat with cheatAnyCage on, any cage, shackles or kidnap
    // runs the clock too, so the minute is already counting when the player talks.
    // The return value stays "in the player's cage" (hail.cpp's captive sightings).
    const Config& config = GetConfig();
    bool foreign = false;
    if (!caged && HoldClockCountsAnyCage(config))
    {
        bool inPrison = false;
        foreign = SafeInPrison(character, &inPrison) && inPrison;
    }
    const bool held = caged || foreign;

    double now = 0.0;
    CharKey key;
    // Looks at everyone in range, so it must not mint an id for each bystander:
    // a character that is not held and has no id cannot have a running clock.
    const bool haveKey = held ? SafeCharKey(character, &key)
                              : SafeFindCharKey(character, &key);
    if (SafeGameHours(&now) && haveKey)
    {
        bool wasHeld = false;
        {
            HoldGuard lock;
            wasHeld = g_holdClock.IsTracked(key);
            g_holdClock.Observe(key, held, now);
        }
        if (config.probeOnClick && wasHeld != held)
        {
            LogInfo(CharacterName(character) +
                    (caged     ? " [hold] clock started: seen in your cage"
                     : foreign ? " [hold] clock started: seen in a cage"
                     : HoldClockCountsAnyCage(config)
                               ? " [hold] clock dropped: no longer in a cage"
                               : " [hold] clock dropped: no longer in your cage"));
        }
    }
    return caged;
}

int SavedHoldStart(const std::string& persistedId)
{
    if (persistedId.empty())
    {
        return 0;
    }
    HoldGuard lock;
    return g_holdClock.SavedStartTenths(CharKey::FromId(persistedId));
}

void RestoreSavedHoldStart(const std::string& persistedId, int tenths)
{
    if (persistedId.empty())
    {
        return;
    }
    HoldGuard lock;
    g_holdClock.RestoreStartTenths(CharKey::FromId(persistedId), tenths);
}

void ResetHoldClock()
{
    HoldGuard lock;
    g_holdClock.Clear();
}
} // namespace totalrecruit
