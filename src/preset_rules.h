#pragma once

// Which preset a config value names, without the engine.
//
// v1 ships two presets: Cheat (0) and Balanced (1). Vanilla+ (2) was retired; a
// config.json saved while it existed still holds `"preset": 2`, and any other
// out-of-range number is equally meaningless. Both load as Balanced -- the
// default -- rather than as an enum value nothing handles.

namespace totalrecruit
{
// True for a value that names a shipped preset (Preset_Cheat or Preset_Balanced).
bool IsKnownPreset(int raw);

// The preset to run for a config value: the value itself when it names a shipped
// preset, Preset_Balanced otherwise.
int ClampPreset(int raw);
} // namespace totalrecruit
