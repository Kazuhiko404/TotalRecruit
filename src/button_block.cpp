#include "button_block.h"
#include "menu_obstacles.h"
#include "log.h"

namespace totalrecruit
{
namespace
{
bool SafeApply(MyGUI::Button* button, const menuslot::Rect& rect)
{
    TR_TRY
    {
        button->setCoord(rect.left, rect.top, rect.width, rect.height);
        button->setVisible(true);
        return true;
    }
    TR_CATCH { return false; }
}

// Split in two: building the std::string name needs unwinding, which cannot
// share a frame with __try.
MyGUI::Button* CreateWidget(MyGUI::Gui* guiManager, const char* skin, const char* name)
{
    return guiManager->createWidget<MyGUI::Button>(
        skin,
        MyGUI::IntCoord(0, 0, kMinButtonWidth, kButtonHeight),
        MyGUI::Align::Default,
        kButtonLayer,
        name);
}

MyGUI::Button* CreateWithSkin(MyGUI::Gui* guiManager, const char* skin, const char* name)
{
    TR_TRY { return CreateWidget(guiManager, skin, name); }
    TR_CATCH { return NULL; }
}
} // namespace

BlockStyle MakeBlockStyle(int menuWidth)
{
    BlockStyle style;
    style.width = (menuWidth < kMinButtonWidth) ? menuWidth : kMinButtonWidth;
    style.height = kButtonHeight;
    style.gapX = kButtonGapX;
    style.gapY = kButtonGapY;
    return style;
}

MyGUI::Button* CreateContextButton(const char* name)
{
    MyGUI::Gui* guiManager = MyGUI::Gui::getInstancePtr();
    if (guiManager == NULL)
    {
        return NULL;
    }
    MyGUI::Button* button = CreateWithSkin(guiManager, kPreferredSkin, name);
    if (button == NULL)
    {
        button = CreateWithSkin(guiManager, kFallbackSkin, name);
    }
    if (button != NULL && !StyleContextButton(button))
    {
        LogError(std::string("Could not style context button ") + name + ".");
    }
    return button;
}

bool PlaceButtonBlock(const MyGUI::IntCoord& menuCoord,
                      const std::vector<MyGUI::Button*>& buttons,
                      const BlockStyle& style,
                      menuslot::Side* ioSide)
{
    if (buttons.empty())
    {
        return false;
    }

    menuslot::Request request;
    request.menu = menuslot::Rect(menuCoord.left, menuCoord.top,
                                  menuCoord.width, menuCoord.height);
    if (!ReadScreenSize(&request.screenWidth, &request.screenHeight))
    {
        return false;
    }
    request.buttonCount = static_cast<int>(buttons.size());
    request.buttonWidth = style.width;
    request.buttonHeight = style.height;
    request.gapX = style.gapX;
    request.gapY = style.gapY;
    request.previousSide = (ioSide != NULL) ? *ioSide : menuslot::SIDE_NONE;
    CollectObstacles(request.menu, &request.obstacles);

    const menuslot::Placement placement = menuslot::ResolveSlot(request);
    if (placement.rects.size() != buttons.size())
    {
        return false;
    }

    bool ok = true;
    for (size_t i = 0; i < buttons.size(); ++i)
    {
        ok = SafeApply(buttons[i], placement.rects[i]) && ok;
    }
    if (ioSide != NULL)
    {
        *ioSide = placement.side;
    }
    return ok;
}
} // namespace totalrecruit
