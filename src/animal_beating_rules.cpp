#include "animal_beating_rules.h"

namespace totalrecruit
{
namespace
{
float Clamp01(float value)
{
    if (value < 0.0f)
    {
        return 0.0f;
    }
    return value > 1.0f ? 1.0f : value;
}
} // namespace

float AnimalBeatingResolve(int npcLevel, const Animals& animals)
{
    const int level = npcLevel > 0 ? npcLevel : 0;
    const float resolve = animals.cageBeatingResolveBase +
                          (float)level * animals.cageBeatingResolvePerLevel;
    return resolve < kAnimalBeatingResolveFloor ? kAnimalBeatingResolveFloor : resolve;
}

float AnimalBeatingDiscount(float beatingStress, int npcLevel, const Animals& animals)
{
    if (!animals.cageBeatingCounts || animals.cageBeatingWeight <= 0.0f)
    {
        return 0.0f;
    }
    const float discount = animals.cageBeatingWeight * Clamp01(beatingStress) /
                           AnimalBeatingResolve(npcLevel, animals);
    if (discount <= 0.0f)
    {
        return 0.0f;
    }
    return discount > kAnimalBeatingMaxDiscount ? kAnimalBeatingMaxDiscount : discount;
}

float ApplyAnimalBeating(float needed, float beatingStress, int npcLevel, const Animals& animals)
{
    return needed * (1.0f - AnimalBeatingDiscount(beatingStress, npcLevel, animals));
}
} // namespace totalrecruit
