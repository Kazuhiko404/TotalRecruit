#pragma once

// Port of TotalLoot/src/menu_obstacles: the engine-side half of the free-slot
// placement. It only reads MyGUI; the geometry decision lives in menu_slot.

#include "common.h"
#include "menu_slot.h"

namespace totalrecruit
{
// Screen size from MyGUI's render manager. False when unavailable.
bool ReadScreenSize(int* outWidth, int* outHeight);

// Rects of visible foreign root widgets (other mods' context-menu buttons)
// that the Hablar button must not cover. Scans every MyGUI root widget and also
// looks up the known overlay buttons by name. Skips our own "TR_" widgets and
// anything IsPlausibleObstacle rejects (the native menu, HUD roots).
void CollectObstacles(const menuslot::Rect& menu, std::vector<menuslot::Rect>* out);
} // namespace totalrecruit
