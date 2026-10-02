#include "mercy_rules.h"

namespace totalrecruit
{
bool CanBuyTheirLife(Temperament personality)
{
    switch (personality)
    {
    case Temperament_Fearful:
    case Temperament_Traitorous:
    case Temperament_Dumb:
    case Temperament_ColdCruel:
    case Temperament_Maniacal:
        return true;
    default:
        return false;
    }
}

PleaOutcome ResolvePlea(PleaAnswer answer, Temperament personality)
{
    switch (answer)
    {
    case PleaAnswer_Accept:
        return PleaOutcome_Join;
    case PleaAnswer_Ignore:
        return CanBuyTheirLife(personality) ? PleaOutcome_LetGoWithTribute
                                            : PleaOutcome_LetGo;
    default:
        return PleaOutcome_FightOn;
    }
}
} // namespace totalrecruit
