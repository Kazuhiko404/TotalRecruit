#pragma once

#include "archetype.h"
#include "temperament.h"

namespace totalrecruit
{
// The one place the engine's enums meet the plugin's own (temperament.h,
// archetype.h). Anything the engine reports that is not listed reads as the
// default, which is what every rule's default branch already meant.
Temperament ToTemperament(int personality);
Archetype ToArchetype(int npcType);
} // namespace totalrecruit
