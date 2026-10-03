#include "common.h"
#include "cursor_hint.h"
#include "rules.h"
#include "config.h"
#include "recruit_core.h"
#include "talk_hint_cache.h"
#include "log.h"

#include <kenshi/gui/ForgottenGUI.h>
#include <kenshi/Dialogue.h>

namespace totalrecruit
{
namespace
{
void (*g_changeMouseCursor_orig)(ForgottenGUI*, CursorType, const hand&, const hand&) = NULL;
bool (*g_hasDialogueFast_orig)(Dialogue*) = NULL;

// Resolving the target costs a rules evaluation, and the engine asks constantly,
// once per candidate NPC in view, so every answer is remembered per NPC for a
// couple of seconds (talk_hint_cache.h). A one-entry cache thrashed with two NPCs
// on screen and paid a full evaluation per NPC per frame.
TalkHintCache g_hints;

bool SafeResolveTarget(const hand& target, Character** out)
{
    TR_TRY
    {
        *out = target.getCharacter();
        return (*out != NULL);
    }
    TR_CATCH { return false; }
}

// Only the "there is something to talk about" question, not the full verdict:
// a refusal the player can act on still deserves the cursor, because finding
// out what someone wants is the interaction.
bool WorthTalkingTo(Character* npc)
{
    if (npc == NULL)
    {
        return false;
    }

    if (!IsRecruitable(npc, NULL))
    {
        return false;
    }

    // Asked as a bystander: a hint must not mint roll keys, start hold clocks or
    // log, and the engine asks this for every NPC in view. A peek supplies no roll
    // key, so a route that rolls never says willJoin here; a real chance of
    // joining is what makes them worth approaching instead.
    const Verdict verdict = EvaluatePeek(npc, NULL);

    // Someone who will (or may) join, or whose refusal names something the
    // player can do about it. A flat "they are dead" is not worth a cursor.
    if (verdict.willJoin || (verdict.rolled && verdict.chancePercent > 0))
    {
        return true;
    }

    return verdict.currency == Currency_Care ||
           verdict.currency == Currency_Cats ||
           verdict.currency == Currency_Submission ||
           verdict.currency == Currency_Freedom;
}

// Dialogue::me is the character this dialogue belongs to (Dialogue.h:395).
bool SafeDialogueOwner(Dialogue* dialogue, Character** out)
{
    TR_TRY
    {
        if (dialogue == NULL)
        {
            return false;
        }
        *out = dialogue->me;
        return (*out != NULL);
    }
    TR_CATCH { return false; }
}

// The hand parameters are references, and the compiler counts those as objects,
// so the guarded read lives in its own POD-only frame.
// The one place the cache is consulted, so the marker and the cursor can never
// disagree about who is worth approaching.
bool CachedWorthTalkingTo(Character* npc)
{
    const unsigned int now = GetTickCount();
    bool worth = false;
    if (!g_hints.Lookup(npc, now, &worth))
    {
        worth = WorthTalkingTo(npc);
        g_hints.Store(npc, now, worth);
    }
    return worth;
}

bool CachedWorthTalkingTo(const hand& target)
{
    Character* npc = NULL;
    if (!SafeResolveTarget(target, &npc))
    {
        return false;
    }
    return CachedWorthTalkingTo(npc);
}


// The vanilla speech-bubble marker over an NPC's head is drawn by the game
// whenever that NPC has something to say, and the question it asks is
// hasDialogue_Fast -- "Fast" because it runs constantly, once per candidate.
//
// So the marker needs no widget of our own and no 3D-to-screen projection:
// answering yes for someone our rules would talk to makes Kenshi draw its own
// icon, in its own style, in the right place. This is what the earlier attempt
// at a custom overlay should have been.
bool HasDialogueFast_hook(Dialogue* thisptr)
{
    bool original = false;
    if (g_hasDialogueFast_orig != NULL)
    {
        original = g_hasDialogueFast_orig(thisptr);
    }

    // Never take the marker away: an NPC with real FCS dialogue keeps it.
    if (original || !GetConfig().showTalkCursor)
    {
        return original;
    }

    Character* npc = NULL;
    if (!SafeDialogueOwner(thisptr, &npc))
    {
        return original;
    }

    return CachedWorthTalkingTo(npc);
}

void ChangeMouseCursor_hook(ForgottenGUI* thisptr, CursorType cursor,
                            const hand& player, const hand& target)
{
    CursorType chosen = cursor;

    // Only ever upgrade a plain cursor. An attack, medic or loot cursor means
    // the player is already being offered something specific, and stealing that
    // would break an interaction the game owns.
    if (GetConfig().showTalkCursor && cursor == DEFAULT_CURSOR)
    {
        if (CachedWorthTalkingTo(target))
        {
            chosen = TALK_CURSOR;
        }
    }

    if (g_changeMouseCursor_orig != NULL)
    {
        g_changeMouseCursor_orig(thisptr, chosen, player, target);
    }
}
} // namespace

bool InstallCursorHint()
{
    // The three-argument overload is the one that carries the target, so it has
    // to be disambiguated explicitly.
    typedef void (ForgottenGUI::*ChangeCursorFn)(CursorType, const hand&, const hand&);
    ChangeCursorFn target = &ForgottenGUI::changeMouseCursor;

    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(target),
            ChangeMouseCursor_hook, &g_changeMouseCursor_orig))
    {
        LogError("Could not hook ForgottenGUI::changeMouseCursor; no talk cursor.");
        return false;
    }

    LogInfo("Talk cursor installed.");

    // The marker is the part that actually shows on screen; the cursor hook
    // above has never been observed firing (see notes S11), so this failing is
    // worth its own line rather than being folded into the same result.
    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&Dialogue::hasDialogue_Fast),
            HasDialogueFast_hook, &g_hasDialogueFast_orig))
    {
        LogError("Could not hook Dialogue::hasDialogue_Fast; no talk marker.");
        return false;
    }

    LogInfo("Talk marker installed.");
    return true;
}

void ForgetCursorTarget()
{
    g_hints.Clear();
}

void ShutdownCursorHint()
{
    ForgetCursorTarget();
}
} // namespace totalrecruit
