#include "common.h"
#include "menu_anchor.h"
#include "config.h"
#include "log.h"

#include <cstdio>

namespace totalrecruit
{
namespace
{
// ContextMenuGUI is only forward-declared through PlayerInterface.h (its full
// header would redefine ContextMenu), so optionsList is read at its documented
// offset from kenshi/gui/ContextMenu.h:27.
const size_t kOptionsListOffset = 0xF8;

// The last line logged for the open menu; cleared when a menu opens.
bool g_logged = false;
menuslot::Rect g_loggedFirst;
bool g_loggedSecond = false;
menuslot::Rect g_loggedSecondRect;
menuslot::Rect g_loggedUnion;

// Both GUI pointers, read in a POD-only frame (C2712).
bool SafeMenuGuis(ContextMenu* menu, void** first, void** second)
{
    TR_TRY
    {
        *first = menu->menuGUI;
        *second = menu->menuGUI2;
        return true;
    }
    TR_CATCH { return false; }
}

// One column's option list: present, visible, absolute coordinates. False when the
// GUI or its list is missing or unreadable. MyGUI::IntCoord has no destructor, so
// it may live in this guarded frame.
bool SafeColumn(void* gui, bool* visible, int* left, int* top, int* width, int* height)
{
    TR_TRY
    {
        if (gui == NULL)
        {
            return false;
        }
        MyGUI::Widget* optionsList = *reinterpret_cast<MyGUI::Widget**>(
            reinterpret_cast<char*>(gui) + kOptionsListOffset);
        if (optionsList == NULL)
        {
            return false;
        }
        const MyGUI::IntCoord coord = optionsList->getAbsoluteCoord();
        *visible = optionsList->getInheritedVisible();
        *left = coord.left;
        *top = coord.top;
        *width = coord.width;
        *height = coord.height;
        return true;
    }
    TR_CATCH { return false; }
}

menuanchor::Column ReadColumn(void* gui)
{
    menuanchor::Column column;
    int left = 0;
    int top = 0;
    int width = 0;
    int height = 0;
    column.present = SafeColumn(gui, &column.visible, &left, &top, &width, &height);
    column.rect = menuslot::Rect(left, top, width, height);
    return column;
}

bool SameRect(const menuslot::Rect& a, const menuslot::Rect& b)
{
    return a.left == b.left && a.top == b.top && a.width == b.width && a.height == b.height;
}

std::string RectText(const menuslot::Rect& r)
{
    char text[64];
    _snprintf_s(text, sizeof(text), _TRUNCATE, "(%d,%d,%d,%d)", r.left, r.top, r.width, r.height);
    return text;
}

// Once per menu open, and again only when what it says changes.
void LogAnchor(const MenuAnchor& anchor)
{
    if (!GetConfig().probeOnClick)
    {
        return;
    }
    const bool second = menuanchor::SecondColumnUsable(anchor.second);
    if (g_logged && SameRect(g_loggedFirst, anchor.first.rect) && g_loggedSecond == second &&
        (!second || SameRect(g_loggedSecondRect, anchor.second.rect)) &&
        SameRect(g_loggedUnion, anchor.menu))
    {
        return;
    }
    g_logged = true;
    g_loggedFirst = anchor.first.rect;
    g_loggedSecond = second;
    g_loggedSecondRect = anchor.second.rect;
    g_loggedUnion = anchor.menu;
    LogInfo("[menu-anchor] gui1=" + RectText(anchor.first.rect) + " gui2=" +
            (second ? RectText(anchor.second.rect) : std::string("none")) + " union=" +
            RectText(anchor.menu));
}
} // namespace

bool ReadMenuAnchor(ContextMenu* menu, MenuAnchor* out)
{
    if (menu == NULL || out == NULL)
    {
        return false;
    }
    void* firstGui = NULL;
    void* secondGui = NULL;
    if (!SafeMenuGuis(menu, &firstGui, &secondGui))
    {
        return false;
    }
    out->first = ReadColumn(firstGui);
    out->second = ReadColumn(secondGui);
    if (!menuanchor::UnionMenuRect(out->first, out->second, &out->menu))
    {
        return false;
    }
    LogAnchor(*out);
    return true;
}

void ResetMenuAnchorLog()
{
    g_logged = false;
}
} // namespace totalrecruit
