#include "common.h"
#include "rescue.h"
#include "rules.h"
#include "rescue_memory.h"
#include "pacify.h"
#include "engine_reads.h"
#include "dialogue_bridge.h"
#include "config.h"
#include "recruit_core.h"
#include "log.h"
#include "mercy.h"
#include "animal_healing.h"
#include "offer_requests.h"

namespace totalrecruit
{
namespace
{
bool (*g_applyFirstAid_orig)(MedicalSystem*, float, Item*, float, Character*) = NULL;
bool (*g_applyDoctoring_orig)(MedicalSystem*, float, Item*, float, Character*) = NULL;
bool (*g_sendDialogEvent_orig)(Character*, Character*, EventTriggerEnum) = NULL;

bool SafePatient(MedicalSystem* medical, Character** out)
{
    TR_TRY
    {
        *out = medical->me;
        return (*out != NULL);
    }
    TR_CATCH { return false; }
}

bool SafeIsDying(MedicalSystem* medical, bool* out)
{
    TR_TRY { *out = medical->isProbablyDying(); return true; }
    TR_CATCH { return false; }
}

// Treating someone who is genuinely dying is what earns the debt. Ordinary
// patching-up of a squadmate is not a rescue, and neither is bandaging an NPC
// who was only knocked out.
void NoteTreatment(MedicalSystem* medical, Character* healer)
{
    const Config& config = GetConfig();
    if (!config.rescueEnabled || medical == NULL || healer == NULL)
    {
        return;
    }

    if (config.rescuePlayerOnly)
    {
        bool healerIsPlayers = false;
        if (!SafeIsPlayerCharacter(healer, &healerIsPlayers) || !healerIsPlayers)
        {
            return;
        }
    }

    bool dying = false;
    if (!SafeIsDying(medical, &dying) || !dying)
    {
        return;
    }

    Character* patient = NULL;
    if (!SafePatient(medical, &patient))
    {
        return;
    }

    // Someone already in the player's faction cannot be recruited again.
    bool patientIsPlayers = false;
    if (SafeIsPlayerCharacter(patient, &patientIsPlayers) && patientIsPlayers)
    {
        return;
    }

    // Already recorded? rememberCharacter is cheap, but the log is not.
    if (RemembersPlayerAsSaviour(patient))
    {
        return;
    }

    MarkRescuedBy(patient, healer);
}

bool ApplyFirstAid_hook(MedicalSystem* thisptr, float skill, Item* equipment,
                        float frameTime, Character* who)
{
    NoteTreatment(thisptr, who);
    // The same healer, for Cheat's "healed by the player" (animal_healing.h).
    NoteAnimalTreatment(thisptr, who);
    // And for the proactive offer: the patient comes to offer once it is up (offer_requests.h).
    NoteHealForOffer(thisptr, who);
    return g_applyFirstAid_orig != NULL
               ? g_applyFirstAid_orig(thisptr, skill, equipment, frameTime, who)
               : false;
}

bool ApplyDoctoring_hook(MedicalSystem* thisptr, float skill, Item* equipment,
                         float frameTime, Character* who)
{
    NoteTreatment(thisptr, who);
    NoteAnimalTreatment(thisptr, who);
    NoteHealForOffer(thisptr, who);
    return g_applyDoctoring_orig != NULL
               ? g_applyDoctoring_orig(thisptr, skill, equipment, frameTime, who)
               : false;
}

// EV_GET_UP_PEACE is the engine's own "I am back on my feet" event, which is
// when the offer should come -- not the moment the bandage goes on.
bool SendDialogEvent_hook(Character* thisptr, Character* target, EventTriggerEnum what)
{
    const bool original = g_sendDialogEvent_orig != NULL
                              ? g_sendDialogEvent_orig(thisptr, target, what)
                              : false;

    const Config& config = GetConfig();

    // Losing a limb ends the fight for them whatever their personality says,
    // and the engine announces it rather than leaving us to infer it.
    if (config.surrenderEnabled && thisptr != NULL &&
        (what == EV_LOST_ARM || what == EV_LOST_LEG))
    {
        OfferMercy(thisptr);
    }

    if (config.rescueEnabled && what == EV_GET_UP_PEACE && thisptr != NULL)
    {
        // Already one of yours? Then there is nothing to offer. This also stops
        // the offer repeating: the tag stays set for good, but once they have
        // joined they are player characters and fall out here.
        bool alreadyYours = false;
        if (RemembersPlayerAsSaviour(thisptr) &&
            SafeIsPlayerCharacter(thisptr, &alreadyYours) && !alreadyYours)
        {
            // Pacify immediately so they don't swing at the player who just saved them
            Character* leader = NULL;
            if (ou != NULL && ou->player != NULL && ou->player->currentPlatoon != NULL)
            {
                leader = ou->player->currentPlatoon->getSquadLeader();
            }
            SafePacify(thisptr, leader);

            const Verdict verdict = Evaluate(thisptr);
            if (verdict.willJoin && verdict.viaRescue)
            {
                const std::string name = CharacterName(thisptr);

                // Offer through FCS dialogue when one is configured, so the
                // player can decline. Without one the plugin still works, but
                // the choice is gone -- so it announces itself rather than
                // silently changing the player's squad.
                if (!OpenConversation(thisptr, config.dialogue.rescue))
                {
                    // Without an FCS package the plugin's own window asks, the
                    // same one the "Hablar" button opens. Joining outright was
                    // the one path that changed the player's squad without
                    // anyone agreeing to it.
                    AttemptRecruit(thisptr);
                    LogInfo("Rescued " + name + " woke and was offered.");
                }
            }
        }
    }

    return original;
}
} // namespace

bool InstallRescueHooks()
{
    bool ok = true;

    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&MedicalSystem::applyFirstAid),
            ApplyFirstAid_hook, &g_applyFirstAid_orig))
    {
        LogError("Could not hook MedicalSystem::applyFirstAid.");
        ok = false;
    }

    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&MedicalSystem::applyDoctoring),
            ApplyDoctoring_hook, &g_applyDoctoring_orig))
    {
        LogError("Could not hook MedicalSystem::applyDoctoring.");
        ok = false;
    }

    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&Character::sendDialogEvent),
            SendDialogEvent_hook, &g_sendDialogEvent_orig))
    {
        LogError("Could not hook Character::sendDialogEvent.");
        ok = false;
    }

    if (ok)
    {
        LogInfo("Rescue hooks installed.");
    }
    return ok;
}

void ShutdownRescueHooks()
{
}

void RescueTick()
{
    // Completes a recruit the player accepted in dialogue. This runs a frame
    // later than the choice because the engine keeps both parties locked until
    // the conversation closes, and mutating them inside the hook risks leaving
    // the character in a broken state.
    int cats = 0;
    Character* pending = ConsumePendingRecruit(&cats);
    if (pending == NULL)
    {
        return;
    }

    const std::string name = CharacterName(pending);

    // The money is collected here, not when the offer was made: declining in
    // the conversation has to cost the player nothing.
    if (!SettlePrice(pending, cats, name))
    {
        return;
    }

    std::string message;
    if (Recruit(pending, &message))
    {
        LogInfo("Recruited " + message + " after dialogue.");
    }
    else
    {
        LogInfo("Post-dialogue recruit failed: " + message);
        RefundPrice(pending, cats, name);
    }
}
} // namespace totalrecruit
