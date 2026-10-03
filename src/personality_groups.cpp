#include "personality_groups.h"

namespace totalrecruit
{
PersonalityGroup GroupOf(Temperament temperament)
{
    switch (temperament)
    {
    case Temperament_Honorable:
    case Temperament_WarmKind:
        return PersonalityGroup_Good;
    case Temperament_Traitorous:
    case Temperament_Smart:
        return PersonalityGroup_Greedy;
    case Temperament_ColdCruel:
    case Temperament_Maniacal:
        return PersonalityGroup_Bad;
    case Temperament_Normal:
    case Temperament_Dumb:
    case Temperament_Fearful:
    case Temperament_Brave:
    default:
        return PersonalityGroup_Neutral;
    }
}
} // namespace totalrecruit
