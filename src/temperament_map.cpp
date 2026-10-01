#include "common.h"
#include "rules.h"

namespace totalrecruit
{
// The one place the engine's personality enum meets the plugin's own. Anything
// not listed reads as Normal, which is what every rule's default branch meant.
Temperament ToTemperament(int personality)
{
    switch (personality)
    {
    case PT_FEARFUL:    return Temperament_Fearful;
    case PT_TRAITOROUS: return Temperament_Traitorous;
    case PT_DUMB:       return Temperament_Dumb;
    case PT_SMART:      return Temperament_Smart;
    case PT_BRAVE:      return Temperament_Brave;
    case PT_HONORABLE:  return Temperament_Honorable;
    case PT_MANIACAL:   return Temperament_Maniacal;
    case PT_COLD_CRUEL: return Temperament_ColdCruel;
    case PT_WARM_KIND:  return Temperament_WarmKind;
    default:            return Temperament_Normal;
    }
}
} // namespace totalrecruit
