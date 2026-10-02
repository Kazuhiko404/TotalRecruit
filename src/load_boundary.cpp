#include "common.h"
#include "load_boundary.h"
#include "character_identity.h"
#include "cursor_hint.h"
#include "defeats.h"
#include "dialogue_bridge.h"
#include "hail.h"
#include "log.h"
#include "mercy.h"
#include "offer_popup.h"
#include "recruit_overlay.h"
#include "readout.h"
#include "hold_tracking.h"
#include "insistence.h"

#pragma warning(push)
#pragma warning(disable: 4091)
#include <kenshi/SaveManager.h>
#pragma warning(pop)

namespace totalrecruit
{
namespace
{
int (*g_loadGameOriginal)(SaveManager*, const std::string&, const std::string&) = NULL;
int (*g_importGameOriginal)(SaveManager*, const std::string&, const std::string&, int) = NULL;

// KenshiLib chains detours: each one calls the original it was given exactly
// once, with the arguments untouched.
int LoadGameHook(SaveManager* self, const std::string& location, const std::string& name)
{
    LoadBoundary::ResetSessionState();
    return g_loadGameOriginal(self, location, name);
}

int ImportGameHook(SaveManager* self, const std::string& location, const std::string& name, int flags)
{
    LoadBoundary::ResetSessionState();
    return g_importGameOriginal(self, location, name, flags);
}
} // namespace

namespace LoadBoundary
{
void ResetSessionState()
{
    // UI and raw pointers first: they refer to characters the load will free.
    HideOffer();
    ForgetOverlayTarget();
    ForgetReadout();
    ForgetCursorTarget();
    ForgetConversation();

    // The per-character tables (all keyed by CharKey).
    ForgetAllMercy();
    ForgetAllHails();
    ResetHoldClock();
    ResetTalkStates();
    ForgetAllDefeats();

    // Last: the ids the keys above were made from.
    CharacterIdentity::AbandonWorldState();
    LogInfo("[identity] load begin: session state cleared (offer window, overlay, readout and cursor targets, "
            "conversation, mercy, hails, hold clock, insistence, defeats, identity registry).");
}

bool Install()
{
    const bool loadOk = KenshiLib::SUCCESS == KenshiLib::AddHook(
        KenshiLib::GetRealAddress(&SaveManager::loadGame), LoadGameHook, &g_loadGameOriginal);
    const bool importOk = KenshiLib::SUCCESS == KenshiLib::AddHook(
        KenshiLib::GetRealAddress(&SaveManager::importGame), ImportGameHook, &g_importGameOriginal);
    if (loadOk && importOk)
        LogInfo("[identity] Hooked SaveManager::loadGame / importGame (session reset at load begin).");
    else
        LogError("[identity] SaveManager::loadGame / importGame hook FAILED; session state is not reset at a load.");
    return loadOk && importOk;
}
} // namespace LoadBoundary
} // namespace totalrecruit
