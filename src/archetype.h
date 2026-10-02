#pragma once

namespace totalrecruit
{
// The engine's NPC archetype (OT_*), restated without the engine so the rule
// modules can be compiled and tested on their own. temperament_map.cpp owns the
// one mapping from OT_* to this; anything not listed reads as Archetype_Other,
// which is what every rule's default branch already meant.
enum Archetype
{
    Archetype_Other,
    Archetype_Bandit,
    Archetype_Trader,
    Archetype_Slaver,
    Archetype_Civilian,
    Archetype_Adventurer,
    Archetype_Diplomat,
    Archetype_Military,
    Archetype_LawEnforcement,
    Archetype_Slave
};
} // namespace totalrecruit
