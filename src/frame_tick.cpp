#include "common.h"
#include "frame_tick.h"
#include "rescue.h"
#include "hail.h"
#include "head_notice.h"
#include "mod_hub_integration.h"
#include "readout.h"
#include "cage_commands.h"
#include "submission_director.h"
#include "temp_member.h"
#include "helper.h"
#include "log.h"

#include <kenshi/gui/ForgottenGUI.h>

namespace totalrecruit
{
namespace
{
void (*g_forgottenGuiUpdate_orig)(ForgottenGUI*) = NULL;

// Kept out of the hook body so a fault in the plugin's own work can never take
// the game's GUI update down with it.
void SafeTick()
{
    // HeadNoticeTick after ReadoutTick: it reads where the normal card is this frame.
    TR_TRY { RescueTick(); HailTick(); ReadoutTick(); HeadNoticeTick(); TickCageCommands(); SubmissionTick(); HelperTick(); TempMemberTick(); ModHubTick(); }
    TR_CATCH { }
}

void ForgottenGUI_update_hook(ForgottenGUI* thisptr)
{
    if (g_forgottenGuiUpdate_orig != NULL)
    {
        g_forgottenGuiUpdate_orig(thisptr);
    }

    SafeTick();
}
} // namespace

bool InstallFrameTick()
{
    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&ForgottenGUI::update),
            ForgottenGUI_update_hook, &g_forgottenGuiUpdate_orig))
    {
        LogError("Could not hook ForgottenGUI::update; dialogue recruits will not complete.");
        return false;
    }

    LogInfo("Frame tick installed.");
    return true;
}

void ShutdownFrameTick()
{
    // The head notices only live through the frame tick, so their widgets go with it.
    ShutdownHeadNotices();
}
} // namespace totalrecruit
