#pragma once

// Where the native context menu is, as the button block sees it.
//
// A character inside a building (a prisoner in a cage) opens two native columns
// side by side: the character's (ContextMenu::menuGUI, 0x48) and the building's
// (ContextMenu::menuGUI2, 0x50; e.g. "Jaula de Prisionero": ABRIR CON LLAVE,
// DESMANTELAR, PONER EN LIBERTAD). The block used to be anchored to the first
// column only, and menu_slot's left-first search could put it on the second. Now
// the block is placed beside the rectangle that holds both columns' option lists
// (UnionMenuRect); menu_slot itself is unchanged.
//
//   ReadMenuAnchor   engine side (menu_anchor.cpp): reads both option lists
//                    behind SEH guards and asks UnionMenuRect.
//   UnionMenuRect    PURE, inline below so tests/rules_tests can include this
//                    header without Kenshi or MyGUI (menu_anchor_tests.cpp).
//
// The first column is taken as today -- present, width > 0 -- so a menu with one
// column gives exactly the rectangle it always did. The second is only taken when
// it is present, visible and has a width: a hidden menuGUI2 left over from an
// earlier building menu must not widen the rectangle. The button width stays the
// first column's (MakeBlockStyle), never the union's. The title widgets
// (nameText, 0xF0) are not read: the option list is what the block always
// anchored to.
//
// Log (probeOnClick), once per menu open and again only if it changes:
// "[menu-anchor] gui1=(l,t,w,h) gui2=(l,t,w,h|none) union=(l,t,w,h)".

#include "menu_slot.h"

class ContextMenu;

namespace totalrecruit
{
namespace menuanchor
{
// One native column's option list, as read from the engine.
struct Column
{
    bool present;          // the GUI and its option list exist
    bool visible;          // the option list is shown (only asked of the second)
    menuslot::Rect rect;   // absolute coordinates

    Column() : present(false), visible(false), rect() {}
};

// The first column, as the block always took it.
inline bool FirstColumnUsable(const Column& column)
{
    return column.present && column.rect.width > 0;
}

// The second column, only when it is really on screen.
inline bool SecondColumnUsable(const Column& column)
{
    return column.present && column.visible && column.rect.width > 0;
}

// The rectangle the block is placed beside. False when the first column is not
// usable (no block, as before). With the second unusable the result is the first
// column exactly; otherwise the smallest rectangle holding both.
inline bool UnionMenuRect(const Column& first, const Column& second, menuslot::Rect* out)
{
    if (out == 0 || !FirstColumnUsable(first))
    {
        return false;
    }
    if (!SecondColumnUsable(second))
    {
        *out = first.rect;
        return true;
    }
    const menuslot::Rect& a = first.rect;
    const menuslot::Rect& b = second.rect;
    const int left = a.left < b.left ? a.left : b.left;
    const int top = a.top < b.top ? a.top : b.top;
    const int rightA = a.left + a.width;
    const int rightB = b.left + b.width;
    const int bottomA = a.top + a.height;
    const int bottomB = b.top + b.height;
    const int right = rightA > rightB ? rightA : rightB;
    const int bottom = bottomA > bottomB ? bottomA : bottomB;
    *out = menuslot::Rect(left, top, right - left, bottom - top);
    return true;
}
} // namespace menuanchor

// What the block needs from the open menu.
struct MenuAnchor
{
    menuanchor::Column first;    // menuGUI's option list
    menuanchor::Column second;   // menuGUI2's option list
    menuslot::Rect menu;         // UnionMenuRect of the two
};

// Reads both columns of `menu` and their union. False when the first column cannot
// be read or has no width (the block is not placed, as before). Logs the line above.
bool ReadMenuAnchor(ContextMenu* menu, MenuAnchor* out);

// A new menu opened: its anchor is logged again (probeOnClick).
void ResetMenuAnchorLog();
} // namespace totalrecruit
