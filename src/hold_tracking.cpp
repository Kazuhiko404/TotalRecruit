#include "common.h"
#include "hold_tracking.h"
#include "hold_clock.h"
#include "engine_reads.h"
#include "char_key_reader.h"
#include "config.h"
#include "log.h"

namespace totalrecruit
{
namespace
{
HoldClock g_holdClock;
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

    g_holdClock.Observe(key, true, now);
    return g_holdClock.HoursHeld(key, now);
}

bool NoteCageState(Character* character)
{
    const bool caged = IsInPlayerCage(character);

    double now = 0.0;
    CharKey key;
    // Looks at everyone in range, so it must not mint an id for each bystander:
    // a character that is not caged and has no id cannot have a running clock.
    const bool haveKey = caged ? SafeCharKey(character, &key)
                               : SafeFindCharKey(character, &key);
    if (SafeGameHours(&now) && haveKey)
    {
        const bool wasHeld = g_holdClock.IsTracked(key);
        g_holdClock.Observe(key, caged, now);
        if (GetConfig().probeOnClick && wasHeld != caged)
        {
            LogInfo(CharacterName(character) +
                    (caged ? " [hold] clock started: seen in your cage"
                           : " [hold] clock dropped: no longer in your cage"));
        }
    }
    return caged;
}

void ResetHoldClock()
{
    g_holdClock.Clear();
}
} // namespace totalrecruit
