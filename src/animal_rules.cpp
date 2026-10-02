#include "animal_rules.h"

namespace totalrecruit
{
float TamingFullnessNeeded(float maturity, int npcLevel, int playerLevel,
                           const Animals& a)
{
    // A cub or a weakling: a mouthful and it follows you home.
    if (maturity < a.cubMaturity || npcLevel <= a.weakLevel)
    {
        return a.cubFullness;
    }

    float needed = a.baseFullness + maturity * a.maturityWeight;

    if (npcLevel > a.strongLevel)
    {
        needed += a.strongPenalty;
    }

    const int edge = playerLevel - npcLevel;
    if (edge >= a.bigEdgeLevel)    needed -= a.bigEdgeDiscount;
    else if (edge >= a.edgeLevel)  needed -= a.edgeDiscount;

    if (needed < a.cubFullness) needed = a.cubFullness;
    if (needed > 1.0f) needed = 1.0f;
    return needed;
}

float ApplyAnimalTemperament(float needed, float resolve, const Animals& a)
{
    needed *= (a.temperamentBase + resolve * a.temperamentWeight);
    if (needed > 1.0f) needed = 1.0f;
    return needed;
}

int MissingStomachPercent(float needed, float fullness)
{
    return (int)((needed - fullness) * 100.0f + 0.5f);
}
} // namespace totalrecruit
