#include "common.h"
#include "cursor_hint.h"
#include "rules.h"
#include "config.h"
#include "recruit_core.h"
#include "log.h"

#include <kenshi/gui/ForgottenGUI.h>

namespace totalrecruit
{
namespace
{
void (*g_changeMouseCursor_orig)(ForgottenGUI*, CursorType, const hand&, const hand&) = NULL;

// Resolving the target costs a rules evaluation, and the cursor is recomputed
// constantly, so the last answer is remembered and only recomputed when the
// target actually changes.
Character* g_lastTarget = NULL;
bool g_lastWasRecruitable = false;

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

    const Verdict verdict = Evaluate(npc);

    // Someone who will join, or whose refusal names something the player can
    // do about it. A flat "they are dead" is not worth a cursor.
    if (verdict.willJoin)
    {
        return true;
    }

    return verdict.currency == Currency_Care ||
           verdict.currency == Currency_Cats ||
           verdict.currency == Currency_Submission ||
           verdict.currency == Currency_Freedom;
}

// The hand parameters are references, and the compiler counts those as objects,
// so the guarded read lives in its own POD-only frame.
bool CachedWorthTalkingTo(const hand& target)
{
    Character* npc = NULL;
    if (!SafeResolveTarget(target, &npc))
    {
        g_lastTarget = NULL;
        g_lastWasRecruitable = false;
        return false;
    }

    if (npc != g_lastTarget)
    {
        g_lastTarget = npc;
        g_lastWasRecruitable = WorthTalkingTo(npc);
    }
    return g_lastWasRecruitable;
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
    return true;
}

void ShutdownCursorHint()
{
    g_lastTarget = NULL;
    g_lastWasRecruitable = false;
}
} // namespace totalrecruit
