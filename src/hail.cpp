#include "common.h"
#include "hail.h"
#include "rules.h"
#include "config.h"
#include "recruit_core.h"
#include "dialogue_authoring.h"
#include "log.h"

#include <map>

namespace totalrecruit
{
namespace
{
// Sweeping the world is not free, so it happens a few times a minute rather
// than every frame. A prisoner is not going anywhere.
const unsigned int kSweepIntervalMs = 4000;

// Only captives close enough for the player to plausibly hear.
const float kHailRadius = 900.0f;

// Once each. A prisoner who keeps shouting the same line every few seconds
// stops being a signal and becomes noise.
std::map<Character*, bool> g_hailed;

unsigned int g_nextSweep = 0;

bool SafePlayerAnchor(Ogre::Vector3* out)
{
    TR_TRY
    {
        if (ou == NULL || ou->player == NULL)
        {
            return false;
        }

        lektor<Character*>& squad = ou->player->playerCharacters;
        for (unsigned int i = 0; i < squad.size(); ++i)
        {
            Character* member = squad[i];
            if (member != NULL)
            {
                *out = member->getPosition();
                return true;
            }
        }
        return false;
    }
    TR_CATCH { return false; }
}

// lektor is an engine-owned buffer that the caller has to release, so the
// gather and the free are paired in one place.
bool SafeGatherNearby(const Ogre::Vector3& centre, lektor<RootObject*>* out)
{
    TR_TRY
    {
        ou->getCharactersWithinSphere(*out, centre, kHailRadius, 0.0f, 30.0f, 40, 0, NULL);
        return true;
    }
    TR_CATCH { return false; }
}

void SafeFreeList(lektor<RootObject*>* list)
{
    TR_TRY
    {
        if (list->stuff != NULL)
        {
            free(list->stuff);
            list->stuff = NULL;
        }
    }
    TR_CATCH { }
}

bool SafeIsCharacterType(RootObject* object, Character** out)
{
    TR_TRY
    {
        const itemType type = object->getDataType();
        if (type != CHARACTER && type != HUMAN_CHARACTER &&
            type != ANIMAL_CHARACTER && type != WORLDMAP_CHARACTER)
        {
            return false;
        }
        *out = static_cast<Character*>(object);
        return true;
    }
    TR_CATCH { return false; }
}

// Only captives call out. A free NPC shouting "let us talk" as the player walks
// past a town would be noise, and the mod has nothing to offer them anyway.
bool WorthHailing(Character* npc)
{
    if (npc == NULL || !IsInPlayerCage(npc))
    {
        return false;
    }

    if (!IsRecruitable(npc, NULL))
    {
        return false;
    }

    const Verdict verdict = Evaluate(npc);
    return verdict.willJoin;
}

void SweepForHails()
{
    Ogre::Vector3 centre;
    if (!SafePlayerAnchor(&centre))
    {
        return;
    }

    lektor<RootObject*> nearby;
    if (!SafeGatherNearby(centre, &nearby))
    {
        return;
    }

    for (unsigned int i = 0; i < nearby.size(); ++i)
    {
        Character* npc = NULL;
        if (!SafeIsCharacterType(nearby[i], &npc))
        {
            continue;
        }

        if (g_hailed.find(npc) != g_hailed.end())
        {
            continue;
        }

        if (!WorthHailing(npc))
        {
            continue;
        }

        g_hailed[npc] = true;
        SpeakAloud(npc, HailLine(npc));
        LogInfo(CharacterName(npc) + " called out to the player.");
    }

    SafeFreeList(&nearby);
}
} // namespace

bool InstallHailing()
{
    g_hailed.clear();
    g_nextSweep = 0;
    LogInfo("Prisoner hailing installed.");
    return true;
}

void ShutdownHailing()
{
    g_hailed.clear();
}

void HailTick()
{
    if (!GetConfig().hailWhenReady)
    {
        return;
    }

    const unsigned int now = GetTickCount();
    if (now < g_nextSweep)
    {
        return;
    }
    g_nextSweep = now + kSweepIntervalMs;

    SweepForHails();
}
} // namespace totalrecruit
