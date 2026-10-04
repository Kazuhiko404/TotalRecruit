#include "preset_rules.h"
#include "config.h"

namespace totalrecruit
{
bool IsKnownPreset(int raw)
{
    return raw == Preset_Cheat || raw == Preset_Balanced;
}

int ClampPreset(int raw)
{
    return IsKnownPreset(raw) ? raw : Preset_Balanced;
}
} // namespace totalrecruit
