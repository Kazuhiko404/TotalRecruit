#include "common.h"
#include "mercy.h"
#include "rules.h"
#include "config.h"
#include "recruit_core.h"
#include "offer_popup.h"
#include "dialogue_authoring.h"
#include "log.h"

#include <map>

namespace totalrecruit
{
namespace
{
// Who is currently pleading. One at a time: a brawl can cripple several people
// in the same second, and a stack of windows would be worse than none.
Character* g_pleading = NULL;

// Everyone who has already begged. A second lost limb is not a second plea --
// the player already gave their answer.
std::map<Character*, bool> g_begged;

bool SafeIsPlayers(Character* npc, bool* out)
{
    TR_TRY
    {
        *out = npc->isPlayerCharacter();
        return true;
    }
    TR_CATCH { return false; }
}

bool SafePlayerLeaderLocal(Character** out)
{
    TR_TRY
    {
        if (ou == NULL || ou->player == NULL || ou->player->currentPlatoon == NULL)
        {
            return false;
        }
        *out = ou->player->currentPlatoon->getSquadLeader();
        return (*out != NULL);
    }
    TR_CATCH { return false; }
}

// What they say while the window is open. Deliberately not branched on
// personality: the whole point of this route is that the wound outranks the
// temperament, so a maniac and a coward beg with the same words.
std::string PleaLine()
{
    return "Piedad. Ya no puedo pelear. Hare lo que me pidas.";
}
} // namespace

bool IsMercyPending(Character* npc)
{
    return npc != NULL && npc == g_pleading;
}

void ForgetMercy(Character* /*npc*/)
{
    g_pleading = NULL;
}

void GrantMercy(Character* npc)
{
    if (npc == NULL)
    {
        return;
    }

    g_pleading = NULL;

    Character* leader = NULL;
    SafePlayerLeaderLocal(&leader);

    // The same reset the rescue route uses: combat off, the player is no
    // longer an enemy, and the AI is told to reconsider so they actually walk
    // away instead of standing there in a stale attack order.
    SafePacify(npc, leader);

    const std::string name = CharacterName(npc);
    SpeakAloud(npc, "Gracias... no lo olvidar\xc3\xa9.");
    TellPlayer(name + ": lo dejaste marchar.", false);
    LogInfo("Spared " + name + " after they lost a limb.");
}

void OfferMercy(Character* npc)
{
    if (npc == NULL || g_pleading != NULL)
    {
        return;
    }

    // Already yours, or already asked once. A limb lost in a later fight does
    // not reopen a question the player has answered.
    bool mine = false;
    if (SafeIsPlayers(npc, &mine) && mine)
    {
        return;
    }
    if (g_begged.find(npc) != g_begged.end())
    {
        return;
    }

    // The exclusion list still rules: someone the player has told the mod to
    // leave alone does not get to beg their way in.
    if (!IsRecruitable(npc, NULL))
    {
        return;
    }

    g_begged[npc] = true;
    g_pleading = npc;

    const std::string line = PleaLine();
    SpeakAloud(npc, line);

    // Three buttons, because two would be a lie. Sparing them is a real
    // outcome, and it is the only one here that costs the player nothing and
    // gains them nothing.
    if (!ShowOffer(npc, line, "ACEPTAR", "RECHAZAR", "PERDONAR"))
    {
        // No window means no choice, and silently recruiting or silently
        // sparing would both be decisions the player never made.
        g_pleading = NULL;
        LogError("Could not open the mercy window for " + CharacterName(npc));
    }
}

void ShutdownMercy()
{
    g_pleading = NULL;
    g_begged.clear();
}
} // namespace totalrecruit
