#include "native_windows.h"

#include "common.h"
#include "config.h"
#include "log.h"

#include <kenshi/gui/ForgottenGUI.h>
#include <kenshi/gui/DialogueWindow.h>

#include <cstdio>

namespace totalrecruit
{
namespace
{
// The last answer, for the probe's change log.
bool g_lastCovered = false;

// POD-only frame (C2712): the guard builds no C++ object.
bool ReadUnguarded(NativeWindowReading* out)
{
    TR_TRY
    {
        if (gui == NULL)
        {
            return false;
        }
        out->inDialogue = gui->inDialogue();
        out->modalMessage = gui->hasModalMessage();
        // getVisible is the non-virtual accessor (isVisible is the virtual one and
        // must not be called from plugin code; KENSHI-MODDING-NOTES.md section 11).
        DialogueWindow* window = gui->dialogue;
        out->dialogueWindowVisible = (window != NULL) && window->getVisible();
        return true;
    }
    TR_CATCH { return false; }
}

void LogChange(const NativeWindowReading& reading, bool covered)
{
    if (covered == g_lastCovered)
    {
        return;
    }
    g_lastCovered = covered;
    if (!GetConfig().probeOnClick)
    {
        return;
    }
    if (!covered)
    {
        LogInfo("[native-window] closed");
        return;
    }
    char buf[96] = { 0 };
    _snprintf_s(buf, sizeof(buf), _TRUNCATE, "open: dialogue=%d window=%d modal=%d",
                reading.inDialogue ? 1 : 0, reading.dialogueWindowVisible ? 1 : 0,
                reading.modalMessage ? 1 : 0);
    LogInfo(std::string("[native-window] ") + buf);
}
} // namespace

NativeWindowReading ReadNativeWindows()
{
    NativeWindowReading reading;
    if (!ReadUnguarded(&reading))
    {
        return NativeWindowReading();
    }
    reading.read = true;
    return reading;
}

bool NativeWindowCoversOverlays()
{
    const NativeWindowReading reading = ReadNativeWindows();
    const bool covered = OverlaysYieldToNativeWindow(reading);
    LogChange(reading, covered);
    return covered;
}
} // namespace totalrecruit
