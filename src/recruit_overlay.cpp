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
#include "cage_commands.h"
#include "cage_command_rules.h"
#include "menu_anchor.h"

namespace totalrecruit
{
namespace
{
// Kenshi's context menu cannot be given extra rows from a plugin without
// patching the row-building loop by address. Loot-Scoot-Execute did exactly
// that and removed it again (commit 3908f54, -6237 lines), so this plugin takes
// the approach that replaced it: MyGUI buttons anchored to the native menu's
// own OptionsList widget, which needs no addresses beyond the two hooks below.
//
// The block is [Hablar] for anyone the mod can talk to, and [Hablar, Golpear,
// Noquear] for a prisoner awake in the player's own cage (ComposeContextBlock).
// Every button shares the same style and creation (button_block), its own
// press-and-release gesture (menu_button_release), and one PlaceButtonBlock call
// places them together.

// Size, skin and look come from button_style.h (the set TotalSparring and
// TotalLoot share); where they go comes from button_block / menu_slot.
const char* const kButtonWidgetNames[ContextButton_Count] = {
    "TR_RecruitButton",
    "TR_BeatButton",
    "TR_KnockoutButton"
};

typedef MyGUI::delegates::IDelegate4<MyGUI::Widget*, int, int, MyGUI::MouseButton> MouseDelegate;
typedef void (*MouseHandler)(MyGUI::Widget*, int, int, MyGUI::MouseButton);

MyGUI::Button* g_buttons[ContextButton_Count] = { NULL, NULL, NULL };
// Pressed AND released, as TotalSparring (spar_overlay.cpp:307-417) and TotalLoot
// (context_button_input.cpp) do: Kenshi's menu acts on release, so the player can
// hold the right button from the click that opened the menu, slide onto a button
// and let go. Separate instances: MyGUI owns each delegate it is given.
MouseDelegate* g_pressDelegates[ContextButton_Count] = { NULL, NULL, NULL };
MouseDelegate* g_releaseDelegates[ContextButton_Count] = { NULL, NULL, NULL };
// One gesture per button acts once: a click's press acts and its release is an
// echo; a hold-and-release only delivers the release, which acts
// (menu_button_release.h). Each button arms its own, so the release of a click on
// one never counts as the echo of another.
clickgesture::State g_gestures[ContextButton_Count];

// The buttons of the open menu, top to bottom.
std::vector<ContextButtonKind> g_block;

// Target of the currently open menu. Only ever dereferenced behind TR_TRY.
RootObject* g_targetObject = NULL;
Character* g_targetCharacter = NULL;

// Side the block took last frame, kept while the menu stays open so it does not
// hop between sides when another mod's hook runs before or after ours.
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

void SafeHide(MyGUI::Button* button)
{
    TR_TRY { button->setVisible(false); }
    TR_CATCH { }
}

void HideButtons()
{
    for (int kind = 0; kind < ContextButton_Count; ++kind)
    {
        if (g_buttons[kind] != NULL)
        {
            SafeHide(g_buttons[kind]);
        }
    }
}

bool InBlock(int kind)
{
    for (size_t i = 0; i < g_block.size(); ++i)
    {
        if ((int)g_block[i] == kind)
        {
            return true;
        }
    }
    return false;
}

// Left (click) and right (the menu's hold-and-release gesture) act; middle and
// wheel buttons never do.
bool IsActingButton(MyGUI::MouseButton button)
{
    return button == MyGUI::MouseButton::Left || button == MyGUI::MouseButton::Right;
}

// A cage command that could not be given says why, in the player's words.
void ReportCageCommand(CageCommandResult result)
{
    if (result != CageCommand_Issued)
    {
        TellPlayer(CageCommandProblem(result), true);
    }
}

// Bound to both pressed and released of every button; the gesture rule of that
// button lets one of them through.
void OnButtonEvent(ContextButtonKind kind, MyGUI::MouseButton button)
{
    if (!IsActingButton(button))
    {
        return;
    }
    const clickgesture::Verdict verdict = clickgesture::Classify(&g_gestures[kind]);
    if (verdict != clickgesture::ACT)
    {
        if (verdict == clickgesture::STRAY && GetConfig().probeOnClick)
        {
            LogInfo(std::string("Context button event ignored: no armed gesture (") +
                    kButtonWidgetNames[kind] + ").");
        }
        return;
    }

    Character* target = g_targetCharacter;
    ClearTarget();
    HideButtons();

    if (target == NULL)
    {
        return;
    }

    switch (kind)
    {
    case ContextButton_Beat:
        ReportCageCommand(OrderSelectedToBeat(target));
        return;

    case ContextButton_Knockout:
        ReportCageCommand(OrderSelectedToKnockout(target));
        return;

    case ContextButton_Talk:
    default:
        AttemptRecruit(target);
        // The readout card is asked for by this click and nothing else (not the
        // rescue path, which also calls AttemptRecruit): it stands beside the
        // offer the attempt just opened, worked out after the attempt so both
        // agree.
        RequestReadout(target);
        return;
    }
}

void OnTalkEvent(MyGUI::Widget*, int, int, MyGUI::MouseButton button)
{
    OnButtonEvent(ContextButton_Talk, button);
}

void OnBeatEvent(MyGUI::Widget*, int, int, MyGUI::MouseButton button)
{
    OnButtonEvent(ContextButton_Beat, button);
}

void OnKnockoutEvent(MyGUI::Widget*, int, int, MyGUI::MouseButton button)
{
    OnButtonEvent(ContextButton_Knockout, button);
}

const MouseHandler kHandlers[ContextButton_Count] = {
    &OnTalkEvent,
    &OnBeatEvent,
    &OnKnockoutEvent
};

// Split out so the guarded frame holds no C++ objects; MSVC rejects __try in a
// function that also unwinds them.
bool ConfigureButton(int kind)
{
    TR_TRY
    {
        MyGUI::Button* button = g_buttons[kind];
        button->setNeedMouseFocus(true);
        button->setNeedKeyFocus(false);
        button->setVisible(false);
        g_pressDelegates[kind] = MyGUI::newDelegate(kHandlers[kind]);
        button->eventMouseButtonPressed += g_pressDelegates[kind];
        g_releaseDelegates[kind] = MyGUI::newDelegate(kHandlers[kind]);
        button->eventMouseButtonReleased += g_releaseDelegates[kind];
        return true;
    }
    TR_CATCH { return false; }
}

bool CreateButton(int kind)
{
    if (g_buttons[kind] != NULL)
    {
        return true;
    }

    // Styled like the other mods' buttons; tries Kenshi_Button1, then the
    // stock skin, since skin availability differs between installs.
    g_buttons[kind] = CreateContextButton(kButtonWidgetNames[kind]);

    if (g_buttons[kind] == NULL)
    {
        LogError(std::string("Could not create ") + kButtonWidgetNames[kind] + " (no usable skin).");
        return false;
    }

    if (!ConfigureButton(kind))
    {
        LogError(std::string("Could not configure ") + kButtonWidgetNames[kind] + ".");
        return false;
    }

    return true;
}

// Every button of the block exists, or none is shown: a block missing its first
// button would put Golpear where the player expects Hablar.
bool CreateBlockButtons()
{
    for (size_t i = 0; i < g_block.size(); ++i)
    {
        if (!CreateButton(g_block[i]))
        {
            return false;
        }
    }
    return true;
}

// Each of these keeps its own POD-only frame; MyGUI's event operators and
// widget destruction both create objects that would otherwise need unwinding.
void Unsubscribe(int kind)
{
    if (g_pressDelegates[kind] != NULL)
    {
        g_buttons[kind]->eventMouseButtonPressed -= g_pressDelegates[kind];
    }
    if (g_releaseDelegates[kind] != NULL)
    {
        g_buttons[kind]->eventMouseButtonReleased -= g_releaseDelegates[kind];
    }
}

void SafeUnsubscribe(int kind)
{
    TR_TRY { Unsubscribe(kind); }
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

void DestroyButtons()
{
    for (int kind = 0; kind < ContextButton_Count; ++kind)
    {
        if (g_buttons[kind] == NULL)
        {
            continue;
        }
        SafeUnsubscribe(kind);
        SafeDestroyWidget(g_buttons[kind]);
        g_buttons[kind] = NULL;
        g_pressDelegates[kind] = NULL;
        g_releaseDelegates[kind] = NULL;
    }
}

// TotalSparring sits right of the menu top-aligned without avoiding anything,
// TotalLoot takes a free slot, Loot-Scoot-Execute anchors below: the slot
// resolver tries left, then right, then each pushed below what is already
// there, and only then a clamped fallback (menu_slot.h). Its own frame because
// the block vector needs unwinding, which cannot share a frame with __try.
// `menuCoord` is the rectangle to stay beside (both native columns, menu_anchor);
// `styleWidth` the first column's width the button style is sized from, as before.
bool PlaceBeside(const MyGUI::IntCoord& menuCoord, int styleWidth)
{
    std::vector<MyGUI::Button*> block;
    for (size_t i = 0; i < g_block.size(); ++i)
    {
        MyGUI::Button* button = g_buttons[g_block[i]];
        if (button == NULL)
        {
            return false;
        }
        block.push_back(button);
    }
    if (block.empty())
    {
        return false;
    }
    return PlaceButtonBlock(menuCoord, block, MakeBlockStyle(styleWidth), &g_previousSide);
}

// Places the block in a free slot beside the native menu, so it reads as part of
// the menu without covering the buttons other mods put there. The menu is both of
// its columns when a character inside a building opens two (menu_anchor.h); with
// one column the rectangle is that column's option list, as it always was.
bool AnchorToMenu(ContextMenu* menu)
{
    if (menu == NULL || g_block.empty())
    {
        return false;
    }

    MenuAnchor anchor;
    if (!ReadMenuAnchor(menu, &anchor))
    {
        return false;
    }
    const MyGUI::IntCoord menuCoord(anchor.menu.left, anchor.menu.top, anchor.menu.width,
                                    anchor.menu.height);
    return PlaceBeside(menuCoord, anchor.first.rect.width);
}

void ForgetMenu()
{
    HideButtons();
    ClearTarget();
    g_block.clear();
}

// The words on each button of the block. Hablar's verb only: the name is already
// in the menu, and the price and the reason are in the window it opens -- packing
// them in here made a caption too long to read. Noquear carries the engine's own
// odds for the selected characters.
void CaptionBlock(Character* character)
{
    for (size_t i = 0; i < g_block.size(); ++i)
    {
        const ContextButtonKind kind = g_block[i];
        std::string caption;
        switch (kind)
        {
        case ContextButton_Beat:     caption = BeatButtonLabel(); break;
        case ContextButton_Knockout: caption = KnockoutButtonLabel(character); break;
        case ContextButton_Talk:
        default:                     caption = GetConfig().recruitLabel; break;
        }
        SafeSetCaption(g_buttons[kind], caption.c_str());
    }
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
        ForgetMenu();
        return;
    }

    // An offer already on screen owns the decision; a second button behind it
    // would just be a way to bypass the one the player is looking at.
    if (IsOfferOpen())
    {
        ForgetMenu();
        return;
    }

    if (!on)
    {
        ForgetMenu();
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
        ForgetMenu();
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
    // because this is the moment the player is actually looking at them: down,
    // it counts once per knockout (with the player's credit); standing, it
    // re-arms the next one.
    NoteIfDown(character);

    std::string excluded;
    if (!IsRecruitable(character, &excluded))
    {
        if (GetConfig().probeOnClick)
        {
            LogInfo("Context menu: " + CharacterName(character) +
                    " skipped (" + excluded + ").");
        }
        ForgetMenu();
        return;
    }

    HideButtons();
    g_targetObject = what;
    g_targetCharacter = character;
    // A freshly opened menu picks its side from scratch.
    g_previousSide = menuslot::SIDE_NONE;
    g_block = ComposeContextBlock(CageCommandsAvailable(character));

    if (!CreateBlockButtons())
    {
        ForgetMenu();
        return;
    }

    // A full evaluation only feeds this diagnostic line, so it only runs when
    // the line is wanted; the verdict itself is worked out when Hablar is pressed.
    if (config.probeOnClick)
    {
        const Verdict verdict = Evaluate(character);
        LogInfo("Context menu: " + CharacterName(character) + " -> " +
                (verdict.willJoin ? "CAN JOIN" : "refuses") +
                " (" + CurrencyName(verdict.currency) + ") " + verdict.reason +
                (g_block.size() > 1 ? " [cage commands]" : ""));
    }

    CaptionBlock(character);

    // A fresh menu: the next press or release over each button is a new
    // gesture. It never fires by itself, so the click that opened the menu
    // cannot trigger a button unless it is released over it.
    for (int kind = 0; kind < ContextButton_Count; ++kind)
    {
        g_gestures[kind] = clickgesture::State();
        if (InBlock(kind))
        {
            clickgesture::Arm(&g_gestures[kind]);
        }
    }

    // The native menu sizes itself during show, so the first anchor attempt can
    // read a stale rect; update() re-anchors every frame while it is open.
    ResetMenuAnchorLog();
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

    if (g_block.empty() || g_targetCharacter == NULL)
    {
        return;
    }

    if (!visible)
    {
        ForgetMenu();
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
    ForgetMenu();
}

void ShutdownRecruitOverlay()
{
    ForgetMenu();
    DestroyButtons();
}
} // namespace totalrecruit
