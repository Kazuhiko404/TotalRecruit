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

// A captive who wants to talk keeps asking, on the game clock: the first call
// after hailFirstHours, then one every hailRepeatHours, up to hailRepeats in
// total. A single shout was too easy to walk past and miss.
struct HailState
{
    // Game hour at which the next call is due.
    double dueAt;
    // How many calls have already gone out.
    int spoken;

    HailState() : dueAt(0.0), spoken(0) {}
};

// Keyed by CharKey (rules.h), never by the pointer. This map is swept several
// times a minute and never pruned, so a raw address would keep a dead
// character's entry alive and hand it to whoever the allocator puts at that
// address next -- a stranger inheriting a dead man's hail clock, and a call
// into a Character that no longer exists.
std::map<CharKey, HailState> g_hailed;

unsigned int g_nextSweep = 0;

// The same clock the price rules use, so "hours" means one thing across the
// mod: in-game hours, which stop when the game is paused.
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
//
// Cheap: address identity only, no engine traversal. Everything expensive is
// gated behind this and behind the clock.
bool CouldEverHail(Character* npc)
{
    return npc != NULL && IsInPlayerCage(npc) && IsRecruitable(npc, NULL);
}

// Expensive: the full decision engine -- medical state, faction, personality,
// talent across 39 stats, price. Asked once per due call rather than once per
// sweep, because the answer cannot change the outcome until the clock says so.
bool IsReadyToHail(Character* npc)
{
    const Verdict verdict = Evaluate(npc);
    return verdict.willJoin;
}

void SweepForHails()
{
    double now = 0.0;
    if (!SafeGameHours(&now))
    {
        return;
    }

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

        // Cheapest first: most characters in range are not captives at all.
        if (!CouldEverHail(npc))
        {
            continue;
        }

        CharKey key;
        if (!SafeCharKey(npc, &key))
        {
            continue;
        }

        const Config& config = GetConfig();
        std::map<CharKey, HailState>::iterator found = g_hailed.find(key);
        if (found == g_hailed.end())
        {
            // First sighting only starts the clock. They have just been caged;
            // shouting immediately would make the wait meaningless.
            HailState fresh;
            fresh.dueAt = now + config.hailFirstHours;
            g_hailed[key] = fresh;
            continue;
        }

        HailState& state = found->second;
        if (state.spoken >= config.hailRepeats || now < state.dueAt)
        {
            continue;
        }

        // Only now, with the clock already due, is the full verdict worth
        // computing. Asking it every sweep ran the whole decision engine on
        // every caged NPC several times a minute for an answer nobody could
        // act on yet.
        if (!IsReadyToHail(npc))
        {
            continue;
        }

        state.spoken += 1;
        state.dueAt = now + config.hailRepeatHours;

        SpeakAloud(npc, HailLine(npc));
        // The bubble floats over their head and fades; the roller keeps a line
        // the player can still read after turning the camera away.
        TellPlayer(CharacterName(npc) + " quiere hablar contigo.", false);
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
