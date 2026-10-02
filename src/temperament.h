#pragma once

namespace totalrecruit
{
// The engine's personality enum (PT_*), restated without the engine so the rule
// modules can be compiled and tested on their own. rules.cpp owns the one
// mapping from PT_* to this (ToTemperament); anything the engine reports that
// is not listed here reads as Temperament_Normal, which is what every rule's
// "default" branch already meant.
enum Temperament
{
    Temperament_Normal,
    Temperament_Fearful,
    Temperament_Traitorous,
    Temperament_Dumb,
    Temperament_Smart,
    Temperament_Brave,
    Temperament_Honorable,
    Temperament_Maniacal,
    Temperament_ColdCruel,
    Temperament_WarmKind,
    // Not a temperament: the number of them, for tables indexed by one.
    Temperament_Count
};
} // namespace totalrecruit
