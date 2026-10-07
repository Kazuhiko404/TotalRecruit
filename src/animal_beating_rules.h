#pragma once

// PURE: beating stress on an animal in the player's cage (priority-one D).
//
// Golpear and Noquear already leave beating stress on an animal (beating_stress),
// and a beaten loose animal already yields through it (route_animal_yield.cpp).
// A caged animal, though, decided on food alone: the blows did nothing. Now, in
// Balanced, the stress lowers the stomach bar a caged animal has to reach
// (route_animal.cpp DecideCagedAnimal):
//
//   resolve  = max(floor, cageBeatingResolveBase + level * cageBeatingResolvePerLevel)
//   discount = clamp(cageBeatingWeight * stress / resolve, 0, kAnimalBeatingMaxDiscount)
//   bar      = bar * (1 - discount)
//
// An animal has no temperament to brace it the way a person's resolve does
// (beating_stress_rules.h ResolveMultiplier), so its level stands in: a big beast
// shrugs off what breaks a small one. The first meal from the player's hand is
// still required (a beating is not care), and the stomach still has to reach the
// lowered bar. The readout's food bar shows the lowered bar as it is.
//
// No Kenshi / MyGUI include: tested in tests/rules_tests/animal_beating_rules_tests.cpp.

#include "config.h"

namespace totalrecruit
{
// The clamps config.cpp and the Mod Hub use.
const float kAnimalBeatingWeightMax = 1.0f;
const float kAnimalBeatingResolveBaseMin = 0.1f;
const float kAnimalBeatingResolveBaseMax = 10.0f;
const float kAnimalBeatingResolvePerLevelMax = 0.5f;
// The resolve never reads below this (no division by ~0).
const float kAnimalBeatingResolveFloor = 0.1f;
// The bar never drops by more than this: food always matters.
const float kAnimalBeatingMaxDiscount = 0.9f;

float AnimalBeatingResolve(int npcLevel, const Animals& animals);

// The fraction of the bar a beating of `beatingStress` (0..1, clamped) removes;
// 0 when cageBeatingCounts is off.
float AnimalBeatingDiscount(float beatingStress, int npcLevel, const Animals& animals);

// `needed` lowered by AnimalBeatingDiscount.
float ApplyAnimalBeating(float needed, float beatingStress, int npcLevel, const Animals& animals);
} // namespace totalrecruit
