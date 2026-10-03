#include "common.h"
#include "offer_popup.h"
#include "rules.h"
#include "config.h"
#include "recruit_core.h"
#include "engine_reads.h"
#include "feeding.h"
#include "log.h"
#include "mercy.h"
#include "cage_commands.h"
#include "submission_director.h"
#include "ui_style.h"
#include "flat_slider.h"
#include "haggle_rules.h"

#include <kenshi/gui/PortraitManager.h>
#include <mygui/MyGUI_ImageBox.h>

// The recruit popup, drawn from an OfferView (offer_view.h).
//
// Built from the mockup (ui-mockup/readout.html, section 5; js/popup.js
// layoutOffer is LayoutUnguarded below, line for line). The frame is Kenshi's own
// Kenshi_WindowCX on the Info layer -- title bar, X and dragging come from the
// engine -- and the client area gets the mockup's flat art over a lacquer plate,
// as TotalSparring's duel popup does. Every widget is built once; an open only
// sets captions, colours, visibility and coordinates. Fills are created (and
// textured) before anything drawn over them (ui_style.h).

namespace totalrecruit
{
namespace
{
using namespace ui;

// Distinct names so this window can coexist with the context-menu overlay and
// with other mods' widgets.
const char* const kRootName     = "TR_OfferRoot";
const char* const kBodyName     = "TR_OfferBody";
const char* const kReasonName   = "TR_OfferReason";
const char* const kAcceptName   = "TR_OfferAccept";
const char* const kDeclineName  = "TR_OfferDecline";
const char* const kExtraName    = "TR_OfferExtra";
const char* const kBeatName     = "TR_OfferBeat";
const char* const kKnockoutName = "TR_OfferKnockout";
const char* const kPortraitName = "TR_OfferPortrait";
const char* const kSliderName   = "TR_OfferSlider";

// The native context menu sits on the Popup layer, so this goes above it.
const char* const kLayer = "Info";

// ------------------------------------------------------------------- Layout
// POPUP_LAYOUT in ui-mockup/js/popup_data.js.
const int kClientWidth  = 540;
const int kMargin       = 14;
const int kPortraitSide = 110;
const int kColumnGap    = 14;
const int kNameH        = 22;
const int kTagW         = 84;
const int kTagH         = 18;
const int kRuleGap      = 3;
const int kBodyGap      = 6;
const int kInfoGap      = 8;
const int kInfoRowH     = 22;
const int kInfoPad      = 4;
const int kInfoLabelW   = 120;
const int kSectionGap   = 12;
const int kReasonBarW   = 3;
const int kReasonPadX   = 10;
const int kReasonPadY   = 7;
const int kButtonHeight = 32;
const int kButtonGap    = 8;
const int kAloneButtonW = 150;
// More buttons than this and the closing one wraps alone to a second row, on the
// right (POPUP_LAYOUT buttonsPerRow): five across would not hold DARLE COMIDA.
const int kButtonsPerRow = 3;
const int kInfoRows     = 3;
// The haggle section (POPUP_LAYOUT haggle*).
const int kHaggleRowH   = 24;
const int kHaggleLabelW = 84;
const int kHaggleValueW = 110;
const int kHaggleGap    = 8;
const int kHaggleScaleH = 16;
const int kHaggleLineH  = 20;

// The frame the skin adds around the client area, measured once at build.
int g_frameW = 16;
int g_frameH = 40;

struct Popup
{
    MyGUI::Window* root;
    MyGUI::Widget* client;

    // Fills, in paint order.
    MyGUI::Widget* plate;
    MyGUI::Widget* portraitFrame[4];
    MyGUI::Widget* tagFrame[4];
    MyGUI::Widget* rule;
    MyGUI::Widget* infoFrame;
    MyGUI::Widget* infoPlate;
    MyGUI::Widget* reasonPlate;
    MyGUI::Widget* reasonBar;
    FlatButton accept;
    FlatButton extra;
    FlatButton beat;
    FlatButton knockout;
    FlatButton cancel;

    // Over the fills.
    MyGUI::ImageBox* portrait;
    MyGUI::TextBox* name;
    MyGUI::TextBox* tag;
    MyGUI::TextBox* infoLabel[kInfoRows];
    MyGUI::TextBox* infoValue[kInfoRows];
    MyGUI::EditBox* body;
    MyGUI::EditBox* reason;

    // The haggle section, over the slider's fills.
    MyGUI::TextBox* haggleLabel;
    MyGUI::TextBox* offerValue;
    MyGUI::TextBox* minLabel;
    MyGUI::TextBox* maxLabel;
    MyGUI::TextBox* haggleLine;
};

Popup g_ui = { 0 };

// Kept apart from Popup: its delegates are bound to this instance's address.
FlatSlider g_slider;

// A priced offer keeps what it was built from, so a slider move rebuilds the view
// (BuildOfferView) and relays only what changed. Off for mercy and plain views.
bool g_haggleLive = false;
OfferInput g_input;
OfferTexts g_texts;
OfferView g_view;
// Where the last layout put the buttons, and which info row is the purse (-1 none).
int g_buttonY = 0;
int g_purseRow = -1;

MyGUI::delegates::IDelegate2<MyGUI::Window*, const std::string&>* g_closeDelegate = NULL;
MyGUI::delegates::IDelegate1<MyGUI::Widget*>* g_acceptDelegate = NULL;
MyGUI::delegates::IDelegate1<MyGUI::Widget*>* g_declineDelegate = NULL;
MyGUI::delegates::IDelegate1<MyGUI::Widget*>* g_extraDelegate = NULL;
MyGUI::delegates::IDelegate1<MyGUI::Widget*>* g_beatDelegate = NULL;
MyGUI::delegates::IDelegate1<MyGUI::Widget*>* g_knockoutDelegate = NULL;

Character* g_subject = NULL;
bool g_open = false;

// Every MyGUI call materialises IntCoord and UString temporaries, so the real
// work lives in plain functions and only the call crosses the guard. C2712
// counts every C++ object in the whole frame, not just those inside __try.

// A TextBox caption parses '#' as a colour code; a name carrying one would lose
// characters. "##" draws a single '#'.
std::string Literal(const std::string& text)
{
    std::string out;
    for (size_t i = 0; i < text.size(); ++i)
    {
        out += text[i];
        if (text[i] == '#') out += '#';
    }
    return out;
}

// --------------------------------------------------------------- Portrait
// The portrait uses the engine's own PortraitManager, so the face in this
// window is the same one the squad bar and the vanilla dialogue draw -- no
// second rendering path to keep in step.
//
// PortraitManager has two paths, and the difference is the whole bug.
// setImageWidget/createPortraitImage go through characterPortraits, a map keyed
// by hand that only holds the player's own people -- so for anyone else they
// succeed and paint nothing. setImageWidgetNPC takes a raw Character* instead
// of a handle precisely because it skips that map, which is the case we have.
void LoadPortrait(Character* npc, int* outStage)
{
    *outStage = 1;
    PortraitManager* manager = PortraitManager::getInstance();
    if (manager == NULL)
    {
        return;
    }

    // The NPC path returns void, so it cannot report failure; reaching the
    // next line is all the success signal the engine gives us.
    *outStage = 2;
    manager->setImageWidgetNPC(npc, g_ui.portrait);
    *outStage = 5;
}

// Kept as a second attempt for characters the NPC path leaves blank: someone
// already in the squad does have a portrait in the map, and this is the call
// that finds it.
void LoadPortraitFromMap(Character* npc, int* outStage)
{
    *outStage = 1;
    PortraitManager* manager = PortraitManager::getInstance();
    if (manager == NULL)
    {
        return;
    }

    *outStage = 2;
    PortraitImage* image = NULL;
    if (!manager->createPortraitImage(image, npc))
    {
        return;
    }

    *outStage = 3;
    if (image == NULL)
    {
        return;
    }

    image->updateImageWidget(g_ui.portrait, true);
    *outStage = 4;
}

int GuardedLoadPortrait(Character* npc)
{
    int stage = 0;
    TR_TRY { LoadPortrait(npc, &stage); }
    TR_CATCH { return -stage; }
    return stage;
}

int GuardedLoadPortraitFromMap(Character* npc)
{
    int stage = 0;
    TR_TRY { LoadPortraitFromMap(npc, &stage); }
    TR_CATCH { return -stage; }
    return stage;
}

void SafeLoadPortrait(Character* npc)
{
    if (g_ui.portrait == NULL || npc == NULL)
    {
        return;
    }

    // The stage says how far the load got, so a blank frame can be told apart
    // from a manager that was never there. The guarded calls keep a POD-only
    // frame; the reporting happens outside them.
    // Stage 5 = the NPC path ran, 4 = the map path painted, negative = threw.
    int stage = GuardedLoadPortrait(npc);
    if (stage != 5)
    {
        stage = GuardedLoadPortraitFromMap(npc);
    }

    // The NPC path cannot say whether it painted anything, so a frame it
    // touched stays visible. Hiding on "unknown" is what would keep a working
    // portrait off screen.
    const bool loaded = (stage == 5 || stage == 4);
    if (!loaded && GetConfig().probeOnClick)
    {
        char stageText[16] = { 0 };
        _snprintf_s(stageText, sizeof(stageText), _TRUNCATE, "%d", stage);
        LogInfo("Portrait not drawn (stage " + std::string(stageText) + ").");
    }

    // A missing portrait is not worth failing the window over; the frame just
    // stays empty.
    SafeSetVisible(g_ui.portrait, loaded);
}

// ------------------------------------------------------------------ Build
// Kenshi_WindowCX is a real MyGUI::Window with title-bar chrome, so it has to
// be created as one and its children parented to getClientWidget() -- placing
// them on the frame itself puts them behind the caption bar and off the edges.
MyGUI::Window* CreateRoot(MyGUI::Gui* gui, const char* skin)
{
    return gui->createWidget<MyGUI::Window>(
        skin, MyGUI::IntCoord(40, 40, kClientWidth + g_frameW, 300),
        MyGUI::Align::Default, kLayer, kRootName);
}

MyGUI::Window* SafeCreateRoot(MyGUI::Gui* gui, const char* skin)
{
    TR_TRY { return CreateRoot(gui, skin); }
    TR_CATCH { return NULL; }
}

// The client area excludes the title bar, so children laid out inside it land
// where they look like they should. Also measures what the frame adds.
bool SafeTakeClient(MyGUI::Window* window)
{
    TR_TRY
    {
        MyGUI::Widget* client = window->getClientWidget();
        g_ui.client = (client != NULL) ? client : window;
        const int fw = window->getWidth() - g_ui.client->getWidth();
        const int fh = window->getHeight() - g_ui.client->getHeight();
        if (fw >= 0 && fw < 200) g_frameW = fw;
        if (fh >= 0 && fh < 200) g_frameH = fh;
        return true;
    }
    TR_CATCH { return false; }
}

// Fills first, in the order they paint; then everything drawn over them.
bool BuildBodyUnguarded()
{
    Popup& p = g_ui;
    MyGUI::Widget* c = p.client;

    p.plate = MakeFill(c, kLacquer);
    for (int i = 0; i < 4; ++i) p.portraitFrame[i] = MakeFill(c, kLine);
    for (int i = 0; i < 4; ++i) p.tagFrame[i] = MakeFill(c, kText);
    p.rule = MakeFill(c, kGoldDim);
    p.infoFrame = MakeFill(c, kLine);
    p.infoPlate = MakeFill(c, kBgDark);
    p.reasonPlate = MakeFill(c, kBgDark);
    p.reasonBar = MakeFill(c, kDanger);
    MakeFlatButtonPlates(c, kAmber, NULL, &p.accept);
    MakeFlatButtonPlates(c, kSumi, &kAmber, &p.extra);
    MakeFlatButtonPlates(c, kSumi, &kAmber, &p.beat);
    MakeFlatButtonPlates(c, kSumi, &kAmber, &p.knockout);
    MakeFlatButtonPlates(c, kLine, &kGoldDim, &p.cancel);
    g_slider.MakeFills(c);

    p.portrait = c->createWidget<MyGUI::ImageBox>(
        "ImageBox", MyGUI::IntCoord(kMargin + 1, kMargin + 1, kPortraitSide, kPortraitSide),
        MyGUI::Align::Default, kPortraitName);
    if (p.portrait != NULL)
    {
        p.portrait->setNeedMouseFocus(false);
    }

    p.name = MakeLabel(c, kNameFont, kBrass);
    p.tag = MakeLabel(c, kSmallFont, kText);
    if (p.tag != NULL) p.tag->setTextAlign(MyGUI::Align::Center);
    for (int i = 0; i < kInfoRows; ++i)
    {
        p.infoLabel[i] = MakeLabel(c, kBodyFont, kMuted);
        p.infoValue[i] = MakeLabel(c, kMediumFont, kText);
    }
    p.body = MakeWrapText(c, kBodyName, kMediumFont, kText);
    p.reason = MakeWrapText(c, kReasonName, kMediumFont, kSoftDanger);
    p.haggleLabel = MakeLabel(c, kBodyFont, kMuted);
    p.offerValue = MakeLabel(c, kMediumFont, kAmber);
    if (p.offerValue != NULL) p.offerValue->setTextAlign(MyGUI::Align::Right | MyGUI::Align::VCenter);
    p.minLabel = MakeLabel(c, kSmallFont, kMuted);
    p.maxLabel = MakeLabel(c, kSmallFont, kMuted);
    if (p.maxLabel != NULL) p.maxLabel->setTextAlign(MyGUI::Align::Right | MyGUI::Align::VCenter);
    p.haggleLine = MakeLabel(c, kBodyFont, kReady);

    MakeFlatButtonCaption(c, kAcceptName, kMediumFont, kLacquer, &p.accept);
    MakeFlatButtonCaption(c, kExtraName, kMediumFont, kAmber, &p.extra);
    MakeFlatButtonCaption(c, kBeatName, kMediumFont, kAmber, &p.beat);
    MakeFlatButtonCaption(c, kKnockoutName, kMediumFont, kAmber, &p.knockout);
    MakeFlatButtonCaption(c, kDeclineName, kMediumFont, kText, &p.cancel);
    g_slider.MakeSurface(c, kSliderName);

    // The caption is the engine's title bar; only its colour is the mockup's.
    MyGUI::TextBox* caption = p.root->getCaptionWidget();
    if (caption != NULL)
    {
        caption->setTextColour(kBrass);
    }

    return p.plate != NULL && p.body != NULL && p.accept.button != NULL &&
           p.extra.button != NULL && p.beat.button != NULL && p.knockout.button != NULL &&
           p.cancel.button != NULL;
}

bool SafeBuildBody()
{
    TR_TRY { return BuildBodyUnguarded(); }
    TR_CATCH { return false; }
}

void OnAccept(MyGUI::Widget* sender);
void OnDecline(MyGUI::Widget* sender);
void OnWindowButton(MyGUI::Window* sender, const std::string& button);
void OnExtra(MyGUI::Widget* sender);
void OnBeat(MyGUI::Widget* sender);
void OnKnockout(MyGUI::Widget* sender);
void OnSliderChange(int step);

bool BindButtons()
{
    g_slider.SetOnChange(&OnSliderChange);
    TR_TRY
    {
        // Kenshi_WindowCX draws a close cross in its title bar; it means the
        // same as Cancel -- the player walked away without deciding.
        g_closeDelegate = MyGUI::newDelegate(&OnWindowButton);
        g_ui.root->eventWindowButtonPressed += g_closeDelegate;

        g_acceptDelegate = MyGUI::newDelegate(&OnAccept);
        g_declineDelegate = MyGUI::newDelegate(&OnDecline);
        g_extraDelegate = MyGUI::newDelegate(&OnExtra);
        g_beatDelegate = MyGUI::newDelegate(&OnBeat);
        g_knockoutDelegate = MyGUI::newDelegate(&OnKnockout);
        g_ui.accept.button->eventMouseButtonClick += g_acceptDelegate;
        g_ui.cancel.button->eventMouseButtonClick += g_declineDelegate;
        g_ui.extra.button->eventMouseButtonClick += g_extraDelegate;
        g_ui.beat.button->eventMouseButtonClick += g_beatDelegate;
        g_ui.knockout.button->eventMouseButtonClick += g_knockoutDelegate;
        return true;
    }
    TR_CATCH { return false; }
}

void Unbind()
{
    if (g_ui.accept.button != NULL && g_acceptDelegate != NULL)
    {
        g_ui.accept.button->eventMouseButtonClick -= g_acceptDelegate;
    }
    if (g_ui.cancel.button != NULL && g_declineDelegate != NULL)
    {
        g_ui.cancel.button->eventMouseButtonClick -= g_declineDelegate;
    }
    if (g_ui.extra.button != NULL && g_extraDelegate != NULL)
    {
        g_ui.extra.button->eventMouseButtonClick -= g_extraDelegate;
    }
    if (g_ui.beat.button != NULL && g_beatDelegate != NULL)
    {
        g_ui.beat.button->eventMouseButtonClick -= g_beatDelegate;
    }
    if (g_ui.knockout.button != NULL && g_knockoutDelegate != NULL)
    {
        g_ui.knockout.button->eventMouseButtonClick -= g_knockoutDelegate;
    }
    if (g_ui.root != NULL && g_closeDelegate != NULL)
    {
        g_ui.root->eventWindowButtonPressed -= g_closeDelegate;
    }
    g_slider.Forget();
}

void SafeUnbind()
{
    TR_TRY { Unbind(); }
    TR_CATCH { }
}

void DestroyWidget(MyGUI::Widget* widget)
{
    MyGUI::Gui* instance = MyGUI::Gui::getInstancePtr();
    if (instance != NULL)
    {
        instance->destroyWidget(widget);
    }
}

void SafeDestroyWidget(MyGUI::Widget* widget)
{
    if (widget == NULL) return;
    TR_TRY { DestroyWidget(widget); }
    TR_CATCH { }
}

bool Build()
{
    if (g_ui.root != NULL)
    {
        return true;
    }

    MyGUI::Gui* gui = MyGUI::Gui::getInstancePtr();
    if (gui == NULL)
    {
        return false;
    }

    // Skin availability differs between installs, so fall back to stock ones.
    const char* panelSkins[3] = { "Kenshi_WindowCX", "WindowCSX", "WindowC" };
    for (int i = 0; i < 3 && g_ui.root == NULL; ++i)
    {
        g_ui.root = SafeCreateRoot(gui, panelSkins[i]);
    }
    if (g_ui.root == NULL)
    {
        LogError("Could not create the offer window (no usable window skin).");
        return false;
    }

    if (!SafeTakeClient(g_ui.root))
    {
        LogError("Offer window has no client area.");
        return false;
    }

    if (!SafeBuildBody())
    {
        LogError("Could not create the offer window's contents.");
        return false;
    }

    BindButtons();
    SafeSetVisible(g_ui.root, false);
    return true;
}

// ----------------------------------------------------------------- Layout
// js/popup.js layoutOffer, in client coordinates. Returns the client height.
void HideAllUnguarded()
{
    Popup& p = g_ui;
    MyGUI::Widget* fills[] = { p.rule, p.infoFrame, p.infoPlate, p.reasonPlate, p.reasonBar,
                               p.name, p.tag, p.body, p.reason, p.haggleLabel, p.offerValue,
                               p.minLabel, p.maxLabel, p.haggleLine };
    for (size_t i = 0; i < sizeof(fills) / sizeof(fills[0]); ++i) Hide(fills[i]);
    for (int i = 0; i < 4; ++i) { Hide(p.portraitFrame[i]); Hide(p.tagFrame[i]); }
    for (int i = 0; i < kInfoRows; ++i) { Hide(p.infoLabel[i]); Hide(p.infoValue[i]); }
    ShowButton(p.accept, false);
    ShowButton(p.extra, false);
    ShowButton(p.beat, false);
    ShowButton(p.knockout, false);
    ShowButton(p.cancel, false);
    g_slider.Hide();
}

MyGUI::Colour ToneColour(OddsTone tone)
{
    switch (tone)
    {
    case OddsTone_Ready:  return kReady;
    case OddsTone_Amber:  return kAmber;
    case OddsTone_Danger:
    default:              return kSoftDanger;
    }
}

// The haggle texts and colours that a slider move changes (the rectangles do not).
void ApplyHaggleTextsUnguarded(const OfferView& view)
{
    Popup& p = g_ui;
    p.haggleLabel->setCaption(view.haggleLabel);
    p.offerValue->setCaption(view.offerText);
    p.minLabel->setCaption(view.minText);
    p.maxLabel->setCaption(view.maxText);
    p.haggleLine->setCaption(view.haggleLine);
    p.haggleLine->setTextColour(ToneColour(view.haggleTone));
    if (g_purseRow >= 0)
    {
        p.infoValue[g_purseRow]->setTextColour(view.affordable ? kText : kSoftDanger);
    }
}

// js/popup.js layoutOffer, the haggle section: OFERTA | slider | value, the min and
// max under the slider's ends, then the line (odds, shortfall, offended). Fixed
// height, so a slider move never resizes the window. Returns the y under it.
int LayoutHaggleUnguarded(const OfferView& view, int y)
{
    Popup& p = g_ui;
    if (!view.showSlider && view.haggleLine.empty())
    {
        return y;
    }
    const int W = kClientWidth;
    const int m = kMargin;
    y += kSectionGap;
    if (view.showSlider)
    {
        const int sx = m + kHaggleLabelW;
        const int sw = W - 2 * m - kHaggleLabelW - kHaggleGap - kHaggleValueW;
        Place(p.haggleLabel, m, y, kHaggleLabelW, kHaggleRowH);
        g_slider.SetRange(view.sliderSteps);
        g_slider.SetValue(view.sliderStep);
        g_slider.Place(sx, y, sw, kHaggleRowH);
        Place(p.offerValue, W - m - kHaggleValueW, y, kHaggleValueW, kHaggleRowH);
        y += kHaggleRowH;
        const int half = sw / 2;
        Place(p.minLabel, sx, y, half, kHaggleScaleH);
        Place(p.maxLabel, sx + half, y, sw - half, kHaggleScaleH);
        y += kHaggleScaleH;
    }
    Place(p.haggleLine, m, y, W - 2 * m, kHaggleLineH);
    y += kHaggleLineH;
    ApplyHaggleTextsUnguarded(view);
    return y;
}

// `count` buttons share one row across the window, in order.
void PlaceButtonRow(const FlatButton* const* order, const std::string* const* captions,
                    int count, int y)
{
    const int W = kClientWidth;
    const int m = kMargin;
    const int bw = (W - 2 * m - (count - 1) * kButtonGap) / count;
    for (int i = 0; i < count; ++i)
    {
        SetButtonCaption(*order[i], *captions[i]);
        PlaceButton(*order[i], m + i * (bw + kButtonGap), y, bw, kButtonHeight);
    }
}

// Accept (or OFRECER), Extra, Golpear, Noquear, Cancel share the row; Cancel alone
// sits on the right. More than kButtonsPerRow: the others share the first row and
// Cancel sits alone on the right of a second one. Returns the y under the buttons.
int LayoutButtonsUnguarded(const OfferView& view, int y)
{
    Popup& p = g_ui;
    const int W = kClientWidth;
    const int m = kMargin;
    ShowButton(p.accept, false);
    ShowButton(p.extra, false);
    ShowButton(p.beat, false);
    ShowButton(p.knockout, false);
    ShowButton(p.cancel, false);

    const FlatButton* order[5] = { NULL, NULL, NULL, NULL, NULL };
    const std::string* captions[5] = { NULL, NULL, NULL, NULL, NULL };
    int count = 0;
    if (!view.acceptLabel.empty())   { order[count] = &p.accept;   captions[count] = &view.acceptLabel;   ++count; }
    if (!view.extraLabel.empty())    { order[count] = &p.extra;    captions[count] = &view.extraLabel;    ++count; }
    if (!view.beatLabel.empty())     { order[count] = &p.beat;     captions[count] = &view.beatLabel;     ++count; }
    if (!view.knockoutLabel.empty()) { order[count] = &p.knockout; captions[count] = &view.knockoutLabel; ++count; }
    const int actions = count;
    order[count] = &p.cancel;
    captions[count] = &view.cancelLabel;
    ++count;

    if (count <= kButtonsPerRow && actions > 0)
    {
        PlaceButtonRow(order, captions, count, y);
        return y + kButtonHeight;
    }
    if (actions > 0)
    {
        PlaceButtonRow(order, captions, actions, y);
        y += kButtonHeight + kButtonGap;
    }
    SetButtonCaption(p.cancel, view.cancelLabel);
    PlaceButton(p.cancel, W - m - kAloneButtonW, y, kAloneButtonW, kButtonHeight);
    return y + kButtonHeight;
}

MyGUI::Colour TagColour(ReadoutRoute route)
{
    switch (route)
    {
    case ReadoutRoute_Captivity: return kAmber;
    case ReadoutRoute_Surrender:
    case ReadoutRoute_FieldYield: return kSoftDanger;
    case ReadoutRoute_Rescue:    return kReady;
    case ReadoutRoute_Animal:    return kIndigo;
    case ReadoutRoute_Mercy:     return kBrass;
    case ReadoutRoute_Free:
    default:                     return kText;
    }
}

int LayoutUnguarded(const OfferView& view)
{
    Popup& p = g_ui;
    HideAllUnguarded();

    const int W = kClientWidth;
    const int m = kMargin;
    const int P = kPortraitSide;
    const int x0 = m + P + 2 + kColumnGap;
    const int colW = W - x0 - m;

    // Portrait framed in line.
    PlaceFrame(p.portraitFrame, m, m, P + 2, P + 2);
    if (p.portrait != NULL) p.portrait->setCoord(m + 1, m + 1, P, P);

    // Name in brass, the route as a framed tag, a golddim rule under them.
    int y = m;
    const int tagY = y + (kNameH - kTagH) / 2;
    const MyGUI::Colour tagColour = TagColour(view.route);
    p.name->setCaption(Literal(view.name));
    Place(p.name, x0, y, colW - kTagW - 6, kNameH);
    for (int i = 0; i < 4; ++i) p.tagFrame[i]->setColour(tagColour);
    PlaceFrame(p.tagFrame, x0 + colW - kTagW, tagY, kTagW, kTagH);
    p.tag->setCaption(Literal(view.routeTag));
    p.tag->setTextColour(tagColour);
    Place(p.tag, x0 + colW - kTagW, tagY, kTagW, kTagH);
    y += kNameH + kRuleGap;
    Place(p.rule, x0, y, colW, 1);
    y += 1 + kBodyGap;

    // The body line, wrapped and measured.
    const int bodyH = MeasureWrapText(p.body, view.bodyLine, colW);
    Place(p.body, x0, y, colW, bodyH);
    y += bodyH;

    // Info box: only the rows that apply.
    int rows = 0;
    const std::string* labels[kInfoRows] = { NULL, NULL, NULL };
    const std::string* values[kInfoRows] = { NULL, NULL, NULL };
    MyGUI::Colour inks[kInfoRows] = { kText, kText, kText };
    if (view.showPrice)
    {
        labels[rows] = &view.priceLabel;
        values[rows] = &view.priceText;
        inks[rows] = view.priceFree ? kReady : kAmber;
        ++rows;
    }
    g_purseRow = -1;
    if (view.showPurse)
    {
        g_purseRow = rows;
        labels[rows] = &view.purseLabel;
        values[rows] = &view.purseText;
        inks[rows] = view.affordable ? kText : kSoftDanger;
        ++rows;
    }
    if (view.showChance)
    {
        labels[rows] = &view.chanceLabel;
        values[rows] = &view.chanceText;
        inks[rows] = kText;
        ++rows;
    }
    if (rows > 0)
    {
        y += kInfoGap;
        const int infoH = rows * kInfoRowH + 2 * kInfoPad;
        Place(p.infoFrame, x0, y, colW, infoH);
        Place(p.infoPlate, x0 + 1, y + 1, colW - 2, infoH - 2);
        for (int i = 0; i < rows; ++i)
        {
            const int ry = y + kInfoPad + i * kInfoRowH;
            p.infoLabel[i]->setCaption(*labels[i]);
            Place(p.infoLabel[i], x0 + 10, ry, kInfoLabelW, kInfoRowH);
            p.infoValue[i]->setCaption(*values[i]);
            p.infoValue[i]->setTextColour(inks[i]);
            Place(p.infoValue[i], x0 + 10 + kInfoLabelW, ry, colW - 20 - kInfoLabelW, kInfoRowH);
        }
        y += infoH;
    }

    if (y < m + P + 2) y = m + P + 2;

    y = LayoutHaggleUnguarded(view, y);

    // The reason: a dark plate across the window, a red bar when it is a no.
    if (!view.reasonText.empty())
    {
        y += kSectionGap;
        const int textX = m + kReasonBarW + kReasonPadX;
        const int textW = W - 2 * m - kReasonBarW - 2 * kReasonPadX;
        p.reason->setTextColour(view.reasonRefused ? kSoftDanger : kText);
        const int textH = MeasureWrapText(p.reason, view.reasonText, textW);
        const int plateH = textH + 2 * kReasonPadY;
        Place(p.reasonPlate, m, y, W - 2 * m, plateH);
        if (view.reasonRefused) Place(p.reasonBar, m, y, kReasonBarW, plateH);
        Place(p.reason, textX, y + kReasonPadY, textW, textH);
        y += plateH;
    }

    // Buttons: one row, or two when there are more than kButtonsPerRow.
    y += kSectionGap;
    g_buttonY = y;
    const int H = LayoutButtonsUnguarded(view, y) + m;

    // The plate last to be sized, first in paint order (created first).
    Place(p.plate, 0, 0, W, H);
    return H;
}

bool SafeLayout(const OfferView* view, int* outHeight)
{
    TR_TRY
    {
        *outHeight = LayoutUnguarded(*view);
        return true;
    }
    TR_CATCH { return false; }
}

void ApplyFrame(const char* title, int left, int top, int width, int height)
{
    g_ui.root->setCaption(title);
    g_ui.root->setCoord(left, top, width, height);
}

bool SafeApplyFrame(const char* title, int left, int top, int width, int height)
{
    TR_TRY
    {
        ApplyFrame(title, left, top, width, height);
        return true;
    }
    TR_CATCH { return false; }
}

// Centred on the screen; never above the top 30 px. Responsive to the view size.
void ChoosePosition(int width, int height, int* outLeft, int* outTop)
{
    int viewWidth = 0;
    int viewHeight = 0;
    TR_TRY
    {
        MyGUI::RenderManager* renderManager = MyGUI::RenderManager::getInstancePtr();
        if (renderManager != NULL)
        {
            viewWidth = renderManager->getViewSize().width;
            viewHeight = renderManager->getViewSize().height;
        }
    }
    TR_CATCH { viewWidth = 0; viewHeight = 0; }

    *outLeft = (viewWidth > width) ? (viewWidth - width) / 2 : 40;
    *outTop = (viewHeight > height) ? (viewHeight - height) / 2 : 40;
    if (*outTop < 30)
    {
        *outTop = 30;
    }
}

// ---------------------------------------------------------------- Answers
void Close()
{
    g_open = false;
    g_subject = NULL;
    g_haggleLive = false;
    SafeSetVisible(g_ui.root, false);
    // Whatever way the window ends -- an answer, the title-bar cross, the HUD
    // hiding it, shutdown -- no plea is on screen any more. A pending plea
    // blocks every later one, so leaving it set meant no one could beg again
    // until a reload. Answer reads the pending kind BEFORE calling Close for
    // the same reason. A submission's mark goes too: the director reads an
    // unanswered close as Dismiss.
    ForgetMercy(NULL);
    ForgetSubmissionOffer();
}

// Who the window is asking about decides what each button means, and it is
// decided here, once, for every answer: a mercy plea and a submission each have
// their own three answers; everything else is an ordinary offer.
enum OfferKind
{
    OfferKind_Plain,
    OfferKind_Mercy,
    OfferKind_Submission
};

enum OfferAnswer
{
    Answer_Accept,
    Answer_Extra,
    Answer_Decline,
    // The cage commands (an ordinary offer only: BuildOfferView alone sets them).
    Answer_Beat,
    Answer_Knockout,
    // The title-bar X. The same as Decline, except for a submission, where it
    // is no decision at all (submission_director.h).
    Answer_Close
};

OfferKind PendingKind(Character* npc)
{
    if (IsMercyPending(npc))
    {
        return OfferKind_Mercy;
    }
    if (IsSubmissionPending(npc))
    {
        return OfferKind_Submission;
    }
    return OfferKind_Plain;
}

// A plea for mercy is answered by its own rule, not by the price evaluation
// every other offer goes through. Refusing it is a real answer: nothing
// changes, they stay hostile, and the fight goes on. The third button is the
// one answer that is neither joining nor killing.
void AnswerMercy(Character* npc, OfferAnswer answer)
{
    switch (answer)
    {
    case Answer_Accept: AcceptMercy(npc); return;
    case Answer_Extra:  GrantMercy(npc); return;
    default:
        TellPlayer(CharacterName(npc) + ": ignoraste su s\xc3\xbaplica.", false);
        return;
    }
}

void AnswerSubmission(Character* npc, OfferAnswer answer)
{
    switch (answer)
    {
    case Answer_Accept:  AcceptSubmission(npc); return;
    case Answer_Extra:   ForgiveSubmission(npc); return;
    case Answer_Decline: RejectSubmission(npc); return;
    case Answer_Close:   DismissSubmission(npc); return;
    default:             return;
    }
}

// GOLPEAR / NOQUEAR: the very orders of the context menu's Golpear / Noquear,
// given to the selected character; the engine decides how it goes. The window
// is already closed: the order is the answer. Nobody selected says so.
void OrderCageCommand(Character* npc, OfferAnswer answer)
{
    const CageCommandResult result = (answer == Answer_Knockout)
                                         ? OrderSelectedToKnockout(npc)
                                         : OrderSelectedToBeat(npc);
    if (result != CageCommand_Issued)
    {
        TellPlayer(CageCommandProblem(result), true);
    }
}

// OFRECER rides on the accept button: below the ask it is one haggle attempt.
// The extra button acts on the refusal instead of answering it -- feeding a
// starving captive, for one -- then reopens so the player sees the new verdict.
void AnswerPlain(Character* npc, OfferAnswer answer, bool offering, int offer)
{
    switch (answer)
    {
    case Answer_Accept:
        if (offering)
        {
            CompleteHaggle(npc, offer);
        }
        else
        {
            CompleteOffer(npc, true);
        }
        return;
    case Answer_Extra:
        HelpCaptive(npc);
        return;
    case Answer_Beat:
    case Answer_Knockout:
        OrderCageCommand(npc, answer);
        return;
    default:
        CompleteOffer(npc, false);
        return;
    }
}

// Every button and the X end here: read what is pending, close, then route.
void Answer(OfferAnswer answer)
{
    Character* npc = g_subject;
    const OfferKind kind = PendingKind(npc);
    const bool offering = g_haggleLive && g_view.acceptIsOffer;
    const int offer = g_view.offerCats;
    Close();

    if (npc == NULL)
    {
        return;
    }
    switch (kind)
    {
    case OfferKind_Mercy:      AnswerMercy(npc, answer); return;
    case OfferKind_Submission: AnswerSubmission(npc, answer); return;
    case OfferKind_Plain:      AnswerPlain(npc, answer, offering, offer); return;
    }
}

void OnAccept(MyGUI::Widget* /*sender*/)
{
    Answer(Answer_Accept);
}

// MyGUI names the title-bar buttons by their skin; "close" is the cross.
void OnWindowButton(MyGUI::Window* /*sender*/, const std::string& button)
{
    if (button == "close")
    {
        Answer(Answer_Close);
    }
}

void OnDecline(MyGUI::Widget* /*sender*/)
{
    Answer(Answer_Decline);
}

void OnExtra(MyGUI::Widget* /*sender*/)
{
    Answer(Answer_Extra);
}

void OnBeat(MyGUI::Widget* /*sender*/)
{
    Answer(Answer_Beat);
}

void OnKnockout(MyGUI::Widget* /*sender*/)
{
    Answer(Answer_Knockout);
}

// The window's screen rectangle as drawn now (the player may have dragged it).
// POD-only frame: the coord is read field by field inside the guard.
bool SafeWindowRect(int* left, int* top, int* width, int* height)
{
    TR_TRY
    {
        if (g_ui.root == NULL)
        {
            return false;
        }
        *left = g_ui.root->getAbsoluteLeft();
        *top = g_ui.root->getAbsoluteTop();
        *width = g_ui.root->getWidth();
        *height = g_ui.root->getHeight();
        return *width > 0 && *height > 0;
    }
    TR_CATCH { return false; }
}

void SafePlayChime()
{
    if (ou == NULL) return;
    TR_TRY { ou->playNotification(NULL); }
    TR_CATCH { }
}

// A slider move: only the haggle texts, the purse colour and the button row change
// (the section has a fixed height), so nothing is re-measured or re-placed above.
void RefreshHaggleUnguarded()
{
    ApplyHaggleTextsUnguarded(g_view);
    LayoutButtonsUnguarded(g_view, g_buttonY);
}

void SafeRefreshHaggle()
{
    TR_TRY { RefreshHaggleUnguarded(); }
    TR_CATCH { }
}

// FlatSlider's change callback: fired only when the position actually moved.
void OnSliderChange(int step)
{
    if (!g_open || !g_haggleLive)
    {
        return;
    }
    g_input.haggle.offer = OfferAtStep(g_view.haggleBounds, step);
    g_view = BuildOfferView(g_input, g_texts);
    if (!g_view.showSlider)
    {
        return;
    }
    SafeRefreshHaggle();
}

// The haggle widgets all exist (the window still opens without them, plain).
bool HaggleWidgetsBuilt()
{
    const Popup& p = g_ui;
    return g_slider.IsBuilt() && p.haggleLabel != NULL && p.offerValue != NULL &&
           p.minLabel != NULL && p.maxLabel != NULL && p.haggleLine != NULL;
}
} // namespace

bool IsOfferOpen()
{
    return g_open;
}

bool OfferWindowRect(int* left, int* top, int* width, int* height)
{
    return g_open && SafeWindowRect(left, top, width, height);
}

void HideOffer()
{
    Close();
}

// Lays the view out and shows the window for `npc`.
static bool OpenWith(Character* npc, const OfferView& view)
{
    if (npc == NULL)
    {
        return false;
    }

    // Opening a window replaces whatever was on screen, plea included, and the
    // buttons below route on whether a plea is pending. A plea that outlived
    // its window would turn this offer's ACEPTAR into a free join. OfferMercy
    // marks its plea AFTER this call succeeds, for the same reason; so does the
    // submission director.
    ForgetMercy(NULL);
    ForgetSubmissionOffer();

    if (!Build())
    {
        return false;
    }

    int clientHeight = 0;
    if (!SafeLayout(&view, &clientHeight))
    {
        LogError("Could not lay out the offer window.");
        return false;
    }

    const int width = kClientWidth + g_frameW;
    const int height = clientHeight + g_frameH;
    int left = 0;
    int top = 0;
    ChoosePosition(width, height, &left, &top);
    if (!SafeApplyFrame(view.title.c_str(), left, top, width, height))
    {
        return false;
    }

    g_subject = npc;
    g_open = true;

    SafeLoadPortrait(npc);
    SafePlayChime();
    SafeSetVisible(g_ui.root, true);
    return true;
}

bool ShowOffer(Character* npc, const OfferView& view)
{
    g_haggleLive = false;
    return OpenWith(npc, view);
}

bool ShowPricedOffer(Character* npc, const OfferInput& input, const OfferTexts& texts)
{
    g_haggleLive = false;
    if (npc == NULL || !Build())
    {
        return false;
    }
    g_input = input;
    // Without its widgets the slider cannot be drawn: the plain window, ACEPTAR at
    // the ask, rather than none.
    if (!HaggleWidgetsBuilt())
    {
        g_input.haggle.enabled = false;
    }
    g_texts = texts;
    g_view = BuildOfferView(g_input, g_texts);
    const bool opened = OpenWith(npc, g_view);
    g_haggleLive = opened && g_view.showSlider;
    return opened;
}

void ShutdownOfferPopup()
{
    Close();
    SafeUnbind();

    // Children go with the root, so only the root is destroyed.
    SafeDestroyWidget(g_ui.root);

    Popup empty = { 0 };
    g_ui = empty;
    g_acceptDelegate = NULL;
    g_declineDelegate = NULL;
    g_extraDelegate = NULL;
    g_beatDelegate = NULL;
    g_knockoutDelegate = NULL;
    g_closeDelegate = NULL;
}
} // namespace totalrecruit
