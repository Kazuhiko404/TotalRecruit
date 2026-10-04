#include "common.h"
#include "npc_level.h"
#include "level_rules.h"
#include "log.h"

#include <kenshi/CharStats.h>

#include <float.h>

namespace totalrecruit
{
namespace
{
// POD-only frames (C2712): no C++ objects next to the __try.
bool SafeOverallSkillLevel(Character* character, float* out)
{
    TR_TRY
    {
        CharStats* stats = character->getStats();
        if (stats == NULL)
        {
            return false;
        }
        *out = stats->getOverallSkillLevel_0_100();
        return true;
    }
    TR_CATCH { return false; }
}

bool SafeRoughLevel(Character* character, int* out)
{
    TR_TRY { *out = character->getRoughLevel(); return true; }
    TR_CATCH { return false; }
}

bool g_loggedFallback = false;
} // namespace

bool SafeNpcLevel(Character* character, int* out)
{
    if (character == NULL || out == NULL)
    {
        return false;
    }
    float overall = 0.0f;
    if (SafeOverallSkillLevel(character, &overall) && _finite(overall))
    {
        *out = RoundLevel(overall);
        return true;
    }
    if (!g_loggedFallback)
    {
        g_loggedFallback = true;
        LogError("[level] The overall skill level could not be read; using the rough "
                 "level instead (logged once).");
    }
    return SafeRoughLevel(character, out);
}
} // namespace totalrecruit
