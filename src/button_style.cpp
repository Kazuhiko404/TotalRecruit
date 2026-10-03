#include "button_style.h"

#include <mygui/MyGUI_Colour.h>

namespace totalrecruit
{
namespace
{
// TotalSparring palette (src/shared/ui/ui_palette.h): kBrass, kAmber, kBgDark.
// Tints multiply against the skin, so the values are copied untouched.
MyGUI::Colour Brass() { return MyGUI::Colour(214.f / 255.f, 176.f / 255.f, 116.f / 255.f, 1.f); }
MyGUI::Colour Amber() { return MyGUI::Colour(236.f / 255.f, 184.f / 255.f, 96.f / 255.f, 1.f); }
MyGUI::Colour BgDark() { return MyGUI::Colour(30.f / 255.f, 27.f / 255.f, 24.f / 255.f, 1.f); }

void OnButtonHover(MyGUI::Widget* sender, MyGUI::Widget* /*old*/)
{
    sender->setColour(Amber());
    static_cast<MyGUI::Button*>(sender)->setTextColour(BgDark());
}

void OnButtonLeave(MyGUI::Widget* sender, MyGUI::Widget* /*next*/)
{
    sender->setColour(Brass());
    static_cast<MyGUI::Button*>(sender)->setTextColour(Amber());
}

// Colour temporaries and delegates need unwinding, so the guarded frame is
// separate from the code that builds them.
void ApplyStyle(MyGUI::Button* button)
{
    button->setFontHeight(kButtonFontHeight);
    button->setTextColour(Amber());
    button->setTextShadow(true);
    button->setTextShadowColour(MyGUI::Colour::Black);
    button->setColour(Brass());
    button->eventMouseSetFocus += MyGUI::newDelegate(&OnButtonHover);
    button->eventMouseLostFocus += MyGUI::newDelegate(&OnButtonLeave);
}
} // namespace

bool StyleContextButton(MyGUI::Button* button)
{
    if (button == NULL)
    {
        return false;
    }
    TR_TRY { ApplyStyle(button); return true; }
    TR_CATCH { return false; }
}
} // namespace totalrecruit
