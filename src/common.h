#pragma once

// Shared includes and small helpers for TotalRecruit.
//
// Include order matters here. Ogre defines Ogre::min/max as functions taking
// const float&, which the min/max macros in <Windows.h> destroy, so the engine
// headers must be seen first. NOMINMAX keeps them suppressed regardless.

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <string>
#include <vector>

#include <Debug.h>
#include <core/Functions.h>

#include <kenshi/Kenshi.h>
#include <kenshi/Globals.h>
#include <kenshi/GameWorld.h>
#include <kenshi/PlayerInterface.h>
#include <kenshi/Character.h>
#include <kenshi/Faction.h>
// Faction.h only forward-declares FactionRelations, which character_facts reads
// for the player's standing (ally tiers, refusals by relation).
#include <kenshi/FactionRelations.h>
#include <kenshi/Platoon.h>
#include <kenshi/GameData.h>
#include <kenshi/RootObject.h>
// ContextMenu is declared in full inside PlayerInterface.h; including
// kenshi/gui/ContextMenu.h as well is a redefinition of the same class.
#include <kenshi/Enums.h>
// The rule engine reads personality and archetype from the character's
// StateBroadcastData, and dying/hunger state from its MedicalSystem.
#include <kenshi/StateBroadcastData.h>
#include <kenshi/MedicalSystem.h>
// Cage ownership resolves through Building; the dialogue bridge needs Dialogue.
// Both are only forward-declared by the headers above.
#include <kenshi/Dialogue.h>
#include <kenshi/util/hand.h>
#include <kenshi/util/lektor.h>

// MyGUI_DelegateImplement.h is an internal header the public ones expand
// several times with different macros; including it directly breaks the build.
#include <mygui/MyGUI_Button.h>
#include <mygui/MyGUI_EditBox.h>
#include <mygui/MyGUI_Gui.h>
#include <mygui/MyGUI_ListBox.h>
#include <mygui/MyGUI_Macros.h>
#include <mygui/MyGUI_MouseButton.h>
#include <mygui/MyGUI_RenderManager.h>
#include <mygui/MyGUI_TextBox.h>
#include <mygui/MyGUI_Widget.h>
#include <mygui/MyGUI_Window.h>

#include <Windows.h>

namespace totalrecruit
{
// The only Kenshi build whose reversed addresses this plugin is validated
// against. Hooking a different build would resolve to the wrong functions.
const char* const kSupportedGameVersion = "1.0.65";
} // namespace totalrecruit

// Reading a RootObject* the engine already freed access-violates rather than
// returning null, so every dereference of engine-owned memory is guarded.
#define TR_TRY __try
#define TR_CATCH __except (EXCEPTION_EXECUTE_HANDLER)
