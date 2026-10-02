#include "common.h"
#include "mercy.h"
#include "rules.h"
#include "messages.h"
#include "config.h"
#include "recruit_core.h"
#include "offer_popup.h"
#include "dialogue_authoring.h"
#include "log.h"
#include "mercy_rules.h"
#include "plea_tracker.h"
#include "cats_purse.h"
#include "money_rules.h"

#include <set>

namespace totalrecruit
{
namespace
{
// Who is currently pleading. One at a time: a brawl can cripple several people
// in the same second, and a stack of windows would be worse than none.
//
// Held as a CharKey (rules.h) inside a PleaTracker, not as the Character*: an
// address is not an identity, and the engine hands a freed character's address
// to the next one, so a stranger could answer to a dead man's plea.
PleaTracker g_pleading;

// Everyone who has already begged. A second lost limb is not a second plea --
// the player already gave their answer.
//
// Keyed by CharKey (rules.h) rather than by the pointer: an address is not an
// identity, and a fresh character at a dead one's address would be marked as
// having already pleaded and could never open the window.
std::set<CharKey> g_begged;

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
bool SafeIsBeast(Character* npc, bool* out)
{
    TR_TRY { *out = (npc->isAnimal() != NULL); return true; }
    TR_CATCH { return false; }
}

bool SafeIsOut(Character* npc, bool* out)
{
    TR_TRY
    {
        *out = npc->_NV_isLiterallyUnconciousNotPretending();
        return true;
    }
    TR_CATCH { return false; }
}

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

// Their temperament, as the engine-free enum the plea rules (mercy_rules.cpp)
// take. Unreadable reads as Normal, which can neither buy its life nor be
// refused the accept.
Temperament ReadTemperament(Character* npc)
{
    int personality = PT_NORMAL;
    if (!SafeReadPersonality(npc, &personality))
    {
        return Temperament_Normal;
    }
    return ToTemperament(personality);
}
} // namespace

bool IsMercyPending(Character* npc)
{
    CharKey key;
    return npc != NULL && SafeCharKey(npc, &key) && g_pleading.IsPending(key);
}

void ForgetMercy(Character* npc)
{
    if (npc == NULL)
    {
        g_pleading.Clear();
        return;
    }

    CharKey key;
    if (SafeCharKey(npc, &key))
    {
        g_pleading.ClearIf(key);
    }
}

// What the beaten NPC is carrying goes to the player. The move is
// money_rules.cpp's TransferAllCats: a checked debit from them, then a credit to
// the player, with the debit put back if the credit fails -- so this can neither
// mint cats out of nothing nor destroy the NPC's purse when the credit is refused.
bool PayTribute(Character* npc)
{
    Character* leader = NULL;
    if (!SafePlayerLeaderLocal(&leader))
    {
        return false;
    }

    CharacterPurse theirs(npc);
    CharacterPurse mine(leader);
    const int amount = TransferAllCats(theirs, mine);
    if (amount <= 0)
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

    ForgetMercy(npc);

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
    if (ResolvePlea(PleaAnswer_Ignore, ReadTemperament(npc)) == PleaOutcome_LetGoWithTribute &&
        PayTribute(npc))
    {
        TellPlayer(name + ": te dej\xc3\xb3 lo que llevaba y se fue.", false);
    }
    else
    {
        TellPlayer(name + ": lo dejaste marchar.", false);
    }
    LogInfo("Let " + name + " go after they lost a limb.");
}

// ACEPTAR on a plea. The plea itself said "I will do what you ask", so the
// answer is not a negotiation: they join, free of charge, whoever they are.
//
// This used to go through the ordinary Evaluate like any other offer, which
// could answer a plea with a cats price, or with "his honour forbids bribes" --
// the opposite of R3.6 (the honourable join or die) and R3.7 (accepting joins).
void AcceptMercy(Character* npc)
{
    if (npc == NULL)
    {
        return;
    }

    ForgetMercy(npc);

    const std::string name = CharacterName(npc);
    if (ResolvePlea(PleaAnswer_Accept, ReadTemperament(npc)) != PleaOutcome_Join)
    {
        return;
    }

    std::string message;
    if (Recruit(npc, &message))
    {
        TellPlayer(Fill(Say("mercy.joinedNotice", "{name}: se une a ti.", npc),
                        name, 0, 0),
                   false);
        LogInfo("Accepted the plea of " + name + ": joined without a price.");
    }
    else
    {
        TellPlayer(name + ": " + message, true);
    }
}

void OfferMercy(Character* npc)
{
    if (npc == NULL || g_pleading.HasPending())
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
    CharKey key;
    if (!SafeCharKey(npc, &key) || g_begged.find(key) != g_begged.end())
    {
        return;
    }

    // An animal has no words to beg with. The plea is written for someone who
    // can speak, and a pack beast asking for mercy in Spanish is the kind of
    // thing that breaks the fiction the rest of the mod works to keep. Beasts
    // are tamed with food, and that path is elsewhere.
    bool beast = false;
    if (SafeIsBeast(npc, &beast) && beast)
    {
        return;
    }

    // Nobody negotiates while out cold. The window was opening over
    // unconscious bodies -- the verdict said "wait until they wake up" and the
    // plea appeared anyway, because these are two different code paths and
    // only one of them was checking.
    bool out = false;
    if (SafeIsOut(npc, &out) && out)
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

    g_begged.insert(key);

    const std::string line = PleaLine(npc);
    SpeakAloud(npc, line);

    // Ignoring is only a third option when there is something to ignore. An
    // honourable NPC has made this a two-answer question: take them or fight.
    const char* thirdLabel = CanBuyTheirLife(ReadTemperament(npc)) ? "IGNORAR" : "";

    if (!ShowOffer(npc, line, "ACEPTAR", "RECHAZAR", thirdLabel))
    {
        // No window means no choice, and silently recruiting or silently
        // sparing would both be decisions the player never made.
        LogError("Could not open the mercy window for " + CharacterName(npc));
        return;
    }

    // Marked only now that the window is up: ShowOffer clears any plea so that
    // an ordinary offer cannot inherit one, and this one has just replaced it.
    g_pleading.Begin(key);
}

void ShutdownMercy()
{
    g_pleading.Clear();
    g_begged.clear();
}
} // namespace totalrecruit
