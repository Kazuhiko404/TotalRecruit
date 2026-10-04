#include "common.h"
#include "load_boundary.h"
#include "character_identity.h"
#include "cursor_hint.h"
#include "defeats.h"
#include "dialogue_bridge.h"
#include "hail.h"
#include "head_notice.h"
#include "log.h"
#include "mercy.h"
#include "offer_popup.h"
#include "recruit_overlay.h"
#include "readout.h"
#include "hold_tracking.h"
#include "insistence.h"
#include "haggle_ledger.h"
#include "cage_commands.h"
#include "beating_stress.h"
#include "feeding_ledger.h"
#include "submission_director.h"
#include "submission_tracker.h"
#include "temp_member.h"
#include "helper.h"
#include "leave_homing.h"
#include "duel_challenge_rules.h"
#include "ts_bridge.h"

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
    // The NPC of a submission in progress gets its pace and routine back while
    // it can still be read.
    ForgetSubmission();
    // The helper offer in progress likewise, then the helpers' pointers and table
    // (each helper's load hook puts its contract back).
    ForgetHelpers();
    ForgetOverlayTarget();
    ForgetReadout();
    ForgetHeadNotices();
    ForgetCursorTarget();
    ForgetConversation();
    ForgetCageCommands();

    // The per-character tables (all keyed by CharKey).
    ForgetAllMercy();
    ForgetAllHails();
    ResetHoldClock();
    ResetTalkStates();
    ForgetAllDefeats();
    ForgetAllBeatingStress();
    ForgetAllPlayerMeals();
    ForgetAllSubmissionMemory();
    ForgetAllTempMembers();
    ForgetLeaveHoming();
    // Pending duel challenges and lost-duel cooldowns (each NPC's load hook puts its
    // cooldown back), and bouts TotalSparring reported for the abandoned world.
    ForgetAllDuelChallenges();
    ForgetTsBoutEvents();
    SessionHaggleLedger().Clear();

    // Last: the ids the keys above were made from.
    CharacterIdentity::AbandonWorldState();
    LogInfo("[identity] load begin: session state cleared (offer window, overlay, readout and cursor targets, "
            "conversation, cage command watch and credits, submission event, mercy, hails, hold clock, "
            "insistence, defeats, meals from the player, submission asks and reject cooldowns, temporary "
            "members, duel challenges and cooldowns, queued TotalSparring bouts, haggle offences, "
            "identity registry).");
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
