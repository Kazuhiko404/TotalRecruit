#include "common.h"
#include "readout_ui.h"
#include "config.h"
#include "log.h"

#include <cstdio>

#pragma warning(push)
#pragma warning(disable: 4091)
#include <ogre/OgreResourceGroupManager.h>
#include <mygui/MyGUI_ImageBox.h>
#pragma warning(pop)

namespace totalrecruit
{
namespace
{
// ------------------------------------------------------------------ Palette
// Tokens of ui-mockup/css/base.css (TotalSparring ui_palette.h). The foreground
// ones stay bright on purpose: see the header.
const MyGUI::Colour kLacquer(20.f / 255.f, 17.f / 255.f, 16.f / 255.f, 1.f);
const MyGUI::Colour kSumi(42.f / 255.f, 37.f / 255.f, 33.f / 255.f, 1.f);
const MyGUI::Colour kLine(58.f / 255.f, 51.f / 255.f, 44.f / 255.f, 1.f);
const MyGUI::Colour kText(238.f / 255.f, 228.f / 255.f, 210.f / 255.f, 1.f);
const MyGUI::Colour kBrass(214.f / 255.f, 176.f / 255.f, 116.f / 255.f, 1.f);
const MyGUI::Colour kGoldDim(150.f / 255.f, 120.f / 255.f, 78.f / 255.f, 1.f);
const MyGUI::Colour kMuted(162.f / 255.f, 150.f / 255.f, 132.f / 255.f, 1.f);
const MyGUI::Colour kAmber(236.f / 255.f, 184.f / 255.f, 96.f / 255.f, 1.f);
const MyGUI::Colour kDanger(206.f / 255.f, 74.f / 255.f, 58.f / 255.f, 1.f);
const MyGUI::Colour kReady(124.f / 255.f, 172.f / 255.f, 116.f / 255.f, 1.f);
const MyGUI::Colour kIndigo(108.f / 255.f, 146.f / 255.f, 206.f / 255.f, 1.f);
const MyGUI::Colour kSoftDanger(240.f / 255.f, 138.f / 255.f, 124.f / 255.f, 1.f);

// ------------------------------------------------------------------- Layout
// px values of readout.css (--card-w, --card-w-compact, --pad, --bar-h ...).
const int kCardWidth = 264;
const int kCardWidthCompact = 176;
const int kPad = 8;
const int kPadCompact = 6;
const int kBarH = 8;
const int kBarHCompact = 5;
const int kHeadH = 18;
const int kLabelH = 14;
const int kLegendH = 12;
const int kLineH = 16;
const int kAltH = 14;
const int kGap = 4;
const int kGapCompact = 3;
const int kTagW = 84;
const int kPipSize = 8;
const int kPipGap = 3;
const int kMaxPips = 10;
const int kArrowGap = 8;
const int kScreenMargin = 4;

const char* const kRootName = "TR_Readout";
const char* const kRootLayer = "ToolTip";
const char* const kRootSkin = "Kenshi_TextBoxEmptySkin";
const char* const kTextSkin = "Kenshi_TextboxStandardText";
const char* const kBodyFont = "Kenshi_StandardFont_MediumSmall";
const char* const kSmallFont = "Kenshi_StandardFont_Small";
const char* const kNameFont = "Kenshi_PaintedTextFont_Small";
const char* const kFillTexture = "tr_white.png";

// ------------------------------------------------------------------- Widgets
struct Card
{
    MyGUI::Widget* root;

    MyGUI::Widget* bg;
    MyGUI::Widget* frame[4];
    MyGUI::Widget* headLine;
    MyGUI::Widget* tagFrame[4];

    MyGUI::Widget* stressBorder;
    MyGUI::Widget* stressBg;
    MyGUI::Widget* segHunger;
    MyGUI::Widget* segWounds;
    MyGUI::Widget* segTired;
    MyGUI::Widget* swatch[3];

    MyGUI::Widget* chanceBorder;
    MyGUI::Widget* chanceBg;
    MyGUI::Widget* chanceFill;

    MyGUI::Widget* foodBorder;
    MyGUI::Widget* foodBg;
    MyGUI::Widget* foodFill;
    MyGUI::Widget* foodNotch;

    MyGUI::Widget* pip[kMaxPips];
    MyGUI::Widget* whyLine;

    MyGUI::TextBox* name;
    MyGUI::TextBox* tag;
    MyGUI::TextBox* stressLabel;
    MyGUI::TextBox* stressValue;
    MyGUI::TextBox* legend[3];
    MyGUI::TextBox* chanceLabel;
    MyGUI::TextBox* chanceValue;
    MyGUI::TextBox* foodLabel;
    MyGUI::TextBox* foodValue;
    MyGUI::TextBox* defeatsLabel;
    MyGUI::TextBox* defeatsValue;
    MyGUI::TextBox* primary;
    MyGUI::TextBox* secondary;
};

Card g_card = { 0 };
bool g_built = false;
bool g_buildFailed = false;
bool g_resourcesTried = false;
bool g_resourcesReady = false;
int g_width = 0;
int g_height = 0;
// What the widgets were last laid out for (LayoutKey). Laying out hides and
// re-captions about forty widgets, so it only runs when this changes; an
// unchanged card is only moved.
std::string g_layoutKey;

// ---------------------------------------------------------------- Resources
// <mod dir>\gui\images, mapped into Ogre GUI resource group the way TotalSparring
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
        LogInfo("[readout] no gui/images/tr_white.png in the mod folder; using the stock WhiteSkin for fills.");
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
        LogInfo("[readout] fills use " + probe);
    }
    catch (...)
    {
        LogError("[readout] could not register the GUI image folder; using the stock WhiteSkin for fills.");
    }
    return g_resourcesReady;
}

// --------------------------------------------------------------- Construction
// Everything here builds C++ temporaries (IntCoord, UString), so it is unguarded;
// the POD-frame wrappers at the bottom hold the guards (C2712).
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

bool BuildUnguarded()
{
    MyGUI::Gui* gui = MyGUI::Gui::getInstancePtr();
    if (gui == NULL)
    {
        return false;
    }

    Card& c = g_card;
    c.root = gui->createWidget<MyGUI::Widget>(
        kRootSkin, MyGUI::IntCoord(0, 0, kCardWidth, 100), MyGUI::Align::Default,
        kRootLayer, kRootName);
    if (c.root == NULL)
    {
        return false;
    }
    c.root->setNeedMouseFocus(false);
    c.root->setNeedKeyFocus(false);
    c.root->setVisible(false);

    // Fills first, in the order they must be painted: textures are drawn in the
    // order they were assigned, so nothing here may be re-textured later.
    c.bg = MakeFill(c.root, kLacquer);
    if (c.bg != NULL) c.bg->setAlpha(0.92f);
    for (int i = 0; i < 4; ++i) c.frame[i] = MakeFill(c.root, kLine);
    c.headLine = MakeFill(c.root, kGoldDim);
    for (int i = 0; i < 4; ++i) c.tagFrame[i] = MakeFill(c.root, kGoldDim);

    c.stressBorder = MakeFill(c.root, kLine);
    c.stressBg = MakeFill(c.root, kSumi);
    c.segHunger = MakeFill(c.root, kAmber);
    c.segWounds = MakeFill(c.root, kDanger);
    c.segTired = MakeFill(c.root, kIndigo);
    c.swatch[0] = MakeFill(c.root, kAmber);
    c.swatch[1] = MakeFill(c.root, kDanger);
    c.swatch[2] = MakeFill(c.root, kIndigo);

    c.chanceBorder = MakeFill(c.root, kLine);
    c.chanceBg = MakeFill(c.root, kSumi);
    c.chanceFill = MakeFill(c.root, kBrass);

    c.foodBorder = MakeFill(c.root, kLine);
    c.foodBg = MakeFill(c.root, kSumi);
    c.foodFill = MakeFill(c.root, kBrass);
    c.foodNotch = MakeFill(c.root, kText);

    for (int i = 0; i < kMaxPips; ++i) c.pip[i] = MakeFill(c.root, kGoldDim);
    c.whyLine = MakeFill(c.root, kLine);

    // Then the texts, above every fill.
    c.name = MakeLabel(c.root, kNameFont, kBrass);
    c.tag = MakeLabel(c.root, kSmallFont, kText);
    c.stressLabel = MakeLabel(c.root, kSmallFont, kMuted);
    c.stressValue = MakeLabel(c.root, kBodyFont, kText);
    for (int i = 0; i < 3; ++i) c.legend[i] = MakeLabel(c.root, kSmallFont, kMuted);
    c.chanceLabel = MakeLabel(c.root, kSmallFont, kMuted);
    c.chanceValue = MakeLabel(c.root, kBodyFont, kAmber);
    c.foodLabel = MakeLabel(c.root, kSmallFont, kMuted);
    c.foodValue = MakeLabel(c.root, kBodyFont, kText);
    c.defeatsLabel = MakeLabel(c.root, kSmallFont, kMuted);
    c.defeatsValue = MakeLabel(c.root, kBodyFont, kText);
    c.primary = MakeLabel(c.root, kBodyFont, kText);
    c.secondary = MakeLabel(c.root, kSmallFont, kMuted);

    // The right-aligned values.
    MyGUI::TextBox* rights[4] = { c.stressValue, c.chanceValue, c.foodValue, c.defeatsValue };
    for (int i = 0; i < 4; ++i)
    {
        if (rights[i] != NULL)
            rights[i]->setTextAlign(MyGUI::Align::Right | MyGUI::Align::VCenter);
    }
    if (c.tag != NULL)
        c.tag->setTextAlign(MyGUI::Align::Center);
    return true;
}

// -------------------------------------------------------------------- Layout
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

// A 1px frame as four fills.
void PlaceFrame(MyGUI::Widget* const parts[4], int x, int y, int w, int h)
{
    Place(parts[0], x, y, w, 1);
    Place(parts[1], x, y + h - 1, w, 1);
    Place(parts[2], x, y, 1, h);
    Place(parts[3], x + w - 1, y, 1, h);
}

void HideAll()
{
    Card& c = g_card;
    MyGUI::Widget* all[] = {
        c.bg, c.headLine, c.stressBorder, c.stressBg, c.segHunger, c.segWounds, c.segTired,
        c.chanceBorder, c.chanceBg, c.chanceFill, c.foodBorder, c.foodBg, c.foodFill,
        c.foodNotch, c.whyLine
    };
    for (size_t i = 0; i < sizeof(all) / sizeof(all[0]); ++i) Hide(all[i]);
    for (int i = 0; i < 4; ++i) { Hide(c.frame[i]); Hide(c.tagFrame[i]); }
    for (int i = 0; i < 3; ++i) { Hide(c.swatch[i]); Hide(c.legend[i]); }
    for (int i = 0; i < kMaxPips; ++i) Hide(c.pip[i]);
    MyGUI::TextBox* texts[] = {
        c.name, c.tag, c.stressLabel, c.stressValue, c.chanceLabel, c.chanceValue,
        c.foodLabel, c.foodValue, c.defeatsLabel, c.defeatsValue, c.primary, c.secondary
    };
    for (size_t i = 0; i < sizeof(texts) / sizeof(texts[0]); ++i) Hide(texts[i]);
}

void SetText(MyGUI::TextBox* box, const std::string& text)
{
    if (box != NULL) box->setCaption(text);
}

// A label row ("ESTRES ......... 53%"): label left, value right, the bar under.
// Returns the y below the bar.
int PlaceBarRow(int x, int y, int innerW, int barH, bool compact, MyGUI::TextBox* label,
                MyGUI::TextBox* value, const std::string& labelText,
                const std::string& valueText, MyGUI::Widget* border, MyGUI::Widget* bg,
                int* barX, int* barW, int* barY)
{
    SetText(label, labelText);
    SetText(value, valueText);
    Place(label, x, y, innerW / 2, kLabelH);
    Place(value, x + innerW / 2, y, innerW - innerW / 2, kLabelH);
    y += kLabelH;
    Place(border, x, y, innerW, barH + 2);
    Place(bg, x + 1, y + 1, innerW - 2, barH);
    *barX = x + 1;
    *barW = innerW - 2;
    *barY = y + 1;
    return y + barH + 2 + (compact ? kGapCompact : kGap);
}

void LayoutUnguarded(const ReadoutView& view, const ReadoutTexts& texts, bool compact)
{
    Card& c = g_card;
    HideAll();

    const int width = compact ? kCardWidthCompact : kCardWidth;
    const int pad = compact ? kPadCompact : kPad;
    const int gap = compact ? kGapCompact : kGap;
    const int barH = compact ? kBarHCompact : kBarH;
    const int inner = width - 2 * pad;
    int y = pad;
    const int x = pad;

    // Header: the name, and the route as a small framed tag.
    SetText(c.name, texts.name);
    SetText(c.tag, texts.route);
    Place(c.name, x, y, inner - kTagW - 4, kHeadH);
    MyGUI::Widget* tagFrame[4] = { c.tagFrame[0], c.tagFrame[1], c.tagFrame[2], c.tagFrame[3] };
    PlaceFrame(tagFrame, x + inner - kTagW, y + 1, kTagW, kHeadH - 2);
    Place(c.tag, x + inner - kTagW, y + 1, kTagW, kHeadH - 2);
    y += kHeadH + 2;
    Place(c.headLine, x, y, inner, 1);
    y += 1 + gap;

    int bx = 0, bw = 0, by = 0;

    if (view.showStress)
    {
        y = PlaceBarRow(x, y, inner, barH, compact, c.stressLabel, c.stressValue,
                        texts.stressLabel, texts.stressValue, c.stressBorder, c.stressBg,
                        &bx, &bw, &by);
        // The three terms side by side; they add up to the bar.
        const int hungerW = bw * view.hungerPts / 100;
        const int woundsW = bw * view.woundsPts / 100;
        const int tiredW = bw * view.tiredPts / 100;
        if (hungerW > 0) Place(c.segHunger, bx, by, hungerW, barH);
        if (woundsW > 0) Place(c.segWounds, bx + hungerW, by, woundsW, barH);
        if (tiredW > 0) Place(c.segTired, bx + hungerW + woundsW, by, tiredW, barH);
        if (!compact)
        {
            const std::string* legends[3] = { &texts.hungerLegend, &texts.woundsLegend,
                                              &texts.tiredLegend };
            const int cell = inner / 3;
            for (int i = 0; i < 3; ++i)
            {
                SetText(c.legend[i], *legends[i]);
                Place(c.swatch[i], x + i * cell, y + 3, 6, 6);
                Place(c.legend[i], x + i * cell + 9, y, cell - 9, kLegendH);
            }
            y += kLegendH + gap;
        }
    }

    if (view.showFood)
    {
        y = PlaceBarRow(x, y, inner, barH, compact, c.foodLabel, c.foodValue, texts.foodLabel,
                        texts.foodValue, c.foodBorder, c.foodBg, &bx, &bw, &by);
        const int fillW = bw * view.foodPct / 100;
        if (fillW > 0)
        {
            c.foodFill->setColour(view.foodPct >= view.foodNeededPct ? kReady : kBrass);
            Place(c.foodFill, bx, by, fillW, barH);
        }
        Place(c.foodNotch, bx + bw * view.foodNeededPct / 100 - 1, by - 3, 2, barH + 6);
    }

    if (view.showChance)
    {
        y = PlaceBarRow(x, y, inner, barH, compact, c.chanceLabel, c.chanceValue,
                        texts.chanceLabel, texts.chanceValue, c.chanceBorder, c.chanceBg,
                        &bx, &bw, &by);
        const int fillW = bw * view.chancePct / 100;
        if (fillW > 0)
        {
            c.chanceFill->setColour(view.chancePct >= 66 ? kReady
                                    : (view.chancePct < 25 ? kDanger : kBrass));
            Place(c.chanceFill, bx, by, fillW, barH);
        }
    }

    if (view.showDefeats && !compact)
    {
        SetText(c.defeatsLabel, texts.defeatsLabel);
        SetText(c.defeatsValue, texts.defeatsValue);
        Place(c.defeatsLabel, x, y, inner / 2, kLabelH);
        Place(c.defeatsValue, x + inner - 30, y, 30, kLabelH);
        int pips = view.defeatsNeeded;
        if (pips > kMaxPips) pips = kMaxPips;
        const int pipsW = pips * kPipSize + (pips > 0 ? (pips - 1) * kPipGap : 0);
        int px = x + inner - 30 - 6 - pipsW;
        for (int i = 0; i < pips; ++i)
        {
            c.pip[i]->setColour(i < view.defeats ? kAmber : kGoldDim);
            Place(c.pip[i], px, y + (kLabelH - kPipSize) / 2, kPipSize, kPipSize);
            px += kPipSize + kPipGap;
        }
        y += kLabelH + gap;
    }

    // The one line standing in the way, and the wait beside another way in.
    Place(c.whyLine, x, y, inner, 1);
    y += 1 + gap;
    SetText(c.primary, texts.primary);
    c.primary->setTextColour(view.primary.tone == ReadoutTone_Good ? kReady
                             : (view.primary.tone == ReadoutTone_Refused ? kSoftDanger : kText));
    Place(c.primary, x, y, inner, kLineH);
    y += kLineH;
    if (!compact && !texts.secondary.empty())
    {
        SetText(c.secondary, texts.secondary);
        Place(c.secondary, x, y, inner, kAltH);
        y += kAltH;
    }
    y += pad - 2;

    // The tag colour follows the route, as the mockup .r-* classes do.
    MyGUI::Colour tagColour = kText;
    switch (view.route)
    {
    case ReadoutRoute_Captivity: tagColour = kAmber; break;
    case ReadoutRoute_Surrender: tagColour = kSoftDanger; break;
    case ReadoutRoute_Rescue:    tagColour = kReady; break;
    case ReadoutRoute_Animal:    tagColour = kIndigo; break;
    case ReadoutRoute_Mercy:     tagColour = kBrass; break;
    case ReadoutRoute_Free:
    default:                     break;
    }
    c.tag->setTextColour(tagColour);
    for (int i = 0; i < 4; ++i) c.tagFrame[i]->setColour(tagColour);

    // Plate and frame: last to be sized, first in paint order (created first).
    Place(c.bg, 0, 0, width, y);
    MyGUI::Widget* frame[4] = { c.frame[0], c.frame[1], c.frame[2], c.frame[3] };
    PlaceFrame(frame, 0, 0, width, y);

    g_width = width;
    g_height = y;
    c.root->setSize(width, y);
}

bool Intersects(int left, int top, int width, int height, const ReadoutRect& r)
{
    return left < r.left + r.width && r.left < left + width &&
           top < r.top + r.height && r.top < top + height;
}

// The card would cover `avoid` (the offer window): it steps to the window's right,
// or its left when the right has no room, or above / below it when neither side
// does. Clamped to the screen either way.
void StepAside(const ReadoutRect& avoid, int viewW, int viewH, int* left, int* top)
{
    if (!Intersects(*left, *top, g_width, g_height, avoid))
    {
        return;
    }

    const int right = avoid.left + avoid.width + kArrowGap;
    const int leftSide = avoid.left - kArrowGap - g_width;
    if (viewW <= 0 || right + g_width <= viewW - kScreenMargin)
    {
        *left = right;
    }
    else if (leftSide >= kScreenMargin)
    {
        *left = leftSide;
    }
    else
    {
        const int above = avoid.top - kArrowGap - g_height;
        *top = (above >= kScreenMargin) ? above : avoid.top + avoid.height + kArrowGap;
    }

    if (viewH > 0 && *top + g_height > viewH - kScreenMargin)
    {
        *top = viewH - kScreenMargin - g_height;
    }
    if (*top < kScreenMargin)
    {
        *top = kScreenMargin;
    }
}

void MoveUnguarded(int anchorX, int anchorY, const ReadoutRect* avoid)
{
    int viewW = 0, viewH = 0;
    MyGUI::RenderManager* render = MyGUI::RenderManager::getInstancePtr();
    if (render != NULL)
    {
        viewW = render->getViewSize().width;
        viewH = render->getViewSize().height;
    }

    int left = anchorX - g_width / 2;
    int top = anchorY - g_height - kArrowGap;
    if (viewW > 0)
    {
        if (left + g_width > viewW - kScreenMargin) left = viewW - kScreenMargin - g_width;
        if (left < kScreenMargin) left = kScreenMargin;
    }
    // No room above the head: hang the card below them instead of off the screen.
    if (top < kScreenMargin)
    {
        top = anchorY + kArrowGap * 4;
    }
    if (viewH > 0 && top + g_height > viewH - kScreenMargin)
    {
        top = viewH - kScreenMargin - g_height;
    }
    if (avoid != NULL)
    {
        StepAside(*avoid, viewW, viewH, &left, &top);
    }
    g_card.root->setPosition(left, top);
}

bool ShowUnguarded(const ReadoutView* view, const ReadoutTexts* texts, bool compact,
                   int anchorX, int anchorY, bool relayout)
{
    if (!g_built)
    {
        if (g_buildFailed)
        {
            return false;
        }
        if (!BuildUnguarded())
        {
            g_buildFailed = true;
            return false;
        }
        g_built = true;
        relayout = true;
    }
    if (relayout)
    {
        LayoutUnguarded(*view, *texts, compact);
    }
    MoveUnguarded(anchorX, anchorY, NULL);
    g_card.root->setVisible(true);
    return true;
}

// -------------------------------------------------------------------- Guards
// POD-only frames; each calls the unguarded work above.
bool SafeShow(const ReadoutView* view, const ReadoutTexts* texts, bool compact,
              int anchorX, int anchorY, bool relayout)
{
    TR_TRY { return ShowUnguarded(view, texts, compact, anchorX, anchorY, relayout); }
    TR_CATCH { return false; }
}

void SafeMove(int anchorX, int anchorY, const ReadoutRect* avoid)
{
    TR_TRY { if (g_built) MoveUnguarded(anchorX, anchorY, avoid); }
    TR_CATCH { }
}

void SafeHide()
{
    TR_TRY { if (g_built && g_card.root != NULL) g_card.root->setVisible(false); }
    TR_CATCH { }
}

bool SafeIsVisible()
{
    TR_TRY { return g_built && g_card.root != NULL && g_card.root->getVisible(); }
    TR_CATCH { return false; }
}

void DestroyUnguarded()
{
    MyGUI::Gui* gui = MyGUI::Gui::getInstancePtr();
    if (gui != NULL && g_card.root != NULL)
    {
        gui->destroyWidget(g_card.root);
    }
}

void SafeDestroy()
{
    TR_TRY { DestroyUnguarded(); }
    TR_CATCH { }
}

// Everything LayoutUnguarded reads, in one string: the texts and every number of
// the view that sizes, colours or places a widget.
std::string LayoutKey(const ReadoutView& v, const ReadoutTexts& t, bool compact)
{
    char nums[192] = { 0 };
    _snprintf_s(nums, sizeof(nums), _TRUNCATE,
                "%d|%d|%d%d%d%d|%d,%d,%d|%d,%d|%d|%d,%d|%d",
                compact ? 1 : 0, (int)v.route,
                v.showStress ? 1 : 0, v.showFood ? 1 : 0, v.showChance ? 1 : 0,
                v.showDefeats ? 1 : 0, v.hungerPts, v.woundsPts, v.tiredPts,
                v.foodPct, v.foodNeededPct, v.chancePct, v.defeats, v.defeatsNeeded,
                (int)v.primary.tone);
    std::string key(nums);
    const std::string* parts[] = {
        &t.name, &t.route, &t.stressLabel, &t.stressValue, &t.hungerLegend,
        &t.woundsLegend, &t.tiredLegend, &t.chanceLabel, &t.chanceValue,
        &t.defeatsLabel, &t.defeatsValue, &t.foodLabel, &t.foodValue,
        &t.primary, &t.secondary
    };
    for (size_t i = 0; i < sizeof(parts) / sizeof(parts[0]); ++i)
    {
        key += '\x1f';
        key += *parts[i];
    }
    return key;
}
} // namespace

bool ReadoutUiShow(const ReadoutView& view, const ReadoutTexts& texts, bool compact,
                   int anchorX, int anchorY)
{
    const std::string key = LayoutKey(view, texts, compact);
    const bool shown = SafeShow(&view, &texts, compact, anchorX, anchorY, key != g_layoutKey);
    g_layoutKey = shown ? key : std::string();
    if (!shown && g_buildFailed)
    {
        static bool reported = false;
        if (!reported)
        {
            reported = true;
            LogError("[readout] could not build the readout widgets; the card stays hidden.");
        }
    }
    return shown;
}

void ReadoutUiMove(int anchorX, int anchorY, const ReadoutRect* avoid)
{
    SafeMove(anchorX, anchorY, avoid);
}

void ReadoutUiHide()
{
    SafeHide();
}

bool ReadoutUiVisible()
{
    return SafeIsVisible();
}

void ReadoutUiDestroy()
{
    SafeDestroy();
    Card empty = { 0 };
    g_card = empty;
    g_built = false;
    g_buildFailed = false;
    g_layoutKey.clear();
}
} // namespace totalrecruit
