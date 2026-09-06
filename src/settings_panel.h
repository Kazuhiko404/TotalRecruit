#pragma once

namespace totalrecruit
{
// The settings window, built on Kenshi's own DatapanelGUI.
//
// DatapanelGUI is native KenshiLib, not a KEP extension, so real tabs,
// checkboxes, sliders and dropdowns come for free with no third-party
// dependency -- the plugin loads with RE_Kenshi alone.
//
// Its widgets write straight through a bool*/float*/int*, so the config struct
// is the model and there is no read-back callback to keep in sync.
void ToggleSettingsPanel();
void ShutdownSettingsPanel();
} // namespace totalrecruit
