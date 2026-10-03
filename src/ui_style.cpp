#include "common.h"
#include "ui_style.h"
#include "config.h"
#include "log.h"

#pragma warning(push)
#pragma warning(disable: 4091)
#include <ogre/OgreResourceGroupManager.h>
#include <mygui/MyGUI_ImageBox.h>
#pragma warning(pop)

namespace totalrecruit
{
namespace ui
{
// Tokens of ui-mockup/css/base.css.
extern const MyGUI::Colour kLacquer(20.f / 255.f, 17.f / 255.f, 16.f / 255.f, 1.f);
extern const MyGUI::Colour kBgDark(30.f / 255.f, 27.f / 255.f, 24.f / 255.f, 1.f);
extern const MyGUI::Colour kSumi(42.f / 255.f, 37.f / 255.f, 33.f / 255.f, 1.f);
extern const MyGUI::Colour kLine(58.f / 255.f, 51.f / 255.f, 44.f / 255.f, 1.f);
extern const MyGUI::Colour kText(238.f / 255.f, 228.f / 255.f, 210.f / 255.f, 1.f);
extern const MyGUI::Colour kBrass(214.f / 255.f, 176.f / 255.f, 116.f / 255.f, 1.f);
extern const MyGUI::Colour kGoldDim(150.f / 255.f, 120.f / 255.f, 78.f / 255.f, 1.f);
extern const MyGUI::Colour kMuted(162.f / 255.f, 150.f / 255.f, 132.f / 255.f, 1.f);
extern const MyGUI::Colour kAmber(236.f / 255.f, 184.f / 255.f, 96.f / 255.f, 1.f);
extern const MyGUI::Colour kDanger(206.f / 255.f, 74.f / 255.f, 58.f / 255.f, 1.f);
extern const MyGUI::Colour kReady(124.f / 255.f, 172.f / 255.f, 116.f / 255.f, 1.f);
extern const MyGUI::Colour kIndigo(108.f / 255.f, 146.f / 255.f, 206.f / 255.f, 1.f);
extern const MyGUI::Colour kSoftDanger(240.f / 255.f, 138.f / 255.f, 124.f / 255.f, 1.f);

extern const char* const kBodyFont = "Kenshi_StandardFont_MediumSmall";
extern const char* const kMediumFont = "Kenshi_StandardFont_Medium";
extern const char* const kSmallFont = "Kenshi_StandardFont_Small";
extern const char* const kNameFont = "Kenshi_PaintedTextFont_Small";

namespace
{
const char* const kTextSkin = "Kenshi_TextboxStandardText";
const char* const kFlatSkin = "Kenshi_TextBoxEmptySkin";
const char* const kFillTexture = "tr_white.png";

bool g_resourcesTried = false;
bool g_resourcesReady = false;
} // namespace

// <mod dir>\gui\images, mapped into Ogre's GUI resource group the way TotalSparring
// and Shutori do (their ui_resources.cpp / ArenaUI.cpp). Optional: without the PNG
// the fills fall back to the stock WhiteSkin.
bool EnsureResources()
{
    if (g_resourcesTried)
    {
        return g_resourcesReady;
    }
    g_resourcesTried = true;

    const std::string modDir = GetModDirectory();
    if (modDir.empty())
    {
        return false;
    }
    const std::string imagesDir = modDir + "\\gui\\images";
    const std::string probe = imagesDir + "\\" + kFillTexture;
    if (GetFileAttributesA(probe.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        LogInfo("[ui] no gui/images/tr_white.png in the mod folder; using the stock WhiteSkin for fills.");
        return false;
    }

    try
    {
        Ogre::ResourceGroupManager& rgm = Ogre::ResourceGroupManager::getSingleton();
        if (!rgm.resourceExists("GUI", kFillTexture))
        {
            rgm.addResourceLocation(imagesDir, "FileSystem", "GUI", false);
            // Without this Ogre has not indexed the folder and setImageTexture
            // resolves nothing (the box stays blank without any error).
            rgm.initialiseResourceGroup("GUI");
        }
        g_resourcesReady = true;
        LogInfo("[ui] fills use " + probe);
    }
    catch (...)
    {
        LogError("[ui] could not register the GUI image folder; using the stock WhiteSkin for fills.");
    }
    return g_resourcesReady;
}

MyGUI::Widget* MakeFill(MyGUI::Widget* parent, const MyGUI::Colour& colour)
{
    MyGUI::Widget* fill = NULL;
    if (EnsureResources())
    {
        MyGUI::ImageBox* box = parent->createWidget<MyGUI::ImageBox>(
            "ImageBox", 0, 0, 1, 1, MyGUI::Align::Default, "");
        if (box != NULL)
        {
            box->setImageTexture(kFillTexture);
            fill = box;
        }
    }
    if (fill == NULL)
    {
        fill = parent->createWidget<MyGUI::Widget>(
            "WhiteSkin", MyGUI::IntCoord(0, 0, 1, 1), MyGUI::Align::Default);
    }
    if (fill != NULL)
    {
        fill->setNeedMouseFocus(false);
        fill->setNeedKeyFocus(false);
        fill->setColour(colour);
        fill->setVisible(false);
    }
    return fill;
}

MyGUI::TextBox* MakeLabel(MyGUI::Widget* parent, const char* font, const MyGUI::Colour& colour)
{
    MyGUI::TextBox* text = parent->createWidget<MyGUI::TextBox>(
        kTextSkin, 0, 0, 1, 1, MyGUI::Align::Default, "");
    if (text != NULL)
    {
        text->setNeedMouseFocus(false);
        text->setNeedKeyFocus(false);
        text->setFontName(font);
        text->setTextColour(colour);
        text->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
        text->setVisible(false);
    }
    return text;
}

// TotalSparring duel_popup CreateReason, without its plate: the caller creates the
// plate with the other fills so it is drawn first.
MyGUI::EditBox* MakeWrapText(MyGUI::Widget* parent, const char* name, const char* font,
                             const MyGUI::Colour& colour)
{
    const char* skins[3] = { "Kenshi_EditBoxEmptySkin", "Kenshi_EditBox", "EditBox" };
    MyGUI::EditBox* box = NULL;
    for (int i = 0; i < 3 && box == NULL; ++i)
    {
        box = parent->createWidget<MyGUI::EditBox>(
            skins[i], MyGUI::IntCoord(0, 0, 10, 10), MyGUI::Align::Default, name);
    }
    if (box == NULL)
    {
        return NULL;
    }
    box->setEditStatic(true);
    box->setEditReadOnly(true);
    box->setEditMultiLine(true);
    box->setEditWordWrap(true);
    box->setNeedKeyFocus(false);
    box->setNeedMouseFocus(false);
    box->setVisibleVScroll(false);
    box->setVisibleHScroll(false);
    box->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Top);
    box->setFontName(font);
    box->setTextColour(colour);
    box->setTextShadow(true);
    box->setTextShadowColour(MyGUI::Colour::Black);
    box->setVisible(false);
    return box;
}

// Width first: wrapping depends on it. setOnlyText skips the caption's markup
// parsing, which would swallow stray formatting characters in a name.
int MeasureWrapText(MyGUI::EditBox* box, const std::string& text, int width)
{
    if (box == NULL)
    {
        return 0;
    }
    box->setSize(width, box->getHeight() > 0 ? box->getHeight() : 10);
    box->setOnlyText(text);
    return box->getTextSize().height;
}

void Place(MyGUI::Widget* w, int x, int y, int width, int height)
{
    if (w == NULL) return;
    w->setCoord(x, y, width, height);
    w->setVisible(true);
}

void Hide(MyGUI::Widget* w)
{
    if (w != NULL) w->setVisible(false);
}

void PlaceFrame(MyGUI::Widget* const parts[4], int x, int y, int w, int h)
{
    Place(parts[0], x, y, w, 1);
    Place(parts[1], x, y + h - 1, w, 1);
    Place(parts[2], x, y, 1, h);
    Place(parts[3], x + w - 1, y, 1, h);
}

void MakeFlatButtonPlates(MyGUI::Widget* parent, const MyGUI::Colour& plate,
                          const MyGUI::Colour* edge, FlatButton* out)
{
    out->edge = (edge != NULL) ? MakeFill(parent, *edge) : NULL;
    out->plate = MakeFill(parent, plate);
    out->button = NULL;
}

void MakeFlatButtonCaption(MyGUI::Widget* parent, const char* name, const char* font,
                           const MyGUI::Colour& ink, FlatButton* out)
{
    MyGUI::Button* button = parent->createWidget<MyGUI::Button>(
        kFlatSkin, MyGUI::IntCoord(0, 0, 10, 10), MyGUI::Align::Default, name);
    if (button != NULL)
    {
        button->setFontName(font);
        button->setTextAlign(MyGUI::Align::Center);
        button->setTextColour(ink);
        button->setNeedMouseFocus(true);
        button->setVisible(false);
    }
    out->button = button;
}

void PlaceButton(const FlatButton& b, int x, int y, int w, int h)
{
    if (b.edge != NULL)
    {
        Place(b.edge, x, y, w, h);
        Place(b.plate, x + 1, y + 1, w - 2, h - 2);
    }
    else
    {
        Place(b.plate, x, y, w, h);
    }
    Place(b.button, x, y, w, h);
}

void ShowButton(const FlatButton& b, bool visible)
{
    if (b.edge != NULL) b.edge->setVisible(visible);
    if (b.plate != NULL) b.plate->setVisible(visible);
    if (b.button != NULL) b.button->setVisible(visible);
}

void SetButtonCaption(const FlatButton& b, const std::string& caption)
{
    if (b.button != NULL) b.button->setCaption(caption);
}

void SafeSetVisible(MyGUI::Widget* widget, bool visible)
{
    if (widget == NULL) return;
    TR_TRY { widget->setVisible(visible); }
    TR_CATCH { }
}
} // namespace ui
} // namespace totalrecruit
