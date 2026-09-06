#include "common.h"
#include "spawn_panel.h"
#include "spawn_core.h"
#include "game_data.h"
#include "config.h"
#include "log.h"

namespace totalrecruit
{
namespace
{
// Two tabs. Squads: pick a template and let the engine build the whole squad,
// so the roster, leader and unit counts come out exactly as designed.
// Characters: a flat searchable list of every character and animal definition.
//
// Every MyGUI call is wrapped in a plain function and only that call is
// guarded, because C2712 counts every C++ object in the guarded function --
// including the temporaries MyGUI's own overloads create.

const int kPanelWidth = 640;
const int kPanelHeight = 500;
const int kRow = 26;
const int kMargin = 8;
const int kListHeight = 260;

enum Tab
{
    Tab_Squads,
    Tab_Characters
};

MyGUI::Window* g_window = NULL;
MyGUI::EditBox* g_search = NULL;
MyGUI::ListBox* g_leftList = NULL;
MyGUI::ListBox* g_rightList = NULL;
MyGUI::TextBox* g_rightHeader = NULL;
MyGUI::TextBox* g_status = NULL;
MyGUI::Button* g_tabSquads = NULL;
MyGUI::Button* g_tabCharacters = NULL;
MyGUI::Button* g_spawnPrimary = NULL;
MyGUI::Button* g_spawnSecondary = NULL;
MyGUI::Button* g_autoRecruitButton = NULL;

Tab g_tab = Tab_Squads;

std::vector<DataEntry> g_visibleSquads;
std::vector<DataEntry> g_visibleMembers;
std::vector<DataEntry> g_visibleCharacters;

// ----------------------------------------------------------- safe wrappers --

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

void ClearListItems(MyGUI::ListBox* list)
{
    list->removeAllItems();
}

void SafeClearList(MyGUI::ListBox* list)
{
    if (list == NULL) return;
    TR_TRY { ClearListItems(list); }
    TR_CATCH { }
}

void AddListItem(MyGUI::ListBox* list, const char* text)
{
    list->addItem(text);
}

void SafeAddItem(MyGUI::ListBox* list, const char* text)
{
    if (list == NULL) return;
    TR_TRY { AddListItem(list, text); }
    TR_CATCH { }
}

void CopyOnlyText(MyGUI::EditBox* box, char* buffer, size_t size)
{
    strncpy_s(buffer, size, box->getOnlyText().asUTF8_c_str(), _TRUNCATE);
}

void SafeGetText(MyGUI::EditBox* box, char* buffer, size_t size)
{
    buffer[0] = 0;
    if (box == NULL) return;
    TR_TRY { CopyOnlyText(box, buffer, size); }
    TR_CATCH { buffer[0] = 0; }
}

size_t SafeGetSelected(MyGUI::ListBox* list)
{
    if (list == NULL) return MyGUI::ITEM_NONE;
    TR_TRY { return list->getIndexSelected(); }
    TR_CATCH { return MyGUI::ITEM_NONE; }
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

bool SafeGetVisible(MyGUI::Widget* widget)
{
    if (widget == NULL) return false;
    TR_TRY { return widget->getVisible(); }
    TR_CATCH { return false; }
}

void SafeSetVisible(MyGUI::Widget* widget, bool visible)
{
    if (widget == NULL) return;
    TR_TRY { widget->setVisible(visible); }
    TR_CATCH { }
}

// ----------------------------------------------------------------- helpers --

void SetStatus(const std::string& text)
{
    SafeSetCaption(g_status, text.c_str());
}

void UpdateAutoRecruitCaption()
{
    const std::string caption =
        std::string(GetConfig().autoRecruit ? "[X] " : "[  ] ") + "Auto recruit";
    SafeSetCaption(g_autoRecruitButton, caption.c_str());
}

std::string CurrentKeyword()
{
    char buffer[128] = { 0 };
    SafeGetText(g_search, buffer, sizeof(buffer));
    return std::string(buffer);
}

// With thousands of templates from many mods, identical display names are
// common. Duplicates are suffixed with their string id so the right one can be
// picked; unique names stay clean.
bool IsDuplicateName(const std::vector<DataEntry>& entries, size_t index)
{
    if (index > 0 && entries[index - 1].name == entries[index].name)
    {
        return true;
    }
    if (index + 1 < entries.size() && entries[index + 1].name == entries[index].name)
    {
        return true;
    }
    return false;
}

void FillList(MyGUI::ListBox* list, const std::vector<DataEntry>& entries,
              bool withRole)
{
    SafeClearList(list);
    for (size_t i = 0; i < entries.size(); ++i)
    {
        std::string label = entries[i].name;

        if (withRole && !entries[i].role.empty())
        {
            label += "  (" + entries[i].role + ")";
        }
        if (IsDuplicateName(entries, i) && !entries[i].stringID.empty())
        {
            label += "  [" + entries[i].stringID + "]";
        }

        SafeAddItem(list, label.c_str());
    }
}

void RefreshCurrentTab()
{
    const std::string keyword = CurrentKeyword();

    if (g_tab == Tab_Squads)
    {
        g_visibleSquads = Filter(SquadTemplates(false), keyword);
        g_visibleMembers.clear();
        FillList(g_leftList, g_visibleSquads, false);
        FillList(g_rightList, g_visibleMembers, true);

        char message[96];
        sprintf_s(message, sizeof(message), "%u squads",
                  (unsigned int)g_visibleSquads.size());
        SetStatus(message);
        return;
    }

    g_visibleCharacters = Filter(Characters(false), keyword);
    FillList(g_leftList, g_visibleCharacters, false);
    SafeClearList(g_rightList);

    char message[96];
    sprintf_s(message, sizeof(message), "%u characters",
              (unsigned int)g_visibleCharacters.size());
    SetStatus(message);
}

void ApplyTab(Tab tab)
{
    g_tab = tab;

    SafeSetCaption(g_tabSquads, tab == Tab_Squads ? "> Squads" : "  Squads");
    SafeSetCaption(g_tabCharacters,
                   tab == Tab_Characters ? "> Characters" : "  Characters");

    if (tab == Tab_Squads)
    {
        SafeSetCaption(g_rightHeader, "Members");
        SafeSetCaption(g_spawnPrimary, "Spawn squad");
        SafeSetCaption(g_spawnSecondary, "Spawn squad (force roaming)");
    }
    else
    {
        SafeSetCaption(g_rightHeader, "");
        SafeSetCaption(g_spawnPrimary, "Spawn selected");
        SafeSetCaption(g_spawnSecondary, "Spawn x5");
    }

    RefreshCurrentTab();
}

// --------------------------------------------------------------- callbacks --

void OnSearchChanged(MyGUI::EditBox* /*sender*/)
{
    RefreshCurrentTab();
}

void OnLeftSelected(MyGUI::ListBox* /*sender*/, size_t index)
{
    // Only the squads tab has a dependent right-hand list.
    if (g_tab != Tab_Squads) return;
    if (index == MyGUI::ITEM_NONE || index >= g_visibleSquads.size()) return;

    const DataEntry& squad = g_visibleSquads[index];
    g_visibleMembers = SquadMembers(squad);
    FillList(g_rightList, g_visibleMembers, true);

    char message[192];
    sprintf_s(message, sizeof(message), "%s: %u members listed",
              squad.name.c_str(), (unsigned int)g_visibleMembers.size());
    SetStatus(message);
}

void OnTabSquads(MyGUI::Widget*, int, int, MyGUI::MouseButton)
{
    ApplyTab(Tab_Squads);
}

void OnTabCharacters(MyGUI::Widget*, int, int, MyGUI::MouseButton)
{
    ApplyTab(Tab_Characters);
}

void OnSpawnPrimary(MyGUI::Widget*, int, int, MyGUI::MouseButton)
{
    const size_t index = SafeGetSelected(g_leftList);
    std::string message;

    if (g_tab == Tab_Squads)
    {
        if (index == MyGUI::ITEM_NONE || index >= g_visibleSquads.size())
        {
            SetStatus("Select a squad first");
            return;
        }
        // The checkbox governs squads as well, so one setting covers everything.
        const SquadSpawnMode mode = GetConfig().autoRecruit ? SquadSpawn_Player
                                                            : SquadSpawn_Roaming;
        SpawnSquad(g_visibleSquads[index], mode, &message);
    }
    else
    {
        if (index == MyGUI::ITEM_NONE || index >= g_visibleCharacters.size())
        {
            SetStatus("Select a character first");
            return;
        }
        SpawnTemplate(g_visibleCharacters[index], 1, &message);
    }

    SetStatus(message);
}

void OnSpawnSecondary(MyGUI::Widget*, int, int, MyGUI::MouseButton)
{
    const size_t index = SafeGetSelected(g_leftList);
    std::string message;

    if (g_tab == Tab_Squads)
    {
        if (index == MyGUI::ITEM_NONE || index >= g_visibleSquads.size())
        {
            SetStatus("Select a squad first");
            return;
        }
        SpawnSquad(g_visibleSquads[index], SquadSpawn_Roaming, &message);
    }
    else
    {
        if (index == MyGUI::ITEM_NONE || index >= g_visibleCharacters.size())
        {
            SetStatus("Select a character first");
            return;
        }
        SpawnTemplate(g_visibleCharacters[index], 5, &message);
    }

    SetStatus(message);
}

// Spawns one member of the selected squad, taken from the right-hand list.
void OnSpawnMember(MyGUI::Widget*, int, int, MyGUI::MouseButton)
{
    if (g_tab != Tab_Squads)
    {
        SetStatus("Switch to the Squads tab first");
        return;
    }

    const size_t index = SafeGetSelected(g_rightList);
    if (index == MyGUI::ITEM_NONE || index >= g_visibleMembers.size())
    {
        SetStatus("Select a member first");
        return;
    }

    std::string message;
    SpawnTemplate(g_visibleMembers[index], 1, &message);
    SetStatus(message);
}

void OnToggleAutoRecruit(MyGUI::Widget*, int, int, MyGUI::MouseButton)
{
    GetConfig().autoRecruit = !GetConfig().autoRecruit;
    UpdateAutoRecruitCaption();
}

void OnReload(MyGUI::Widget*, int, int, MyGUI::MouseButton)
{
    ClearDataCache();
    RefreshCurrentTab();
}

void OnWindowButton(MyGUI::Window* sender, const std::string& button)
{
    if (button == "close")
    {
        SafeSetVisible(sender, false);
    }
}

// ------------------------------------------------------------ construction --

MyGUI::Button* CreateChildButton(MyGUI::Widget* parent, const char* skin,
                                 int left, int top, int width, const char* name)
{
    return parent->createWidget<MyGUI::Button>(
        skin, MyGUI::IntCoord(left, top, width, kRow),
        MyGUI::Align::Default, name);
}

MyGUI::Button* SafeCreateChildButton(MyGUI::Widget* parent, const char* skin,
                                     int left, int top, int width, const char* name)
{
    TR_TRY { return CreateChildButton(parent, skin, left, top, width, name); }
    TR_CATCH { return NULL; }
}

MyGUI::Button* MakeButton(MyGUI::Widget* parent, int left, int top, int width,
                          const char* caption, const char* name)
{
    MyGUI::Button* button = NULL;
    // Skin availability differs between installs, so fall back to the stock one.
    const char* skins[2] = { "Kenshi_Button1", "Button" };
    for (int i = 0; i < 2 && button == NULL; ++i)
    {
        button = SafeCreateChildButton(parent, skins[i], left, top, width, name);
    }

    if (button != NULL)
    {
        SafeSetCaption(button, caption);
    }
    return button;
}

void PopulatePanel()
{
    g_window->setCaption("Unlimited Recruit");
    g_window->eventWindowButtonPressed += MyGUI::newDelegate(&OnWindowButton);

    MyGUI::Widget* client = g_window->getClientWidget();
    if (client == NULL)
    {
        client = g_window;
    }

    const int half = (kPanelWidth - kMargin * 3) / 2;
    int y = kMargin;

    g_tabSquads = MakeButton(client, kMargin, y, 130, "> Squads", "TR_TabSquads");
    if (g_tabSquads != NULL)
    {
        g_tabSquads->eventMouseButtonPressed += MyGUI::newDelegate(&OnTabSquads);
    }

    g_tabCharacters =
        MakeButton(client, kMargin + 138, y, 130, "  Characters", "TR_TabChars");
    if (g_tabCharacters != NULL)
    {
        g_tabCharacters->eventMouseButtonPressed +=
            MyGUI::newDelegate(&OnTabCharacters);
    }

    y += kRow + kMargin;

    g_search = client->createWidget<MyGUI::EditBox>(
        "Kenshi_EditBox", MyGUI::IntCoord(kMargin, y, half, kRow),
        MyGUI::Align::Default, "TR_Search");
    g_search->setEditMultiLine(false);
    g_search->eventEditTextChange += MyGUI::newDelegate(&OnSearchChanged);

    g_rightHeader = client->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxPaintedText",
        MyGUI::IntCoord(kMargin * 2 + half, y, half, kRow),
        MyGUI::Align::Default, "TR_RightHeader");
    g_rightHeader->setCaption("Members");

    y += kRow + kMargin;

    g_leftList = client->createWidget<MyGUI::ListBox>(
        "Kenshi_ListBox", MyGUI::IntCoord(kMargin, y, half, kListHeight),
        MyGUI::Align::Default, "TR_LeftList");
    g_leftList->eventListChangePosition += MyGUI::newDelegate(&OnLeftSelected);

    g_rightList = client->createWidget<MyGUI::ListBox>(
        "Kenshi_ListBox",
        MyGUI::IntCoord(kMargin * 2 + half, y, half, kListHeight),
        MyGUI::Align::Default, "TR_RightList");

    y += kListHeight + kMargin;

    g_status = client->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxPaintedText",
        MyGUI::IntCoord(kMargin, y, kPanelWidth - kMargin * 2, kRow),
        MyGUI::Align::Default, "TR_Status");

    y += kRow + kMargin;

    g_spawnPrimary = MakeButton(client, kMargin, y, 170,
                                "Spawn squad (mine)", "TR_SpawnPrimary");
    if (g_spawnPrimary != NULL)
    {
        g_spawnPrimary->eventMouseButtonPressed +=
            MyGUI::newDelegate(&OnSpawnPrimary);
    }

    g_spawnSecondary = MakeButton(client, kMargin + 178, y, 190,
                                  "Spawn squad (roaming)", "TR_SpawnSecondary");
    if (g_spawnSecondary != NULL)
    {
        g_spawnSecondary->eventMouseButtonPressed +=
            MyGUI::newDelegate(&OnSpawnSecondary);
    }

    MyGUI::Button* reload =
        MakeButton(client, kMargin + 376, y, 100, "Reload", "TR_Reload");
    if (reload != NULL)
    {
        reload->eventMouseButtonPressed += MyGUI::newDelegate(&OnReload);
    }

    y += kRow + kMargin;

    MyGUI::Button* spawnMember =
        MakeButton(client, kMargin, y, 170, "Spawn selected member", "TR_SpawnMember");
    if (spawnMember != NULL)
    {
        spawnMember->eventMouseButtonPressed += MyGUI::newDelegate(&OnSpawnMember);
    }

    g_autoRecruitButton =
        MakeButton(client, kMargin + 178, y, 190, "", "TR_AutoRecruit");
    if (g_autoRecruitButton != NULL)
    {
        g_autoRecruitButton->eventMouseButtonPressed +=
            MyGUI::newDelegate(&OnToggleAutoRecruit);
    }
    UpdateAutoRecruitCaption();

    g_window->setVisible(false);
}

bool SafePopulatePanel()
{
    TR_TRY
    {
        PopulatePanel();
        return true;
    }
    TR_CATCH { return false; }
}

MyGUI::Window* CreatePanelWindow(MyGUI::Gui* guiManager)
{
    return guiManager->createWidget<MyGUI::Window>(
        "Kenshi_WindowCX",
        MyGUI::IntCoord(120, 120, kPanelWidth, kPanelHeight),
        MyGUI::Align::Default, "Window", "TR_SpawnPanel");
}

MyGUI::Window* SafeCreatePanelWindow(MyGUI::Gui* guiManager)
{
    TR_TRY { return CreatePanelWindow(guiManager); }
    TR_CATCH { return NULL; }
}

bool BuildPanel()
{
    if (g_window != NULL)
    {
        return true;
    }

    MyGUI::Gui* guiManager = MyGUI::Gui::getInstancePtr();
    if (guiManager == NULL)
    {
        LogError("MyGUI is not available yet.");
        return false;
    }

    g_window = SafeCreatePanelWindow(guiManager);
    if (g_window == NULL)
    {
        LogError("Could not create the spawn panel window.");
        return false;
    }

    if (!SafePopulatePanel())
    {
        LogError("Could not build the spawn panel contents.");
        return false;
    }

    return true;
}
} // namespace

void ToggleSpawnPanel()
{
    if (!BuildPanel())
    {
        return;
    }

    const bool visible = SafeGetVisible(g_window);

    if (!visible)
    {
        // Indexing runs on first open so startup stays fast.
        ApplyTab(g_tab);
    }

    SafeSetVisible(g_window, !visible);
}

void ShutdownSpawnPanel()
{
    SafeDestroyWidget(g_window);

    g_window = NULL;
    g_search = NULL;
    g_leftList = NULL;
    g_rightList = NULL;
    g_rightHeader = NULL;
    g_status = NULL;
    g_tabSquads = NULL;
    g_tabCharacters = NULL;
    g_spawnPrimary = NULL;
    g_spawnSecondary = NULL;
    g_autoRecruitButton = NULL;
    g_visibleSquads.clear();
    g_visibleMembers.clear();
    g_visibleCharacters.clear();
}
} // namespace totalrecruit
