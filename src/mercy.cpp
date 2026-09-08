#include "common.h"
#include "mercy.h"
#include "rules.h"
#include "messages.h"
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

bool SafeReadPersonality(Character* npc, int* out)
{
    TR_TRY
    {
        StateBroadcastData* broadcast = npc->getStateBroadcast();
        if (broadcast == NULL)
        {
            return false;
        }
        *out = (int)broadcast->myPersonality;
        return true;
    }
    TR_CATCH { return false; }
}

// What they say while the window is open. Deliberately not branched on
// personality: the whole point of this route is that the wound outranks the
// temperament, so a maniac and a coward beg with the same words.
// Dead, bled out, or past saving. blood below zero only happens to a body that
// has bled out, which is the honest death signal -- isDead alone missed these.
bool IsBeyondPleading(Character* npc)
{
    TR_TRY
    {
        if (npc->isDead())
        {
            return true;
        }
        MedicalSystem* medical = npc->getMedical();
        if (medical == NULL)
        {
            return false;
        }
        if (medical->isDead() || medical->isProbablyDying())
        {
            return true;
        }
        return (medical->blood < 0.0f);
    }
    TR_CATCH { return true; }
}

// The plea, in their own voice. It used to be one hardcoded sentence for
// everyone, which meant the mod's most dramatic moment was also its most
// repetitive -- and it never said who was speaking, so a window full of
// strangers all begged in identical words.
std::string PleaLine(Character* npc)
{
    return Fill(Say("mercy.plea",
                    "{name}: Piedad. Ya no puedo pelear. Hare lo que me pidas.",
                    npc),
                CharacterName(npc), 0, 0);
}

// Who can buy their way out of this, and who cannot.
//
// A coward, a turncoat or a plain brute will offer whatever they carry -- and
// that is what gives "Ignorar" its weight, because walking away now costs the
// player something concrete. An honourable or brave one will not: they join or
// they die on their feet. Letting them purchase an exit would contradict the
// one thing that defines them, and would cheapen both routes at once.
bool CanBuyTheirLife(Character* npc)
{
    int personality = PT_NORMAL;
    if (!SafeReadPersonality(npc, &personality))
    {
        return false;
    }

    switch (personality)
    {
    case PT_FEARFUL:
    case PT_TRAITOROUS:
    case PT_DUMB:
    case PT_COLD_CRUEL:
    case PT_MANIACAL:
        return true;
    default:
        return false;   // honourable, brave, and anyone unread
    }
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

// Moves what the beaten NPC is carrying into the player's purse.
//
// takeMoney(n) removes n and reports whether the funds were there, so a
// negative n is the documented way the engine adds -- the same call the price
// path already relies on, run in reverse. Whatever the NPC actually has is
// taken first, so this can never mint cats out of nothing.
bool SafeMoveMoney(Character* from, Character* to, int* outAmount)
{
    TR_TRY
    {
        const int purse = from->_NV_getMoney();
        if (purse <= 0)
        {
            return false;
        }
        if (!from->_NV_takeMoney(purse))
        {
            return false;
        }
        to->_NV_takeMoney(-purse);
        *outAmount = purse;
        return true;
    }
    TR_CATCH { return false; }
}

bool PayTribute(Character* npc)
{
    Character* leader = NULL;
    if (!SafePlayerLeaderLocal(&leader))
    {
        return false;
    }

    int amount = 0;
    if (!SafeMoveMoney(npc, leader, &amount))
    {
        return false;
    }

    char buf[64] = { 0 };
    _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%d", amount);
    LogInfo("Tribute of " + std::string(buf) + " cats from " + CharacterName(npc));
    return true;
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
    // They pay for the road out with whatever they carry. That is what makes
    // this "ignore" rather than "spare": the player is declining something
    // concrete, not just being merciful in the abstract.
    if (CanBuyTheirLife(npc) && PayTribute(npc))
    {
        TellPlayer(name + ": te dej\xc3\xb3 lo que llevaba y se fue.", false);
    }
    else
    {
        TellPlayer(name + ": lo dejaste marchar.", false);
    }
    LogInfo("Let " + name + " go after they lost a limb.");
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

    // A corpse does not negotiate. Checked here rather than trusting the
    // exclusion list, because exclude.dead defaults to off and this is not a
    // preference: two bodies with blood at -0.42 and -0.95 both begged for
    // mercy, which reads as a bug however the player has configured things.
    if (IsBeyondPleading(npc))
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

    const std::string line = PleaLine(npc);
    SpeakAloud(npc, line);

    // Ignoring is only a third option when there is something to ignore. An
    // honourable NPC has made this a two-answer question: take them or fight.
    const char* thirdLabel = CanBuyTheirLife(npc) ? "IGNORAR" : "";

    if (!ShowOffer(npc, line, "ACEPTAR", "RECHAZAR", thirdLabel))
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
