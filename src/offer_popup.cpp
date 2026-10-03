#include "common.h"
#include "offer_popup.h"
#include "rules.h"
#include "config.h"
#include "recruit_core.h"
#include "engine_reads.h"
#include "feeding.h"
#include "log.h"
#include "mercy.h"
#include "ui_style.h"

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
const char* const kPortraitName = "TR_OfferPortrait";

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
const int kInfoRows     = 3;

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
    FlatButton cancel;

    // Over the fills.
    MyGUI::ImageBox* portrait;
    MyGUI::TextBox* name;
    MyGUI::TextBox* tag;
    MyGUI::TextBox* infoLabel[kInfoRows];
    MyGUI::TextBox* infoValue[kInfoRows];
    MyGUI::EditBox* body;
    MyGUI::EditBox* reason;
};

Popup g_ui = { 0 };

MyGUI::delegates::IDelegate2<MyGUI::Window*, const std::string&>* g_closeDelegate = NULL;
MyGUI::delegates::IDelegate1<MyGUI::Widget*>* g_acceptDelegate = NULL;
MyGUI::delegates::IDelegate1<MyGUI::Widget*>* g_declineDelegate = NULL;
MyGUI::delegates::IDelegate1<MyGUI::Widget*>* g_extraDelegate = NULL;

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
    MakeFlatButtonPlates(c, kLine, &kGoldDim, &p.cancel);

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

    MakeFlatButtonCaption(c, kAcceptName, kMediumFont, kLacquer, &p.accept);
    MakeFlatButtonCaption(c, kExtraName, kMediumFont, kAmber, &p.extra);
    MakeFlatButtonCaption(c, kDeclineName, kMediumFont, kText, &p.cancel);

    // The caption is the engine's title bar; only its colour is the mockup's.
    MyGUI::TextBox* caption = p.root->getCaptionWidget();
    if (caption != NULL)
    {
        caption->setTextColour(kBrass);
    }

    return p.plate != NULL && p.body != NULL && p.accept.button != NULL &&
           p.extra.button != NULL && p.cancel.button != NULL;
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

bool BindButtons()
{
    TR_TRY
    {
        // Kenshi_WindowCX draws a close cross in its title bar; it means the
        // same as Cancel -- the player walked away without deciding.
        g_closeDelegate = MyGUI::newDelegate(&OnWindowButton);
        g_ui.root->eventWindowButtonPressed += g_closeDelegate;

        g_acceptDelegate = MyGUI::newDelegate(&OnAccept);
        g_declineDelegate = MyGUI::newDelegate(&OnDecline);
        g_extraDelegate = MyGUI::newDelegate(&OnExtra);
        g_ui.accept.button->eventMouseButtonClick += g_acceptDelegate;
        g_ui.cancel.button->eventMouseButtonClick += g_declineDelegate;
        g_ui.extra.button->eventMouseButtonClick += g_extraDelegate;
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
    if (g_ui.root != NULL && g_closeDelegate != NULL)
    {
        g_ui.root->eventWindowButtonPressed -= g_closeDelegate;
    }
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
                               p.name, p.tag, p.body, p.reason };
    for (size_t i = 0; i < sizeof(fills) / sizeof(fills[0]); ++i) Hide(fills[i]);
    for (int i = 0; i < 4; ++i) { Hide(p.portraitFrame[i]); Hide(p.tagFrame[i]); }
    for (int i = 0; i < kInfoRows; ++i) { Hide(p.infoLabel[i]); Hide(p.infoValue[i]); }
    ShowButton(p.accept, false);
    ShowButton(p.extra, false);
    ShowButton(p.cancel, false);
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
    if (view.showPurse)
    {
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

    // Buttons: Accept, Extra, Cancel share the row; Cancel alone sits right.
    y += kSectionGap;
    const FlatButton* order[3] = { NULL, NULL, NULL };
    const std::string* captions[3] = { NULL, NULL, NULL };
    int count = 0;
    if (!view.acceptLabel.empty()) { order[count] = &p.accept; captions[count] = &view.acceptLabel; ++count; }
    if (!view.extraLabel.empty())  { order[count] = &p.extra;  captions[count] = &view.extraLabel;  ++count; }
    order[count] = &p.cancel;
    captions[count] = &view.cancelLabel;
    ++count;
    if (count == 1)
    {
        SetButtonCaption(p.cancel, view.cancelLabel);
        PlaceButton(p.cancel, W - m - kAloneButtonW, y, kAloneButtonW, kButtonHeight);
    }
    else
    {
        const int bw = (W - 2 * m - (count - 1) * kButtonGap) / count;
        for (int i = 0; i < count; ++i)
        {
            SetButtonCaption(*order[i], *captions[i]);
            PlaceButton(*order[i], m + i * (bw + kButtonGap), y, bw, kButtonHeight);
        }
    }
    const int H = y + kButtonHeight + m;

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
    SafeSetVisible(g_ui.root, false);
    // Whatever way the window ends -- an answer, the title-bar cross, the HUD
    // hiding it, shutdown -- no plea is on screen any more. A pending plea
    // blocks every later one, so leaving it set meant no one could beg again
    // until a reload. The handlers below read IsMercyPending BEFORE calling
    // Close for the same reason.
    ForgetMercy(NULL);
}

void OnAccept(MyGUI::Widget* /*sender*/)
{
    Character* npc = g_subject;
    const bool plea = IsMercyPending(npc);
    Close();

    if (npc != NULL)
    {
        // A plea for mercy is answered by its own rule, not by the price
        // evaluation every other offer goes through.
        if (plea)
        {
            AcceptMercy(npc);
        }
        else
        {
            CompleteOffer(npc, true);
        }
    }
}

// MyGUI names the title-bar buttons by their skin; "close" is the cross.
void OnWindowButton(MyGUI::Window* /*sender*/, const std::string& button)
{
    if (button == "close")
    {
        OnDecline(NULL);
    }
}

void OnDecline(MyGUI::Widget* /*sender*/)
{
    Character* npc = g_subject;
    const bool plea = IsMercyPending(npc);
    Close();

    if (npc != NULL)
    {
        // Refusing a plea is a real answer: nothing changes, they stay hostile,
        // and the fight goes on. It is not the same as the captive decline,
        // which only reports that they were left where they are.
        if (plea)
        {
            TellPlayer(CharacterName(npc) + ": ignoraste su s\xc3\xbaplica.", false);
        }
        else
        {
            CompleteOffer(npc, false);
        }
    }
}

// The extra button acts on the refusal instead of answering it -- feeding a
// starving captive, for one -- then reopens so the player sees the new verdict
// without having to click through the menu again.
void OnExtra(MyGUI::Widget* /*sender*/)
{
    Character* npc = g_subject;
    const bool plea = IsMercyPending(npc);
    Close();

    if (npc != NULL)
    {
        // The third button is context: it feeds a starving captive, but for
        // someone pleading over a lost limb it is the one answer that is
        // neither joining nor killing.
        if (plea)
        {
            GrantMercy(npc);
        }
        else
        {
            HelpCaptive(npc);
        }
    }
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

bool ShowOffer(Character* npc, const OfferView& view)
{
    if (npc == NULL)
    {
        return false;
    }

    // Opening a window replaces whatever was on screen, plea included, and the
    // buttons below route on whether a plea is pending. A plea that outlived
    // its window would turn this offer's ACEPTAR into a free join. OfferMercy
    // marks its plea AFTER this call succeeds, for the same reason.
    ForgetMercy(NULL);

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
    g_closeDelegate = NULL;
}
} // namespace totalrecruit
