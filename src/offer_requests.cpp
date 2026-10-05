#include "common.h"
#include "offer_requests.h"
#include "approach_driver.h"
#include "submission_rules.h"
#include "npc_motion.h"
#include "engine_reads.h"
#include "char_key_reader.h"
#include "config.h"
#include "log.h"

#include <cstdio>

namespace totalrecruit
{
namespace
{
// Real seconds that stop while the game is paused (approach_driver.h EventClock).
EventClock g_clock;
OfferRequestQueue g_queue;

void Log(const std::string& line)
{
    LogInfo("[offer] " + line);
}

// Its own POD-only frame: C2712 counts every C++ object in the whole function.
Character* SafePatientOf(MedicalSystem* medical)
{
    TR_TRY { return medical->me; }
    TR_CATCH { return NULL; }
}

bool IsPlayers(Character* character, bool* out)
{
    *out = false;
    return character != NULL && SafeIsPlayerCharacter(character, out);
}
} // namespace

void RequestOffer(Character* npc, OfferTrigger trigger, OfferRequestVerdict verdict)
{
    CharKey key;
    if (npc == NULL || !SafeCharKey(npc, &key))
    {
        return;
    }
    const OfferRequestResult result = g_queue.Request(key, trigger, verdict, g_clock.Now());
    // A treatment asks every frame: only a new request, or a newly settled one, is news.
    if (result == OfferRequest_Queued || (result == OfferRequest_Refreshed && verdict != OfferVerdict_Roll))
    {
        Log("request " + std::string(OfferRequestResultName(result)) + ": " + CharacterName(npc) +
            " (" + OfferTriggerName(trigger) + ", " + OfferVerdictName(verdict) + ") [key " +
            key.ToString() + "]");
    }
}

void NoteHealForOffer(MedicalSystem* medical, Character* healer)
{
    const Config& config = GetConfig();
    if (!config.enabled || !config.offer.onHeal || medical == NULL || healer == NULL)
    {
        return;
    }
    Character* patient = SafePatientOf(medical);
    if (patient == NULL || patient == healer)
    {
        return;
    }
    bool healerIsPlayers = false;
    bool patientIsPlayers = false;
    if (!IsPlayers(healer, &healerIsPlayers) || !healerIsPlayers ||
        !IsPlayers(patient, &patientIsPlayers) || patientIsPlayers)
    {
        return;
    }
    RequestOffer(patient, OfferTrigger_Heal, OfferVerdict_Roll);
}

bool HasRequestedOffers()
{
    return g_queue.HasPending();
}

bool FindRequestedOffer(Character* npc, OfferRequest* out)
{
    CharKey key;
    return npc != NULL && g_queue.HasPending() && SafeFindCharKey(npc, &key) &&
           g_queue.FindPending(key, out);
}

void ConsumeRequestedOffer(Character* npc, const char* why)
{
    CharKey key;
    if (npc == NULL || !g_queue.HasPending() || !SafeFindCharKey(npc, &key))
    {
        return;
    }
    if (g_queue.Consume(key, g_clock.Now()))
    {
        Log("consume: " + CharacterName(npc) + " (" + why + ") [key " + key.ToString() + "]");
    }
}

bool WithinOfferReach(Character* npc)
{
    Character* leader = NULL;
    float meters = -1.0f;
    return npc != NULL && SafePlayerLeader(&leader) && leader != NULL &&
           NpcMotion::DistanceMeters(npc, leader, &meters) && meters >= 0.0f &&
           meters <= kSubmissionAbortMeters;
}

void OfferRequestsTick()
{
    g_clock.Advance();
    const int expired = g_queue.Expire(g_clock.Now());
    if (expired > 0)
    {
        char line[96] = { 0 };
        _snprintf_s(line, sizeof(line), _TRUNCATE,
                    "%d request(s) expired unserved after %.0f s (never up or never in reach)",
                    expired, kOfferRequestTtlSec);
        Log(line);
    }
}

void ForgetOfferRequests()
{
    g_queue.Clear();
    g_clock.Reset();
}
} // namespace totalrecruit
