#include "common.h"
#include "recruit_overlay.h"
#include "recruit_core.h"
#include "defeats.h"
#include "config.h"
#include "log.h"
#include "probe.h"
#include "rules.h"
#include "offer_popup.h"

namespace totalrecruit
{
namespace
{
// Kenshi's context menu cannot be given extra rows from a plugin without
// patching the row-building loop by address. Loot-Scoot-Execute did exactly
// that and removed it again (commit 3908f54, -6237 lines), so this plugin takes
// the approach that replaced it: a MyGUI button anchored to the native menu's
// own OptionsList widget, which needs no addresses beyond the two hooks below.

const char* const kButtonWidgetName = "TR_RecruitButton";
const char* const kButtonLayer = "Popup";
const char* const kPreferredSkin = "Kenshi_Button1";
const char* const kFallbackSkin = "Button";

const int kButtonHeight = 26;
const int kButtonGap = 2;
const int kMinButtonWidth = 120;

MyGUI::Button* g_button = NULL;
MyGUI::delegates::IDelegate4<MyGUI::Widget*, int, int, MyGUI::MouseButton>*
    g_buttonDelegate = NULL;

// Target of the currently open menu. Only ever dereferenced behind TR_TRY.
RootObject* g_targetObject = NULL;
Character* g_targetCharacter = NULL;

void (*g_showContextMenu_orig)(ContextMenu*, bool, RootObject*) = NULL;
void (*g_contextMenuUpdate_orig)(ContextMenu*) = NULL;

// The guarded function's frame must hold no C++ objects at all -- not just
// inside the __try block -- so the UString is built by the caller.
void ApplyCaption(MyGUI::TextBox* widget, const char* text)
{
    widget->setCaption(text);
}

void SafeSetCaption(MyGUI::TextBox* widget, const char* text)
{
    TR_TRY { ApplyCaption(widget, text); }
    TR_CATCH { }
}

void ClearTarget()
{
    g_targetObject = NULL;
    g_targetCharacter = NULL;
}

void HideButton()
{
    if (g_button != NULL)
    {
        TR_TRY { g_button->setVisible(false); }
        TR_CATCH { }
    }
}

void OnRecruitClicked(MyGUI::Widget* /*sender*/, int /*left*/, int /*top*/,
                      MyGUI::MouseButton /*button*/)
{
    Character* target = g_targetCharacter;
    ClearTarget();
    HideButton();

    if (target == NULL)
    {
        return;
    }

    AttemptRecruit(target);
}

// Split out so the guarded frame holds no C++ objects; MSVC rejects __try in a
// function that also unwinds them.
bool ConfigureButton()
{
    TR_TRY
    {
        g_button->setNeedMouseFocus(true);
        g_button->setNeedKeyFocus(false);
        g_button->setVisible(false);
        g_buttonDelegate = MyGUI::newDelegate(&OnRecruitClicked);
        g_button->eventMouseButtonPressed += g_buttonDelegate;
        return true;
    }
    TR_CATCH { return false; }
}

// createWidget materialises IntCoord and UString temporaries, so the guard sits
// in its own frame; C2712 counts every object in the function.
MyGUI::Button* CreateButtonWidget(MyGUI::Gui* guiManager, const char* skin)
{
    return guiManager->createWidget<MyGUI::Button>(
        skin,
        MyGUI::IntCoord(0, 0, kMinButtonWidth, kButtonHeight),
        MyGUI::Align::Default,
        kButtonLayer,
        kButtonWidgetName);
}

MyGUI::Button* SafeCreateButton(MyGUI::Gui* guiManager, const char* skin)
{
    TR_TRY { return CreateButtonWidget(guiManager, skin); }
    TR_CATCH { return NULL; }
}

bool CreateButton()
{
    if (g_button != NULL)
    {
        return true;
    }

    MyGUI::Gui* guiManager = MyGUI::Gui::getInstancePtr();
    if (guiManager == NULL)
    {
        return false;
    }

    // Skin availability differs between installs, so fall back to the stock one.
    const char* skins[2] = { kPreferredSkin, kFallbackSkin };
    for (int i = 0; i < 2 && g_button == NULL; ++i)
    {
        g_button = SafeCreateButton(guiManager, skins[i]);
    }

    if (g_button == NULL)
    {
        LogError("Could not create the recruit button (no usable skin).");
        return false;
    }

    if (!ConfigureButton())
    {
        LogError("Could not configure the recruit button.");
        return false;
    }

    return true;
}

// Each of these keeps its own POD-only frame; MyGUI's event operators and
// widget destruction both create objects that would otherwise need unwinding.
void Unsubscribe()
{
    if (g_buttonDelegate != NULL)
    {
        g_button->eventMouseButtonPressed -= g_buttonDelegate;
    }
}

void SafeUnsubscribe()
{
    TR_TRY { Unsubscribe(); }
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
    TR_TRY { DestroyWidget(widget); }
    TR_CATCH { }
}

void DestroyButton()
{
    if (g_button == NULL)
    {
        return;
    }

    SafeUnsubscribe();
    SafeDestroyWidget(g_button);

    g_button = NULL;
    g_buttonDelegate = NULL;
}

// Places the button beside the native menu's option list, so it reads as part
// of the menu without competing for the space below it.
bool AnchorToMenu(ContextMenu* menu)
{
    if (menu == NULL || g_button == NULL)
    {
        return false;
    }

    // ContextMenuGUI is only forward-declared here (its full header would
    // redefine ContextMenu), so optionsList is read at its documented offset
    // from kenshi/gui/ContextMenu.h:27.
    const size_t kOptionsListOffset = 0xF8;

    MyGUI::IntCoord optionsCoord;
    TR_TRY
    {
        void* menuGUI = menu->menuGUI;
        if (menuGUI == NULL)
        {
            return false;
        }

        MyGUI::Widget* optionsList = *reinterpret_cast<MyGUI::Widget**>(
            reinterpret_cast<char*>(menuGUI) + kOptionsListOffset);
        if (optionsList == NULL)
        {
            return false;
        }
        optionsCoord = optionsList->getAbsoluteCoord();
    }
    TR_CATCH { return false; }

    if (optionsCoord.width <= 0)
    {
        return false;
    }

    // The caption is a single word now, so matching the menu's full width would
    // leave a mostly empty button. Narrower also means less of the world is
    // covered by it.
    int width = kMinButtonWidth;
    if (optionsCoord.width < width)
    {
        width = optionsCoord.width;
    }

    // Sit to the right of the menu rather than below it: other mods
    // (Loot-Scoot-Execute among them) anchor their own overlay underneath, and
    // stacking there would cover theirs.
    int left = optionsCoord.left + optionsCoord.width + kButtonGap;
    const int top = optionsCoord.top;

    // Flip to the left side when the menu opens near the right screen edge.
    // getViewSize lives on RenderManager, not on Gui.
    int viewWidth = 0;
    TR_TRY
    {
        MyGUI::RenderManager* renderManager = MyGUI::RenderManager::getInstancePtr();
        if (renderManager != NULL)
        {
            viewWidth = renderManager->getViewSize().width;
        }
    }
    TR_CATCH { viewWidth = 0; }

    if (viewWidth > 0 && left + width > viewWidth)
    {
        left = optionsCoord.left - width - kButtonGap;
        if (left < 0)
        {
            left = 0;
        }
    }

    TR_TRY
    {
        g_button->setCoord(left, top, width, kButtonHeight);
        g_button->setVisible(true);
    }
    TR_CATCH { return false; }

    return true;
}

void ContextMenu_showContextMenu_hook(ContextMenu* thisptr, bool on, RootObject* what)
{
    if (g_showContextMenu_orig != NULL)
    {
        g_showContextMenu_orig(thisptr, on, what);
    }

    const Config& config = GetConfig();
    if (!config.enabled || !config.showRecruitButton)
    {
        HideButton();
        ClearTarget();
        return;
    }

    // An offer already on screen owns the decision; a second button behind it
    // would just be a way to bypass the one the player is looking at.
    if (IsOfferOpen())
    {
        HideButton();
        ClearTarget();
        return;
    }

    if (!on)
    {
        HideButton();
        ClearTarget();
        return;
    }

    Character* character = CharacterFromTarget(what);
    if (character == NULL)
    {
        // Logged because "nothing happened on right-click" is otherwise
        // indistinguishable from the plugin not running at all.
        if (GetConfig().probeOnClick)
        {
            LogInfo("Context menu: no character resolved from the target.");
        }
        HideButton();
        ClearTarget();
        return;
    }

    // Diagnostic dump of everything the rule engine reads. It runs on the
    // right-click, before the recruitable filter, because the states worth
    // measuring are exactly the ones that get filtered out: an unconscious or
    // excluded character never grows a "Talk" button, so hanging the probe off
    // that button made the interesting cases unreachable.
    if (GetConfig().probeOnClick)
    {
        ProbeCharacter(character);
    }

    // Someone the player finds unconscious was put down by somebody, and the
    // knockout hook does not fire for every way that happens. Checked here
    // because this is the moment the player is actually looking at them.
    NoteIfDown(character);

    std::string excluded;
    if (!IsRecruitable(character, &excluded))
    {
        if (GetConfig().probeOnClick)
        {
            LogInfo("Context menu: " + CharacterName(character) +
                    " skipped (" + excluded + ").");
        }
        HideButton();
        ClearTarget();
        return;
    }

    g_targetObject = what;
    g_targetCharacter = character;

    if (!CreateButton())
    {
        ClearTarget();
        return;
    }

    // The label carries the verdict, so the player learns what an NPC wants
    // without having to click and be refused. Telling them WHY is the feature.
    const Verdict verdict = Evaluate(character);
    if (config.probeOnClick)
    {
        LogInfo("Context menu: " + CharacterName(character) + " -> " +
                (verdict.willJoin ? "CAN JOIN" : "refuses") +
                " (" + CurrencyName(verdict.currency) + ") " + verdict.reason);
    }

    // Just the verb. The name is already in the menu this button is attached
    // to, and the price and the reason are in the window it opens -- packing
    // them in here made a caption too long to read, which is worse than saying
    // less.
    SafeSetCaption(g_button, config.recruitLabel.c_str());

    // The native menu sizes itself during show, so the first anchor attempt can
    // read a stale rect; update() re-anchors every frame while it is open.
    AnchorToMenu(thisptr);
}

void ContextMenu_update_hook(ContextMenu* thisptr)
{
    if (g_contextMenuUpdate_orig != NULL)
    {
        g_contextMenuUpdate_orig(thisptr);
    }

    if (g_button == NULL || g_targetCharacter == NULL || thisptr == NULL)
    {
        return;
    }

    bool visible = false;
    TR_TRY { visible = thisptr->isVisible(); }
    TR_CATCH { visible = false; }

    if (!visible)
    {
        HideButton();
        ClearTarget();
        return;
    }

    AnchorToMenu(thisptr);
}
} // namespace

bool InstallRecruitOverlay()
{
    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&ContextMenu::showContextMenu),
            ContextMenu_showContextMenu_hook,
            &g_showContextMenu_orig))
    {
        LogError("Failed to hook ContextMenu::showContextMenu.");
        return false;
    }

    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&ContextMenu::update),
            ContextMenu_update_hook,
            &g_contextMenuUpdate_orig))
    {
        LogError("Failed to hook ContextMenu::update.");
        return false;
    }

    LogInfo("Recruit overlay installed.");
    return true;
}

void ForgetOverlayTarget()
{
    ClearTarget();
    HideButton();
}

void ShutdownRecruitOverlay()
{
    ClearTarget();
    DestroyButton();
}
} // namespace totalrecruit
