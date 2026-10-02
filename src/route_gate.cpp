#include "route_decisions.h"

namespace totalrecruit
{
Verdict BaseVerdict(const CoreFacts& core)
{
    Verdict v;
    v.isAnimal = core.animal;
    v.wantsFood = core.starving;
    return v;
}

// The questions that cannot be asked, in the order they used to be checked.
bool DecideGate(const CoreFacts& core, const Config& config, Verdict* out)
{
    Verdict v;
    v.isAnimal = core.animal;

    if (core.animal && !config.animalsEnabled)
    {
        v.reasonKey = Reason_AnimalsDisabled;
        *out = v;
        return true;
    }

    if (!core.traitsReadable)
    {
        // Without personality and archetype there is nothing to price against.
        // Refusing outright would break the very NPCs this mod exists to cover,
        // so fall back to the plain recruit instead.
        v.willJoin = true;
        v.reasonKey = Reason_UnreadableTraits;
        *out = v;
        return true;
    }

    // From here on the offer window can read whether they are starving.
    v.wantsFood = core.starving;

    // PS_PLAYING_DEAD is a real prone state, so "unconscious" alone would let the
    // player farm recruits off NPCs that are only pretending.
    if (core.pretending)
    {
        v.reasonKey = Reason_Pretending;
        *out = v;
        return true;
    }

    if (core.corpse)
    {
        v.reasonKey = Reason_Dead;
        *out = v;
        return true;
    }

    return false;
}
} // namespace totalrecruit
