#include "overlay_visibility_rules.h"

namespace totalrecruit
{
bool OverlaysYieldToNativeWindow(const NativeWindowReading& reading)
{
    if (!reading.read)
    {
        return false;
    }
    return reading.inDialogue || reading.dialogueWindowVisible || reading.modalMessage;
}
} // namespace totalrecruit
