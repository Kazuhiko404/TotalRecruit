// TotalRecruit - universal, non-cheat recruitment for Kenshi.
//
// Every other recruitment mod attaches an FCS dialogue package to each NPC, so
// characters added by other mods are unreachable and the author ends up
// maintaining a patch per mod. A plugin does not ask "does this NPC have the
// dialogue installed?" -- it asks "is this a Character?", which covers mods
// that do not exist yet.
//
// Driven by hooks rather than polling:
//   * ContextMenu hooks anchor a "Recruit" button to Kenshi's own right-click
//     menu (see recruit_overlay.cpp for why rows are not injected natively).
//   * MedicalSystem hooks notice when the player saves a dying character.
//   * A Dialogue hook reads which reply the player chose.
//   * A GUI-update hook completes recruits a frame after the conversation ends.
//   * An InputHandler hook opens the spawn panel on Shift+K.

#include "common.h"
#include "config.h"
#include "log.h"
#include "recruit_core.h"
#include "recruit_overlay.h"
#include "dialogue_bridge.h"
#include "rescue.h"
#include "frame_tick.h"
#include "settings_panel.h"
#include "offer_popup.h"
#include "cursor_hint.h"
#include "hail.h"
#include "spawn_panel.h"



#include <kenshi/InputHandler.h>
#include <ois/OISKeyboard.h>

namespace totalrecruit
{
// Set by DllMain before startPlugin runs, so it cannot live in an anonymous
// namespace.
HMODULE g_module = NULL;

namespace
{
void (*g_keyDownEvent_orig)(InputHandler*, OIS::KeyCode) = NULL;

// Directory the DLL was loaded from, used to find config.json.
std::string ModuleDirectory()
{
    if (g_module == NULL)
    {
        return std::string();
    }

    char path[MAX_PATH] = { 0 };
    if (GetModuleFileNameA(g_module, path, MAX_PATH) == 0)
    {
        return std::string();
    }

    std::string full(path);
    const size_t slash = full.find_last_of("\\/");
    if (slash == std::string::npos)
    {
        return std::string();
    }
    return full.substr(0, slash);
}

void InputHandler_keyDownEvent_hook(InputHandler* thisptr, OIS::KeyCode key)
{
    if (g_keyDownEvent_orig != NULL)
    {
        g_keyDownEvent_orig(thisptr, key);
    }

    const Config& config = GetConfig();
    if (!config.enabled || config.panelToggleKey == 0)
    {
        return;
    }
    if ((int)key != config.panelToggleKey)
    {
        return;
    }

    // Shift+K opens the spawn panel, Ctrl+K the settings. Plain K is left to
    // the game and to other mods.
    bool shiftHeld = false;
    bool ctrlHeld = false;
    TR_TRY
    {
        shiftHeld = (thisptr != NULL) && thisptr->shift;
        ctrlHeld = (thisptr != NULL) && thisptr->ctrl;
    }
    TR_CATCH { return; }

    if (ctrlHeld)
    {
        ToggleSettingsPanel();
    }
    else if (shiftHeld)
    {
        ToggleSpawnPanel();
    }
}

bool InstallInputHook()
{
    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&InputHandler::keyDownEvent),
            InputHandler_keyDownEvent_hook,
            &g_keyDownEvent_orig))
    {
        LogError("Failed to hook InputHandler::keyDownEvent.");
        return false;
    }
    return true;
}
} // namespace
} // namespace totalrecruit

__declspec(dllexport) void startPlugin()
{
    using namespace totalrecruit;

    // First line of the log, before anything can fail, so a silent plugin is
    // always distinguishable from one that never loaded at all.
    LogInfo("startPlugin called.");

    // Addresses are reversed from one build. A mismatch is reported but not
    // fatal: RE_Kenshi runs a patched binary whose reported version can differ,
    // and refusing outright made the plugin silently do nothing.
    KenshiLib::BinaryVersion versionInfo = KenshiLib::GetKenshiVersion();
    const std::string version = versionInfo.GetVersion();
    LogInfo("Detected Kenshi version: '" + version + "' (" +
            versionInfo.GetPlatformStr() + ").");

    if (version != kSupportedGameVersion)
    {
        LogError("Version is not " + std::string(kSupportedGameVersion) +
                 "; hooks may resolve to the wrong functions. Continuing anyway.");
    }

    LoadConfig(ModuleDirectory());
    LogInfo("Config loaded from: " + ModuleDirectory());

    if (!GetConfig().enabled)
    {
        LogInfo("Disabled by config; no hooks installed.");
        return;
    }

    const bool overlayOk = InstallRecruitOverlay();
    LogInfo(overlayOk ? "Context-menu hooks installed."
                      : "Context-menu hooks FAILED.");

    const bool inputOk = InstallInputHook();
    LogInfo(inputOk ? "Keyboard hook installed." : "Keyboard hook FAILED.");

    // Reads which dialogue line the player chose. There is no callback API for
    // this; hooking _doActions is the only route.
    const bool dialogueOk = InstallDialogueBridge();

    // Route A. Also installs the per-frame tick that completes recruits the
    // player accepted in dialogue, which cannot happen inside the hook itself.
    const bool rescueOk = InstallRescueHooks();
    const bool tickOk = InstallFrameTick();

    // Makes the mod visible: the cursor tells the player who can be talked to.
    const bool cursorOk = InstallCursorHint();
    InstallHailing();

    if (!overlayOk && !inputOk)
    {
        LogError("No hooks installed; the mod will do nothing.");
        return;
    }

    LogInfo(std::string("Subsystems: dialogue=") + (dialogueOk ? "on" : "off") +
            " rescue=" + (rescueOk ? "on" : "off") +
            " tick=" + (tickOk ? "on" : "off") +
            " cursor=" + (cursorOk ? "on" : "off") + ".");
    LogInfo("Ready. Right-click an NPC to recruit; Ctrl+K settings, Shift+K spawn panel.");
}

__declspec(dllexport) void stopPlugin()
{
    using namespace totalrecruit;
    ShutdownHailing();
    ShutdownCursorHint();
    ShutdownOfferPopup();
    ShutdownSettingsPanel();
    ShutdownFrameTick();
    ShutdownRescueHooks();
    ShutdownDialogueBridge();
    ShutdownRecruitOverlay();
    ShutdownSpawnPanel();
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        totalrecruit::g_module = module;
        DisableThreadLibraryCalls(module);
        break;
    case DLL_PROCESS_DETACH:
        // Only clean up on an explicit unload; on process exit the engine's
        // widgets are already gone.
        if (reserved == NULL)
        {
            stopPlugin();
        }
        break;
    default:
        break;
    }
    return TRUE;
}
