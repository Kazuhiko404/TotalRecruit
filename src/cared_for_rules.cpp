#include "cared_for_rules.h"
#include "chance_rules.h"

namespace totalrecruit
{
int CaredForBonusFromSeed(unsigned int seed, int minPct, int maxPct)
{
    const int lo = minPct > 0 ? minPct : 0;
    int hi = maxPct > 0 ? maxPct : 0;
    if (hi < lo)
    {
        hi = lo;
    }
    const unsigned int span = (unsigned int)(hi - lo) + 1u;
    return lo + (int)(seed % span);
}

int CaredForBonus(bool caredFor, bool haveKey, const CharKey& key, int dayIndex,
                  const Submission& sub)
{
    if (!caredFor || !haveKey)
    {
        return 0;
    }
    return CaredForBonusFromSeed(DailySeed(key, dayIndex, kCaredForBonusSalt),
                                 sub.caredForBonusMin, sub.caredForBonusMax);
}
} // namespace totalrecruit
