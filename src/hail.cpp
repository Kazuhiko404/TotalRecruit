#include "common.h"
#include "hail.h"
#include "rules.h"
#include "hold_tracking.h"
#include "npc_speech.h"
#include "char_key_reader.h"
#include "defeats.h"
#include "engine_reads.h"
#include "config.h"
#include "recruit_core.h"
#include "dialogue_authoring.h"
#include "head_notice.h"
#include "messages.h"
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

// Only captives call out (the caller has already established the cage). A free
// NPC shouting "let us talk" as the player walks past a town would be noise,
// and the mod has nothing to offer them anyway.
//
// Cheap: no engine traversal beyond the exclusion list. Everything expensive is
// gated behind this and behind the clock.
bool CouldEverHail(Character* npc)
{
    return npc != NULL && IsRecruitable(npc, NULL);
}

// Expensive: the full decision engine -- medical state, faction, personality,
// talent across 39 stats, price. Asked once per due call rather than once per
// sweep, because the answer cannot change the outcome until the clock says so.
bool IsReadyToHail(Character* npc)
{
    const Verdict verdict = Evaluate(npc);
    return verdict.willJoin;
}

// The sweep also keeps the hold clock honest for everyone in range (see
// NoteCageState), which is why it runs even with hailing switched off:
// `hailing` only decides whether a ready captive calls out.
void SweepForHails(bool hailing)
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

    std::vector<Character*> nearby;
    if (!CharactersNear(centre, kHailRadius, &nearby))
    {
        return;
    }

    for (unsigned int i = 0; i < nearby.size(); ++i)
    {
        Character* npc = nearby[i];

        // Looked at for everyone in range, caged or not: the first sighting in
        // the cage starts the prisoner's hold clock and a sighting outside it
        // ends it, so a released-and-recaged prisoner starts over.
        const bool caged = NoteCageState(npc);

        // Already looking at them: someone the player put down who is back on
        // their feet re-arms the next knockout (defeat_episode_rules). Reads the
        // engine only for a character whose last knockout counted.
        NoteSeenStanding(npc);

        // Cheapest first: most characters in range are not captives at all.
        if (!hailing || !caged || !CouldEverHail(npc))
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
        // And over their head, so a player who missed the name sees who it was.
        RaiseHeadNotice(npc, Say("headNotice.wantsToTalk", "Quiere hablar contigo", npc));
        LogInfo(CharacterName(npc) + " called out to the player.");
    }
}
} // namespace

void ForgetAllHails()
{
    g_hailed.clear();
    g_nextSweep = 0;
}

bool InstallHailing()
{
    ForgetAllHails();
    ResetHoldClock();
    LogInfo("Prisoner hailing installed.");
    return true;
}

void ShutdownHailing()
{
    g_hailed.clear();
    ResetHoldClock();
}

void HailTick()
{
    const unsigned int now = GetTickCount();
    if (now < g_nextSweep)
    {
        return;
    }
    g_nextSweep = now + kSweepIntervalMs;

    SweepForHails(GetConfig().hailWhenReady);
}
} // namespace totalrecruit
