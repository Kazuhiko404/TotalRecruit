#include "common.h"
#include "recruit_overlay.h"
#include "recruit_core.h"
#include "engine_reads.h"
#include "defeats.h"
#include "config.h"
#include "log.h"
#include "probe.h"
#include "rules.h"
#include "offer_popup.h"
#include "readout.h"
#include "button_block.h"
#include "menu_button_release.h"

namespace totalrecruit
{
namespace
{
// Kenshi's context menu cannot be given extra rows from a plugin without
// patching the row-building loop by address. Loot-Scoot-Execute did exactly
// that and removed it again (commit 3908f54, -6237 lines), so this plugin takes
// the approach that replaced it: a MyGUI button anchored to the native menu's
// own OptionsList widget, which needs no addresses beyond the two hooks below.

// Size, skin and look come from button_style.h (the set TotalSparring and
// TotalLoot share); where it goes comes from button_block / menu_slot.
const char* const kButtonWidgetName = "TR_RecruitButton";

typedef MyGUI::delegates::IDelegate4<MyGUI::Widget*, int, int, MyGUI::MouseButton> MouseDelegate;

MyGUI::Button* g_button = NULL;
// Pressed AND released, as TotalSparring (spar_overlay.cpp:307-417) and TotalLoot
// (context_button_input.cpp) do: Kenshi's menu acts on release, so the player can
// hold the right button from the click that opened the menu, slide onto Hablar and
// let go. Separate instances: MyGUI owns each delegate it is given.
MouseDelegate* g_buttonDelegate = NULL;
MouseDelegate* g_buttonReleaseDelegate = NULL;
// One gesture acts once: a click's press acts and its release is an echo; a
// hold-and-release only delivers the release, which acts (menu_button_release.h).
clickgesture::State g_gesture;

// Target of the currently open menu. Only ever dereferenced behind TR_TRY.
RootObject* g_targetObject = NULL;
Character* g_targetCharacter = NULL;

// Side the button took last frame, kept while the menu stays open so it does
// not hop between sides when another mod's hook runs before or after ours.
menuslot::Side g_previousSide = menuslot::SIDE_NONE;

// Whether Kenshi's right-click menu is on screen, whoever it is about (the readout
// card hides under it). Refreshed by the update hook every frame the menu is open;
// a mark older than kMenuStaleMs means the hook stopped running, so it is closed.
const DWORD kMenuStaleMs = 500;
bool g_menuOpen = false;
DWORD g_menuSeenAt = 0;

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
    g_previousSide = menuslot::SIDE_NONE;
}

void NoteMenuOpen(bool open)
{
    g_menuOpen = open;
    g_menuSeenAt = GetTickCount();
}

void HideButton()
{
    if (g_button != NULL)
    {
        TR_TRY { g_button->setVisible(false); }
        TR_CATCH { }
    }
}

// Left (click) and right (the menu's hold-and-release gesture) act; middle and
// wheel buttons never do.
bool IsActingButton(MyGUI::MouseButton button)
{
    return button == MyGUI::MouseButton::Left || button == MyGUI::MouseButton::Right;
}

// Bound to both pressed and released; the gesture rule lets one of them through.
void OnRecruitClicked(MyGUI::Widget* /*sender*/, int /*left*/, int /*top*/,
                      MyGUI::MouseButton button)
{
    if (!IsActingButton(button))
    {
        return;
    }
    const clickgesture::Verdict verdict = clickgesture::Classify(&g_gesture);
    if (verdict != clickgesture::ACT)
    {
        if (verdict == clickgesture::STRAY && GetConfig().probeOnClick)
        {
            LogInfo("Recruit button event ignored: no armed gesture.");
        }
        return;
    }

    Character* target = g_targetCharacter;
    ClearTarget();
    HideButton();

    if (target == NULL)
    {
        return;
    }

    AttemptRecruit(target);

    // The readout card is asked for by this click and nothing else (not the
    // rescue path, which also calls AttemptRecruit): it stands beside the offer
    // the attempt just opened, worked out after the attempt so both agree.
    RequestReadout(target);
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
        g_buttonReleaseDelegate = MyGUI::newDelegate(&OnRecruitClicked);
        g_button->eventMouseButtonReleased += g_buttonReleaseDelegate;
        return true;
    }
    TR_CATCH { return false; }
}

bool CreateButton()
{
    if (g_button != NULL)
    {
        return true;
    }

    // Styled like the other mods' buttons; tries Kenshi_Button1, then the
    // stock skin, since skin availability differs between installs.
    g_button = CreateContextButton(kButtonWidgetName);

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
    if (g_buttonReleaseDelegate != NULL)
    {
        g_button->eventMouseButtonReleased -= g_buttonReleaseDelegate;
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
    g_buttonReleaseDelegate = NULL;
}

// TotalSparring sits right of the menu top-aligned without avoiding anything,
// TotalLoot takes a free slot, Loot-Scoot-Execute anchors below: the slot
// resolver tries left, then right, then each pushed below what is already
// there, and only then a clamped fallback (menu_slot.h). Its own frame because
// the block vector needs unwinding, which cannot share a frame with __try.
bool PlaceBeside(const MyGUI::IntCoord& optionsCoord)
{
    std::vector<MyGUI::Button*> block(1, g_button);
    return PlaceButtonBlock(optionsCoord, block,
                            MakeBlockStyle(optionsCoord.width), &g_previousSide);
}

// Places the button in a free slot beside the native menu's option list, so it
// reads as part of the menu without covering the buttons other mods put there.
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

    return PlaceBeside(optionsCoord);
}

void ContextMenu_showContextMenu_hook(ContextMenu* thisptr, bool on, RootObject* what)
{
    if (g_showContextMenu_orig != NULL)
    {
        g_showContextMenu_orig(thisptr, on, what);
    }
    NoteMenuOpen(on);

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
    // A freshly opened menu picks its side from scratch.
    g_previousSide = menuslot::SIDE_NONE;

    if (!CreateButton())
    {
        ClearTarget();
        return;
    }

    // A full evaluation only feeds this diagnostic line, so it only runs when
    // the line is wanted; the verdict itself is worked out when Hablar is pressed.
    if (config.probeOnClick)
    {
        const Verdict verdict = Evaluate(character);
        LogInfo("Context menu: " + CharacterName(character) + " -> " +
                (verdict.willJoin ? "CAN JOIN" : "refuses") +
                " (" + CurrencyName(verdict.currency) + ") " + verdict.reason);
    }

    // Just the verb. The name is already in the menu this button is attached
    // to, and the price and the reason are in the window it opens -- packing
    // them in here made a caption too long to read, which is worse than saying
    // less.
    SafeSetCaption(g_button, config.recruitLabel.c_str());

    // A fresh menu: the next press or release over Hablar is a new gesture. It never
    // fires by itself, so the click that opened the menu cannot trigger the button
    // unless it is released over it.
    clickgesture::Arm(&g_gesture);

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

    if (thisptr == NULL)
    {
        return;
    }

    bool visible = false;
    TR_TRY { visible = thisptr->isVisible(); }
    TR_CATCH { visible = false; }
    NoteMenuOpen(visible);

    if (g_button == NULL || g_targetCharacter == NULL)
    {
        return;
    }

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

bool IsContextMenuOpen()
{
    return g_menuOpen && GetTickCount() - g_menuSeenAt < kMenuStaleMs;
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
