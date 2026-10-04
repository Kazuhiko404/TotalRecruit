#pragma once

// The one level TotalRecruit uses, without the engine.
//
// Play-test (2026-10-04): the recruit popup said "nivel 76" for an NPC TotalSparring's
// duel card showed as "Lv 84". TotalRecruit read Character::getRoughLevel();
// TotalSparring reads CharStats::getOverallSkillLevel_0_100() and shows it rounded
// half away from zero (TotalSparring src/duel/chance_card/chance_card_model.cpp
// RoundLevel). User decision: one level for both mods, TotalSparring's. So every
// level TotalRecruit uses (prices, ally tiers, chances, the 80+ duel gate, the popup's
// "nivel N") is that overall skill level, rounded here exactly the way TotalSparring
// rounds it. The engine read is npc_level.h.

namespace totalrecruit
{
// The overall skill level (0-100) as TotalSparring prints it: rounded half away from
// zero (83.5 -> 84, 84.49 -> 84, -0.5 -> -1). Same formula as TotalSparring's
// RoundLevel; a non-finite value is the caller's to reject (npc_level.cpp does).
int RoundLevel(float level);
} // namespace totalrecruit
