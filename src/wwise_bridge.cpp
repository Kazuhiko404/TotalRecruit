#include "common.h"
#include "wwise_bridge.h"

namespace totalrecruit
{
namespace
{
// The exported signatures (kenshi_x64.exe, Wwise 2013-era API).
typedef unsigned long (*PostEventFn)(const char* eventName, unsigned __int64 gameObject,
                                     unsigned long flags, void* callback, void* cookie,
                                     unsigned long externals, void* externalSources,
                                     unsigned long playingId);
typedef int (*SetSwitchFn)(const char* group, const char* value, unsigned __int64 gameObject);

const char* const kPostEventExport =
    "?PostEvent@SoundEngine@AK@@YAKPEBD_KKP6AXW4AkCallbackType@@PEAUAkCallbackInfo@@@ZPEAXKPEAUAkExternalSourceInfo@@K@Z";
const char* const kSetSwitchExport = "?SetSwitch@SoundEngine@AK@@YA?AW4AKRESULT@@PEBD0_K@Z";
const int kAkSuccess = 1;

bool g_resolved = false;
PostEventFn g_postEvent = NULL;
SetSwitchFn g_setSwitch = NULL;

void Resolve()
{
    if (g_resolved)
    {
        return;
    }
    g_resolved = true;
    HMODULE game = GetModuleHandleA(NULL);
    if (game == NULL)
    {
        return;
    }
    g_postEvent = reinterpret_cast<PostEventFn>(GetProcAddress(game, kPostEventExport));
    g_setSwitch = reinterpret_cast<SetSwitchFn>(GetProcAddress(game, kSetSwitchExport));
}
} // namespace

bool SafeAudioObject(Character* character, unsigned __int64* out)
{
    if (character == NULL || out == NULL)
    {
        return false;
    }
    TR_TRY
    {
        *out = character->getAudioObject();
        return *out != 0;
    }
    TR_CATCH { return false; }
}

bool WwisePostEventAvailable()
{
    Resolve();
    return g_postEvent != NULL;
}

bool WwiseSetSwitchAvailable()
{
    Resolve();
    return g_setSwitch != NULL;
}

unsigned long WwisePostEvent(const char* eventName, unsigned __int64 gameObject)
{
    Resolve();
    if (g_postEvent == NULL || eventName == NULL || eventName[0] == '\0' || gameObject == 0)
    {
        return 0;
    }
    TR_TRY { return g_postEvent(eventName, gameObject, 0, NULL, NULL, 0, NULL, 0); }
    TR_CATCH { return 0; }
}

bool WwiseSetSwitch(const char* group, const char* value, unsigned __int64 gameObject)
{
    Resolve();
    if (g_setSwitch == NULL || group == NULL || value == NULL || gameObject == 0)
    {
        return false;
    }
    TR_TRY { return g_setSwitch(group, value, gameObject) == kAkSuccess; }
    TR_CATCH { return false; }
}
} // namespace totalrecruit
