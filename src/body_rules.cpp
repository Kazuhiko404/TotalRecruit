#include "body_rules.h"
#include "chance_rules.h"

namespace totalrecruit
{
bool IsWounded(bool unconscious, bool dying, float health, float woundedThreshold)
{
    return unconscious ? dying : (health <= woundedThreshold);
}

bool IsStarving(bool engineSaysStarving, float rawHunger, float hungerThreshold)
{
    if (engineSaysStarving)
    {
        return true;
    }
    return hungerThreshold > 0.0f && HungerToFullness01(rawHunger) <= hungerThreshold;
}

bool IsFullyCaredFor(bool wounded, bool starving, float health)
{
    return !wounded && !starving && health >= 1.0f;
}

float RestoredFraction(bool unconscious, float bloodFraction, float health)
{
    return unconscious ? bloodFraction : (health > 1.0f ? 1.0f : health);
}
} // namespace totalrecruit
