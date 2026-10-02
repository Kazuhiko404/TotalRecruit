#include "common.h"
#include "temperament_map.h"

namespace totalrecruit
{
// Anything not listed reads as Normal, which is what every rule's default branch
// meant.
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

// Anything not listed reads as Other, which is the default branch of every rule
// that looks at the archetype.
Archetype ToArchetype(int npcType)
{
    switch (npcType)
    {
    case OT_BANDIT:          return Archetype_Bandit;
    case OT_TRADER:          return Archetype_Trader;
    case OT_SLAVER:          return Archetype_Slaver;
    case OT_CIVILIAN:        return Archetype_Civilian;
    case OT_ADVENTURER:      return Archetype_Adventurer;
    case OT_DIPLOMAT:        return Archetype_Diplomat;
    case OT_MILITARY:        return Archetype_Military;
    case OT_LAW_ENFORCEMENT: return Archetype_LawEnforcement;
    case OT_SLAVE:           return Archetype_Slave;
    default:                 return Archetype_Other;
    }
}
} // namespace totalrecruit
