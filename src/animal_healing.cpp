#include "common.h"
#include "animal_healing.h"
#include "engine_reads.h"
#include "char_key_reader.h"
#include "healing_ledger.h"
#include "config.h"
#include "log.h"

namespace totalrecruit
{
namespace
{
// Its own POD-only frame: C2712 counts every C++ object in the whole function.
Character* SafePatientOf(MedicalSystem* medical)
{
    TR_TRY { return medical->me; }
    TR_CATCH { return NULL; }
}
} // namespace

void NoteAnimalTreatment(MedicalSystem* medical, Character* healer)
{
    if (medical == NULL || healer == NULL || !GetConfig().enabled)
    {
        return;
    }
    Character* patient = SafePatientOf(medical);
    if (patient == NULL || patient == healer)
    {
        return;
    }

    bool animal = false;
    if (!SafeIsAnimal(patient, &animal) || !animal)
    {
        return;
    }
    // Already marked: one key lookup, no id minted, nothing logged.
    CharKey known;
    if (SafeFindCharKey(patient, &known) && HealedByPlayer(known))
    {
        return;
    }

    bool healerIsPlayers = false;
    bool patientIsPlayers = false;
    if (!SafeIsPlayerCharacter(healer, &healerIsPlayers) ||
        !SafeIsPlayerCharacter(patient, &patientIsPlayers))
    {
        return;
    }
    if (!CountsAsPlayerHealing(healerIsPlayers, animal, patientIsPlayers))
    {
        return;
    }

    // The mark is saved with the animal, so it needs an id (minted here, as a meal does).
    CharKey key;
    if (!SafeCharKey(patient, &key) || !RecordPlayerHealing(key))
    {
        return;
    }
    LogInfo("[heal] " + CharacterName(patient) + " treated by " + CharacterName(healer) +
            ": healed by the player [key " + key.ToString() + "]");
}
} // namespace totalrecruit
