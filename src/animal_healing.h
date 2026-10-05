#pragma once

// "Healed by the player", engine side (animal-duel-submission D4).
//
// The rescue hooks on MedicalSystem::applyFirstAid and applyDoctoring (rescue.cpp)
// receive the character doing the treatment. Each call is handed here as well: when
// one of the player's characters treats an animal that is not the player's
// (healing_ledger.h CountsAsPlayerHealing), the animal is marked healed by the player,
// once, and the mark is saved with it. Cheat reads the mark (route_cheat.cpp): a free
// animal the player healed joins like a fed one.
//
// Runs on every frame of a treatment, so it is cheap once marked: an animal that
// already carries the mark costs one key lookup and nothing else. No log line after
// the first.

class MedicalSystem;
class Character;

namespace totalrecruit
{
void NoteAnimalTreatment(MedicalSystem* medical, Character* healer);
} // namespace totalrecruit
