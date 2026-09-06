#include "common.h"
#include "offer_popup.h"
#include "rules.h"
#include "config.h"
#include "recruit_core.h"
#include "feeding.h"
#include "log.h"

#include <kenshi/gui/PortraitManager.h>
#include <mygui/MyGUI_ImageBox.h>

namespace totalrecruit
{
namespace
{
// Distinct names so this window can coexist with the context-menu overlay and
// with other mods' widgets.
const char* const kRootName    = "TR_OfferRoot";
const char* const kTextName    = "TR_OfferText";
const char* const kAcceptName  = "TR_OfferAccept";
const char* const kDeclineName = "TR_OfferDecline";
const char* const kExtraName   = "TR_OfferExtra";
const char* const kPortraitName = "TR_OfferPortrait";

// The native context menu sits on the Popup layer, so this goes above it.
const char* const kLayer = "Info";

const int kWidth = 460;
const int kHeight = 230;
const int kMargin = 14;
const int kButtonHeight = 30;
const int kTextHeight = 96;

MyGUI::Window* g_root = NULL;
MyGUI::Widget* g_client = NULL;
MyGUI::EditBox* g_text = NULL;
MyGUI::ImageBox* g_portrait = NULL;
MyGUI::Button* g_accept = NULL;
MyGUI::Button* g_decline = NULL;
MyGUI::Button* g_extra = NULL;

MyGUI::delegates::IDelegate1<MyGUI::Widget*>* g_acceptDelegate = NULL;
MyGUI::delegates::IDelegate1<MyGUI::Widget*>* g_declineDelegate = NULL;
MyGUI::delegates::IDelegate1<MyGUI::Widget*>* g_extraDelegate = NULL;

Character* g_subject = NULL;
bool g_open = false;

// Every MyGUI call materialises IntCoord and UString temporaries, so the real
// work lives in plain functions and only the call crosses the guard. C2712
// counts every C++ object in the whole frame, not just those inside __try.

// setCaption lives on TextBox, not Widget; Button derives from it.
void ApplyCaption(MyGUI::TextBox* widget, const char* text)
{
    widget->setCaption(text);
}

void SafeSetCaption(MyGUI::TextBox* widget, const char* text)
{
    if (widget == NULL) return;
    TR_TRY { ApplyCaption(widget, text); }
    TR_CATCH { }
}



// setOnlyText avoids the caption's markup parsing, which would swallow any
// stray formatting characters in a character name.
void ApplyOnlyText(MyGUI::EditBox* box, const char* text)
{
    box->setOnlyText(text);
}

void SafeSetOnlyText(MyGUI::EditBox* box, const char* text)
{
    if (box == NULL) return;
    TR_TRY { ApplyOnlyText(box, text); }
    TR_CATCH { }
}

void SafeSetVisible(MyGUI::Widget* widget, bool visible)
{
    if (widget == NULL) return;
    TR_TRY { widget->setVisible(visible); }
    TR_CATCH { }
}

// Kenshi_WindowCX is a real MyGUI::Window with title-bar chrome, so it has to
// be created as one and its children parented to getClientWidget() -- placing
// them on the frame itself puts them behind the caption bar and off the edges.
MyGUI::Window* CreateRoot(MyGUI::Gui* guiManager, const char* skin, int left, int top)
{
    return guiManager->createWidget<MyGUI::Window>(
        skin,
        MyGUI::IntCoord(left, top, kWidth, kHeight),
        MyGUI::Align::Default,
        kLayer,
        kRootName);
}

MyGUI::Window* SafeCreateRoot(MyGUI::Gui* guiManager, const char* skin, int left, int top)
{
    TR_TRY { return CreateRoot(guiManager, skin, left, top); }
    TR_CATCH { return NULL; }
}

// The client area excludes the title bar, so children laid out inside it land
// where they look like they should.
MyGUI::Widget* SafeGetClient(MyGUI::Window* window)
{
    TR_TRY
    {
        MyGUI::Widget* client = window->getClientWidget();
        return (client != NULL) ? client : window;
    }
    TR_CATCH { return NULL; }
}

// Width and height of the client area, which is smaller than the window.
bool SafeClientSize(MyGUI::Widget* client, int* outWidth, int* outHeight)
{
    TR_TRY
    {
        *outWidth = client->getWidth();
        *outHeight = client->getHeight();
        return true;
    }
    TR_CATCH { return false; }
}

// An EditBox rather than a TextBox, because only EditBox can word-wrap: a
// TextBox draws one line and clips whatever does not fit, which is how the
// message ended up cut off mid-sentence. Its editing behaviour is switched off
// below so it still reads as a label.
MyGUI::EditBox* CreateText(MyGUI::Widget* parent, const char* skin,
                           int left, int top, int width, int height)
{
    return parent->createWidget<MyGUI::EditBox>(
        skin,
        MyGUI::IntCoord(left, top, width, height),
        MyGUI::Align::Default,
        kTextName);
}

MyGUI::EditBox* SafeCreateText(MyGUI::Widget* parent, const char* skin,
                               int left, int top, int width, int height)
{
    TR_TRY { return CreateText(parent, skin, left, top, width, height); }
    TR_CATCH { return NULL; }
}

// The portrait uses the engine's own PortraitManager, so the face in this
// window is the same one the squad bar and the vanilla dialogue draw -- no
// second rendering path to keep in step.
MyGUI::ImageBox* CreatePortrait(MyGUI::Widget* parent, int left, int top, int side)
{
    return parent->createWidget<MyGUI::ImageBox>(
        "ImageBox",
        MyGUI::IntCoord(left, top, side, side),
        MyGUI::Align::Default,
        kPortraitName);
}

MyGUI::ImageBox* SafeCreatePortrait(MyGUI::Widget* parent, int left, int top, int side)
{
    TR_TRY { return CreatePortrait(parent, left, top, side); }
    TR_CATCH { return NULL; }
}

// createPortraitImage hands back a PortraitImage that knows how to paint itself
// into an ImageBox, which is exactly what the squad bar does with it.
void LoadPortrait(Character* npc, int* outStage)
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

    image->updateImageWidget(g_portrait, true);
    *outStage = 4;
}

int GuardedLoadPortrait(Character* npc)
{
    int stage = 0;
    TR_TRY { LoadPortrait(npc, &stage); }
    TR_CATCH { return -stage; }
    return stage;
}

void SafeLoadPortrait(Character* npc)
{
    if (g_portrait == NULL || npc == NULL)
    {
        return;
    }

    // The stage says how far the load got, so a blank frame can be told apart
    // from a manager that was never there. The guarded call keeps a POD-only
    // frame; the reporting happens outside it.
    const int stage = GuardedLoadPortrait(npc);

    const bool loaded = (stage == 4);
    if (!loaded && GetConfig().probeOnClick)
    {
        char stageText[16] = { 0 };
        _snprintf_s(stageText, sizeof(stageText), _TRUNCATE, "%d", stage);
        LogInfo("Portrait not drawn (stage " + std::string(stageText) + ").");
    }

    // A missing portrait is not worth failing the window over; the frame just
    // stays empty.
    SafeSetVisible(g_portrait, loaded);
}

MyGUI::Button* CreateButton(MyGUI::Widget* parent, const char* skin, const char* name,
                            int left, int top, int width)
{
    return parent->createWidget<MyGUI::Button>(
        skin,
        MyGUI::IntCoord(left, top, width, kButtonHeight),
        MyGUI::Align::Default,
        name);
}

MyGUI::Button* SafeCreateButton(MyGUI::Widget* parent, const char* skin, const char* name,
                                int left, int top, int width)
{
    TR_TRY { return CreateButton(parent, skin, name, left, top, width); }
    TR_CATCH { return NULL; }
}

// The decline button spans the row on its own when there is no accept button
// beside it, so a lone option does not sit oddly in half the window.
void LayoutDecline(bool sharingRow)
{
    int clientWidth = kWidth;
    int clientHeight = kHeight;
    clientWidth = g_client->getWidth();
    clientHeight = g_client->getHeight();

    const int buttonTop = clientHeight - kMargin - kButtonHeight;
    const int halfWidth = (clientWidth - kMargin * 3) / 2;

    if (sharingRow)
    {
        g_decline->setCoord(kMargin * 2 + halfWidth, buttonTop, halfWidth, kButtonHeight);
    }
    else
    {
        g_decline->setCoord(kMargin, buttonTop, clientWidth - kMargin * 2, kButtonHeight);
    }
}

void SafeLayoutDecline(bool sharingRow)
{
    if (g_decline == NULL || g_client == NULL) return;
    TR_TRY { LayoutDecline(sharingRow); }
    TR_CATCH { }
}

// Word-wrapped, read-only, and never taking focus: a label that happens to be
// an EditBox because that is the only widget here that wraps.
bool ConfigureText()
{
    TR_TRY
    {
        g_text->setEditStatic(true);
        g_text->setEditReadOnly(true);
        g_text->setEditMultiLine(true);
        g_text->setEditWordWrap(true);
        g_text->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Top);
        g_text->setNeedKeyFocus(false);
        g_text->setNeedMouseFocus(false);
        g_text->setVisibleVScroll(false);
        g_text->setVisibleHScroll(false);
        return true;
    }
    TR_CATCH { return false; }
}

void OnAccept(MyGUI::Widget* sender);
void OnDecline(MyGUI::Widget* sender);
void OnExtra(MyGUI::Widget* sender);

bool BindButtons()
{
    TR_TRY
    {
        g_acceptDelegate = MyGUI::newDelegate(&OnAccept);
        g_declineDelegate = MyGUI::newDelegate(&OnDecline);
        g_accept->eventMouseButtonClick += g_acceptDelegate;
        g_decline->eventMouseButtonClick += g_declineDelegate;
        if (g_extra != NULL)
        {
            g_extraDelegate = MyGUI::newDelegate(&OnExtra);
            g_extra->eventMouseButtonClick += g_extraDelegate;
        }
        return true;
    }
    TR_CATCH { return false; }
}

void Unbind()
{
    if (g_accept != NULL && g_acceptDelegate != NULL)
    {
        g_accept->eventMouseButtonClick -= g_acceptDelegate;
    }
    if (g_decline != NULL && g_declineDelegate != NULL)
    {
        g_decline->eventMouseButtonClick -= g_declineDelegate;
    }
    if (g_extra != NULL && g_extraDelegate != NULL)
    {
        g_extra->eventMouseButtonClick -= g_extraDelegate;
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

// Centred horizontally, a little above centre vertically, so it does not sit on
// top of whatever the player just clicked.
void ChoosePosition(int* outLeft, int* outTop)
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

    *outLeft = (viewWidth > kWidth) ? (viewWidth - kWidth) / 2 : 40;
    *outTop = (viewHeight > kHeight) ? (viewHeight / 2) - kHeight : 40;
    if (*outTop < 0)
    {
        *outTop = 40;
    }
}

bool Build()
{
    if (g_root != NULL)
    {
        return true;
    }

    MyGUI::Gui* guiManager = MyGUI::Gui::getInstancePtr();
    if (guiManager == NULL)
    {
        return false;
    }

    int left = 0;
    int top = 0;
    ChoosePosition(&left, &top);

    const char* panelSkins[3] = { "Kenshi_WindowCX", "WindowCSX", "WindowC" };
    for (int i = 0; i < 3 && g_root == NULL; ++i)
    {
        g_root = SafeCreateRoot(guiManager, panelSkins[i], left, top);
    }
    if (g_root == NULL)
    {
        LogError("Could not create the offer window (no usable window skin).");
        return false;
    }

    g_client = SafeGetClient(g_root);
    if (g_client == NULL)
    {
        LogError("Offer window has no client area.");
        return false;
    }

    // Lay out against the client area's real size rather than the window's, so
    // the title bar does not push everything off the bottom edge.
    int clientWidth = kWidth;
    int clientHeight = kHeight;
    SafeClientSize(g_client, &clientWidth, &clientHeight);

    // A portrait square on the left, text to its right -- the same shape the
    // vanilla dialogue window uses, so the two read as the same kind of moment.
    const int portraitSide = 84;
    const int textLeft = kMargin + portraitSide + kMargin;
    const int textWidth = clientWidth - textLeft - kMargin;
    const int buttonTop = clientHeight - kMargin - kButtonHeight;
    const int textHeight = buttonTop - kMargin * 2;
    const int buttonWidth = (clientWidth - kMargin * 3) / 2;

    const char* textSkins[3] = { "Kenshi_EditBox", "EditBox", "TextBox" };
    for (int i = 0; i < 3 && g_text == NULL; ++i)
    {
        g_text = SafeCreateText(g_client, textSkins[i], textLeft, kMargin,
                                textWidth, textHeight);
    }

    g_portrait = SafeCreatePortrait(g_client, kMargin, kMargin, portraitSide);

    const char* buttonSkins[2] = { "Kenshi_Button1", "Button" };
    for (int i = 0; i < 2 && g_accept == NULL; ++i)
    {
        g_accept = SafeCreateButton(g_client, buttonSkins[i], kAcceptName,
                                    kMargin, buttonTop, buttonWidth);
    }
    for (int i = 0; i < 2 && g_decline == NULL; ++i)
    {
        g_decline = SafeCreateButton(g_client, buttonSkins[i], kDeclineName,
                                     kMargin * 2 + buttonWidth, buttonTop, buttonWidth);
    }

    // The third button sits on its own row above the other two, so a long label
    // like "Give them food (Foodcube)" is not squeezed into a third of a row.
    const int extraTop = buttonTop - kButtonHeight - kMargin / 2;
    for (int i = 0; i < 2 && g_extra == NULL; ++i)
    {
        g_extra = SafeCreateButton(g_client, buttonSkins[i], kExtraName,
                                   kMargin, extraTop, clientWidth - kMargin * 2);
    }

    if (g_accept == NULL || g_decline == NULL)
    {
        LogError("Could not create the offer buttons.");
        return false;
    }

    if (g_text != NULL)
    {
        ConfigureText();
    }
    BindButtons();

    SafeSetVisible(g_root, false);
    return true;
}

void Close()
{
    g_open = false;
    g_subject = NULL;
    SafeSetVisible(g_root, false);
}

void OnAccept(MyGUI::Widget* /*sender*/)
{
    Character* npc = g_subject;
    Close();

    if (npc != NULL)
    {
        CompleteOffer(npc, true);
    }
}

void OnDecline(MyGUI::Widget* /*sender*/)
{
    Character* npc = g_subject;
    Close();

    if (npc != NULL)
    {
        CompleteOffer(npc, false);
    }
}

// The extra button acts on the refusal instead of answering it -- feeding a
// starving captive, for one -- then reopens so the player sees the new verdict
// without having to click through the menu again.
void OnExtra(MyGUI::Widget* /*sender*/)
{
    Character* npc = g_subject;
    Close();

    if (npc != NULL)
    {
        HelpCaptive(npc);
    }
}
} // namespace

bool IsOfferOpen()
{
    return g_open;
}

void HideOffer()
{
    Close();
}

bool ShowOffer(Character* npc, const std::string& npcLine,
               const std::string& acceptLabel, const std::string& declineLabel,
               const std::string& extraLabel)
{
    if (npc == NULL)
    {
        return false;
    }

    if (!Build())
    {
        return false;
    }

    g_subject = npc;
    g_open = true;

    // A fixed title says what kind of window this is; who it is about is in
    // the body, where there is room for a full name.
    SafeSetCaption(g_root, "Reclutamiento");
    SafeLoadPortrait(npc);
    SafeSetOnlyText(g_text, npcLine.c_str());
    // An empty accept label means there is nothing to agree to, so that button
    // is hidden and the decline one widens to fill the row.
    const bool haveAccept = !acceptLabel.empty();
    if (haveAccept)
    {
        SafeSetCaption(g_accept, acceptLabel.c_str());
    }
    SafeSetVisible(g_accept, haveAccept);
    SafeLayoutDecline(haveAccept);

    SafeSetCaption(g_decline, declineLabel.c_str());

    // The third row only appears when there is actually something to offer.
    const bool haveExtra = !extraLabel.empty();
    if (haveExtra)
    {
        SafeSetCaption(g_extra, extraLabel.c_str());
    }
    SafeSetVisible(g_extra, haveExtra);

    SafeSetVisible(g_root, true);

    return true;
}

void ShutdownOfferPopup()
{
    Close();
    SafeUnbind();

    // Children go with the root, so only the root is destroyed.
    SafeDestroyWidget(g_root);

    g_root = NULL;
    g_text = NULL;
    g_portrait = NULL;
    g_accept = NULL;
    g_decline = NULL;
    g_extra = NULL;
    g_acceptDelegate = NULL;
    g_declineDelegate = NULL;
    g_extraDelegate = NULL;
}
} // namespace totalrecruit
