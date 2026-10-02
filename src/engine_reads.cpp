#include "engine_reads.h"

namespace totalrecruit
{
namespace
{
// The cage is resolved as RootObjectBase rather than Building: getFaction lives
// on the base, and including Building.h collides with Platoon.h over
// BuildingDesignation, which both define identically and neither guards.
bool SafeCageFaction(Character* character, Faction** out)
{
    TR_TRY
    {
        RootObjectBase* cage = character->inWhat.getRootObjectBase();
        if (cage == NULL)
        {
            return false;
        }
        *out = cage->getFaction();
        return (*out != NULL);
    }
    TR_CATCH { return false; }
}

// isMySlave / isMyFactionsSlave answer "does this belong to me" for a captive
// with no cage object to inspect.
bool SafeIsMyPrisoner(Character* leader, Character* captive, bool* out)
{
    TR_TRY
    {
        *out = leader->isMySlave(captive) || leader->isMyFactionsSlave(captive);
        return true;
    }
    TR_CATCH { return false; }
}

bool SafeIsProbablyDying(Character* character, bool* out)
{
    TR_TRY
    {
        MedicalSystem* medical = character->getMedical();
        if (medical == NULL)
        {
            return false;
        }
        *out = medical->isProbablyDying();
        return true;
    }
    TR_CATCH { return false; }
}

// getName returns std::string by value, so the call and the copy out of it
// both live in a helper that the guard only jumps through.
void CopyName(Character* character, char* buffer, size_t size)
{
    strncpy_s(buffer, size, character->getName().c_str(), _TRUNCATE);
}

bool SafeCopyName(Character* character, char* buffer, size_t size)
{
    TR_TRY
    {
        CopyName(character, buffer, size);
        return true;
    }
    TR_CATCH { return false; }
}
} // namespace

// inSomething only reads IN_PRISON while the character is actually inside a
// cage object. A prisoner you carried in and dropped, or one wearing shackles,
// reads IN_NOTHING -- so captivity is the union of the three signals the engine
// offers rather than just that one.
bool SafeInPrison(Character* character, bool* out)
{
    TR_TRY
    {
        *out = (character->inSomething == IN_PRISON) ||
               character->isChainedMode() ||
               character->isKidnapped();
        return true;
    }
    TR_CATCH { return false; }
}

bool SafePlayerFaction(Faction** out)
{
    TR_TRY
    {
        if (ou == NULL || ou->player == NULL)
        {
            return false;
        }
        *out = ou->player->participant;
        return (*out != NULL);
    }
    TR_CATCH { return false; }
}

// An active platoon is required, not just a current one: getSquadLeader reads
// through it, and a copy of this that skipped the check could be asked while the
// squad was still being built.
bool SafePlayerLeader(Character** out)
{
    TR_TRY
    {
        if (ou == NULL || ou->player == NULL)
        {
            return false;
        }
        Platoon* platoon = ou->player->currentPlatoon;
        if (platoon == NULL || platoon->activePlatoon == NULL)
        {
            return false;
        }
        *out = platoon->getSquadLeader();
        return (*out != NULL);
    }
    TR_CATCH { return false; }
}

bool SafeGameHours(double* out)
{
    TR_TRY
    {
        if (ou == NULL)
        {
            return false;
        }
        *out = ou->getTimeStamp_inGameHours().getTotalHours();
        return true;
    }
    TR_CATCH { return false; }
}

bool SafeIsPlayerCharacter(Character* character, bool* out)
{
    TR_TRY { *out = character->isPlayerCharacter(); return true; }
    TR_CATCH { return false; }
}

bool SafeIsAnimal(Character* character, bool* out)
{
    TR_TRY { *out = (character->isAnimal() != NULL); return true; }
    TR_CATCH { return false; }
}

bool SafeIsDead(Character* character, bool* out)
{
    TR_TRY { *out = character->isDead(); return true; }
    TR_CATCH { return false; }
}

bool SafeIsDown(Character* character, bool* out)
{
    TR_TRY { *out = character->isDown(); return true; }
    TR_CATCH { return false; }
}

bool SafeMemoryTag(Character* character, Character* about,
                   CharacterPerceptionTags_LongTerm tag, bool* out)
{
    TR_TRY
    {
        *out = character->getCharacterMemoryTag(about, tag);
        return true;
    }
    TR_CATCH { return false; }
}

bool SafeRememberTag(Character* character, Character* about,
                     CharacterPerceptionTags_LongTerm tag)
{
    TR_TRY
    {
        character->rememberCharacter(about, tag);
        return true;
    }
    TR_CATCH { return false; }
}

bool SafeIsPretending(Character* character, bool* out)
{
    TR_TRY
    {
        // PS_PLAYING_DEAD is the engine's own answer, so ask for it. The old test
        // -- down but not unconscious -- also caught anyone whose legs are
        // crippled: they cannot stand, so isDown() is true, and they are wide
        // awake, so the negation is true too. A one-legged NPC standing there
        // talking was being called a faker.
        *out = (character->getProneState() == PS_PLAYING_DEAD);
        return true;
    }
    TR_CATCH { return false; }
}

bool IsGenuinelyDying(Character* character)
{
    if (character == NULL)
    {
        return false;
    }

    bool dying = false;
    if (!SafeIsProbablyDying(character, &dying))
    {
        return false;
    }

    bool pretending = false;
    SafeIsPretending(character, &pretending);

    return dying && !pretending;
}

bool IsInPlayerCage(Character* character)
{
    if (character == NULL)
    {
        return false;
    }

    bool inPrison = false;
    if (!SafeInPrison(character, &inPrison) || !inPrison)
    {
        return false;
    }

    Faction* playerFaction = NULL;
    if (!SafePlayerFaction(&playerFaction))
    {
        return false;
    }

    // Held in an actual cage: the cage's owner decides whose prisoner they are.
    Faction* cageFaction = NULL;
    if (SafeCageFaction(character, &cageFaction))
    {
        return cageFaction == playerFaction;
    }

    // Shackled or carried off, with no cage object to ask. The engine still
    // knows who owns them, so fall back to that -- otherwise a prisoner dragged
    // home would be unreachable for having been put down in the wrong spot.
    Character* leader = NULL;
    if (SafePlayerLeader(&leader))
    {
        bool mine = false;
        if (SafeIsMyPrisoner(leader, character, &mine) && mine)
        {
            return true;
        }
    }

    return false;
}

namespace
{
// lektor is an engine-owned buffer that the caller has to release, so the gather
// and the free are paired here, each in its own POD-only frame.
bool SafeGatherNearby(const Ogre::Vector3& centre, float radius, lektor<RootObject*>* out)
{
    TR_TRY
    {
        ou->getCharactersWithinSphere(*out, centre, radius, 0.0f, 30.0f, 60, 0, NULL);
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
} // namespace

bool SafeAsCharacter(RootObject* object, Character** out)
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

bool CharactersNear(const Ogre::Vector3& centre, float radius, std::vector<Character*>* out)
{
    lektor<RootObject*> nearby;
    if (!SafeGatherNearby(centre, radius, &nearby))
    {
        return false;
    }

    for (unsigned int i = 0; i < nearby.size(); ++i)
    {
        Character* npc = NULL;
        if (SafeAsCharacter(nearby[i], &npc))
        {
            out->push_back(npc);
        }
    }

    SafeFreeList(&nearby);
    return true;
}

std::string CharacterName(Character* character)
{
    if (character == NULL)
    {
        return "<unnamed>";
    }

    char buffer[128] = { 0 };
    if (!SafeCopyName(character, buffer, sizeof(buffer)))
    {
        return "<unnamed>";
    }
    return std::string(buffer);
}
} // namespace totalrecruit
