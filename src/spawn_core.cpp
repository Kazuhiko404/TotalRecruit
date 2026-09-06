#include "common.h"
#include "spawn_core.h"
#include "config.h"
#include "log.h"

#include <cmath>

#include <kenshi/RootObjectFactory.h>

namespace totalrecruit
{
namespace
{
// Ring placement constants taken from Emkej's Test Kit
// (test_kit_spawn_runtime.cpp:20-29). Spawning every unit at one point leaves
// them stacked inside each other.
const float kBaseRadius = 180.0f;
const float kRingSpacing = 120.0f;
const int kFirstRingSlots = 6;
const float kPi = 3.14159265358979323846f;

const int kMaxPerSpawn = 50;

int g_spawnedThisSession = 0;

void RingOffset(int index, float* outX, float* outZ)
{
    int ring = 0;
    int slot = index;
    int slotsInRing = kFirstRingSlots;
    while (slot >= slotsInRing)
    {
        slot -= slotsInRing;
        ++ring;
        slotsInRing = kFirstRingSlots * (ring + 1);
    }

    const float radius = kBaseRadius + (float)ring * kRingSpacing;
    const float angle = (2.0f * kPi * (float)slot) / (float)slotsInRing;
    *outX = cosf(angle) * radius;
    *outZ = sinf(angle) * radius;
}

bool IsFinite(const Ogre::Vector3& position)
{
    const float axes[3] = { position.x, position.y, position.z };
    for (int i = 0; i < 3; ++i)
    {
        // NaN compares false against itself.
        if (axes[i] != axes[i])
        {
            return false;
        }
        if (axes[i] > 1.0e30f || axes[i] < -1.0e30f)
        {
            return false;
        }
    }
    return true;
}

// Anchors spawns to a character that is already standing on valid ground.
bool ResolveAnchor(Ogre::Vector3* outPosition)
{
    TR_TRY
    {
        if (ou == NULL || ou->player == NULL)
        {
            return false;
        }

        lektor<Character*>& characters = ou->player->playerCharacters;
        for (unsigned int i = 0; i < characters.count; ++i)
        {
            Character* character = characters.stuff[i];
            if (character == NULL)
            {
                continue;
            }
            const Ogre::Vector3 position = character->getPosition();
            if (IsFinite(position))
            {
                *outPosition = position;
                return true;
            }
        }
    }
    TR_CATCH { return false; }

    return false;
}

bool ResolvePlayerState(Faction** outFaction, Platoon** outPlatoon, ActivePlatoon** outSquad)
{
    TR_TRY
    {
        if (ou == NULL || ou->player == NULL)
        {
            return false;
        }
        *outFaction = ou->player->participant;
        *outPlatoon = ou->player->currentPlatoon;
        *outSquad = (*outPlatoon != NULL) ? (*outPlatoon)->activePlatoon : NULL;
        return (*outFaction != NULL);
    }
    TR_CATCH { return false; }
}

// One unit: create it, place it in the right faction, settle it on the ground.
bool SpawnOne(Faction* faction, ActivePlatoon* squad,
              GameData* templateData, const Ogre::Vector3& anchor, int index,
              bool recruit)
{
    float offsetX = 0.0f;
    float offsetZ = 0.0f;
    RingOffset(index, &offsetX, &offsetZ);
    const Ogre::Vector3 position(anchor.x + offsetX, anchor.y, anchor.z + offsetZ);

    RootObject* created = NULL;
    TR_TRY
    {
        // Only ActivePlatoon derives from RootObjectContainer (Platoon.h:218);
        // Platoon does not, so it must never be cast to one.
        RootObjectContainer* owner = recruit ? squad : NULL;
        created = ou->theFactory->createRandomCharacter(
            faction, position, owner, templateData, NULL, 0.0f);
    }
    TR_CATCH { return false; }

    if (created == NULL)
    {
        return false;
    }

    if (recruit)
    {
        TR_TRY { created->setFaction(faction, squad); }
        TR_CATCH { }
    }

    // Without this the unit can end up inside terrain; it is what makes
    // spawning safe without sampling terrain height ourselves.
    TR_TRY
    {
        Character* character = static_cast<Character*>(created);
        character->setRagdollNavmeshSafePos();
    }
    TR_CATCH { }

    return true;
}

bool CheckBudget(int requested, std::string* outMessage)
{
    const Config& config = GetConfig();
    if (config.maxSpawnedCharacters > 0 &&
        g_spawnedThisSession + requested > config.maxSpawnedCharacters)
    {
        char message[192];
        sprintf_s(message, sizeof(message),
                  "spawn limit reached (%d/%d); reload or raise maxSpawnedCharacters",
                  g_spawnedThisSession, config.maxSpawnedCharacters);
        if (outMessage) *outMessage = message;
        return false;
    }
    return true;
}

bool RunSpawn(const std::vector<GameData*>& templates, std::string* outMessage)
{
    if (templates.empty())
    {
        if (outMessage) *outMessage = "nothing to spawn";
        return false;
    }

    if (!CheckBudget((int)templates.size(), outMessage))
    {
        return false;
    }

    Faction* faction = NULL;
    Platoon* platoon = NULL;
    ActivePlatoon* squad = NULL;
    if (!ResolvePlayerState(&faction, &platoon, &squad))
    {
        if (outMessage) *outMessage = "game not loaded";
        return false;
    }

    Ogre::Vector3 anchor;
    if (!ResolveAnchor(&anchor))
    {
        if (outMessage) *outMessage = "no living squad member to spawn next to";
        return false;
    }

    const bool recruit = GetConfig().autoRecruit;

    int created = 0;
    for (size_t i = 0; i < templates.size(); ++i)
    {
        if (SpawnOne(faction, squad, templates[i], anchor, (int)i, recruit))
        {
            ++created;
        }
    }

    if (created == 0)
    {
        if (outMessage) *outMessage = "the engine created nothing";
        return false;
    }

    g_spawnedThisSession += created;

    char message[128];
    sprintf_s(message, sizeof(message), "%d spawned (%s)", created,
              recruit ? "recruited" : "roaming");
    if (outMessage) *outMessage = message;
    LogInfo(message);
    return true;
}
} // namespace

bool SpawnTemplate(const DataEntry& entry, int quantity, std::string* outMessage)
{
    if (entry.data == NULL)
    {
        if (outMessage) *outMessage = "nothing selected";
        return false;
    }

    int count = quantity;
    if (count < 1) count = 1;
    if (count > kMaxPerSpawn) count = kMaxPerSpawn;

    std::vector<GameData*> templates;
    for (int i = 0; i < count; ++i)
    {
        templates.push_back(entry.data);
    }
    return RunSpawn(templates, outMessage);
}

bool SpawnMembers(const std::vector<DataEntry>& members, std::string* outMessage)
{
    if (members.empty())
    {
        if (outMessage) *outMessage = "this squad lists no members";
        return false;
    }

    std::vector<GameData*> templates;
    for (size_t i = 0; i < members.size() && templates.size() < kMaxPerSpawn; ++i)
    {
        if (members[i].data != NULL)
        {
            templates.push_back(members[i].data);
        }
    }
    return RunSpawn(templates, outMessage);
}

namespace
{
// createRandomSquad takes the template itself, so the engine builds the roster,
// the leader and the unit counts. Reading the leader/squad/squad2 lists by hand
// loses all of that, which is why squads came out as a couple of random units.
// Argument order follows KEP's SpawnTool.cpp:517 (player) and :557 (roaming).
Platoon* CallCreateRandomSquad(Faction* faction, const Ogre::Vector3& position,
                               GameData* squadTemplate, SquadType squadType)
{
    return ou->theFactory->createRandomSquad(
        faction,        //  1 faction
        position,       //  2 position
        NULL,           //  3 homeTown
        99,             //  4 maxnum
        NULL,           //  5 home building
        squadTemplate,  //  6 squad template
        NULL,           //  7 owner
        NULL,           //  8 map area
        NULL,           //  9 active platoon
        false,          // 10 permanent
        hand(),         // 11 AI target
        NULL,           // 12 target town
        1.0f,           // 13 size multiplier
        squadType,      // 14 squad type
        false);         // 15 is just a refresh
}

Platoon* SafeCreateRandomSquad(Faction* faction, const Ogre::Vector3& position,
                               GameData* squadTemplate, SquadType squadType)
{
    TR_TRY { return CallCreateRandomSquad(faction, position, squadTemplate, squadType); }
    TR_CATCH { return NULL; }
}

int SafeGetCharacterCount(Platoon* platoon)
{
    TR_TRY { return platoon->getCharacterCount(); }
    TR_CATCH { return 0; }
}

// A roaming squad needs a faction of its own; the template names it.
Faction* SafeResolveTemplateFaction(GameData* squadTemplate)
{
    TR_TRY
    {
        if (ou == NULL || ou->factionMgr == NULL)
        {
            return NULL;
        }
        return ou->factionMgr->getFactionByStringID(squadTemplate->stringID);
    }
    TR_CATCH { return NULL; }
}
} // namespace

bool SpawnSquad(const DataEntry& squad, SquadSpawnMode mode, std::string* outMessage)
{
    if (squad.data == NULL)
    {
        if (outMessage) *outMessage = "no squad selected";
        return false;
    }

    if (ou == NULL || ou->theFactory == NULL || ou->player == NULL)
    {
        if (outMessage) *outMessage = "game not loaded";
        return false;
    }

    // A squad can bring dozens of units, so reserve the full budget up front.
    if (!CheckBudget(kMaxPerSpawn, outMessage))
    {
        return false;
    }

    Faction* faction = NULL;
    Platoon* unusedPlatoon = NULL;
    ActivePlatoon* unusedSquad = NULL;
    if (!ResolvePlayerState(&faction, &unusedPlatoon, &unusedSquad))
    {
        if (outMessage) *outMessage = "could not read player state";
        return false;
    }

    SquadType squadType = SQ_UNKNOWN;
    if (mode == SquadSpawn_Roaming)
    {
        Faction* own = SafeResolveTemplateFaction(squad.data);
        if (own != NULL)
        {
            faction = own;
        }
        squadType = SQ_ROAMING;
    }

    Ogre::Vector3 anchor;
    if (!ResolveAnchor(&anchor))
    {
        if (outMessage) *outMessage = "no living squad member to spawn next to";
        return false;
    }

    Platoon* platoon = SafeCreateRandomSquad(faction, anchor, squad.data, squadType);
    if (platoon == NULL)
    {
        if (outMessage) *outMessage = "the engine created no squad";
        return false;
    }

    const int count = SafeGetCharacterCount(platoon);
    if (count == 0)
    {
        if (outMessage) *outMessage = "the spawned squad is empty";
        return false;
    }

    g_spawnedThisSession += count;

    char message[192];
    sprintf_s(message, sizeof(message), "%s: %d units (%s)", squad.name.c_str(),
              count, (mode == SquadSpawn_Roaming) ? "roaming" : "yours");
    if (outMessage) *outMessage = message;

    char detail[320];
    sprintf_s(detail, sizeof(detail), "%s [%s]: %d units (%s)",
              squad.name.c_str(), squad.stringID.c_str(), count,
              (mode == SquadSpawn_Roaming) ? "roaming" : "yours");
    LogInfo(detail);
    return true;
}

int SpawnedCount()
{
    return g_spawnedThisSession;
}
} // namespace totalrecruit
