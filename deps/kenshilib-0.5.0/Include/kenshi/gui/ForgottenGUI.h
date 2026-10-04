#pragma once

#include <kenshi/util/lektor.h>
#include "CharacterEditWindow.h"
#include <mygui/MyGUI.h>
#include <kenshi/Enums.h>
#include <ogre/OgreVector2.h>
#include <boost/thread/shared_mutex.hpp>
#include "ScreenLabel.h"

class ScreenLabelInterface;
class DatapanelGUI;
class ToolTip;
class GUIWindow;
class Character;
class Town;
class InventoryGUI;
class MainBarGUI;
class DialogueWindow;
class TutorialGUI;
class TutorialpediaGUI;
class Inventory;
class InventoryLayout;
class CharacterStatsWindow;
class FloatingProgressBar;

enum TradeWindowType
{
    TW_OFF,
    TW_MONEY_TRADING,
    TW_LOOTING,
    TW_AUTO
};

namespace MyGUI
{
    // TODO this is likely non-standard
    class OgrePlatform;
}

class ForgottenGUI : public Ogre::GeneralAllocatedObject
{
public:
    // Ogre::AllocatedObject<Ogre::CategorisedAllocPolicy<0> > offset = 0x8, length = 0x1
    // VTable         : (none)
    // no_addr void ForgottenGUI(const class ForgottenGUI & _a1);// public missing arg names
    ForgottenGUI();// public RVA = 0x6E7D80
    ForgottenGUI* _CONSTRUCTOR();// public RVA = 0x6E7D80
    virtual ~ForgottenGUI();// public RVA = 0x6EA310 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x6EA310 vtable offset = 0x0
    void clearGUI();// public RVA = 0x6E8AF0
    void shutDown();// public RVA = 0x6EA160
    void restart();// public RVA = 0x6E8DE0
    static void changeFontSize();// public RVA = 0x3E7C80
    void setupGuiScaling();// public RVA = 0x6E31F0
    void init();// public RVA = 0x6E44A0
    float getTopOfScreenMessagePosition();// public RVA = 0x6E1800
    void showLoadingMessage(bool on);// public RVA = 0x6E9110
    bool isLoadingMessageVisible();// public RVA = 0x6E1660
    void showPauseMessage(bool on);// public RVA = 0x6E1690
    bool isLoading() const;// public RVA = 0x6E16C0
    void setLoading(bool value);// public RVA = 0x6E1710
    bool isPaused() const;// public RVA = 0x6E1770
    bool inDialogue() const;// public RVA = 0x6E17D0
    void destroy(ScreenLabelInterface* label);// public RVA = 0x6E9080
    void destroy(DatapanelGUI* datapanel);// public RVA = 0x6E2690
    void destroy(GUIWindow* win);// public RVA = 0x6E5510
    DatapanelGUI* createDatapanel(const std::string& name, MyGUI::Widget* win, bool scrolls);// public RVA = 0x73EE10
    DatapanelGUI* createDatapanel(float top, float left, float width, float height, bool scrolls, std::string layer, bool window);// public RVA = 0x73EEE0
    MyGUI::Window* messageBox(const std::string& title, const std::string& message, int btn, bool modal, MyGUI::delegates::IDelegate1<int>* callback);// public RVA = 0x740F60
    bool hideMessageBox(bool enter);// public RVA = 0x73EA10
    bool hasModalMessage() const;// public RVA = 0x73EA20
    void showMainbar(bool on);// public RVA = 0x6E1520
    void updateBountyPanelVisibility();// public RVA = 0x71FFC0
    // no_addr void showDialogue(bool _a1);// public missing arg names
    bool getInDialogue() const;// public RVA = 0x6E1590
    ToolTip* getToolTip();// public RVA = 0x6E15A0
    bool isVisible();// public RVA = 0xDEDD0
    void toggle();// public RVA = 0x6E1FE0
    void showNames(bool on);// public RVA = 0x6E2230
    void showPrintOutput(bool v);// public RVA = 0x6E1910
    void showCharacterEditor(const lektor<Character*> characters, CharacterEditMode mode, const Ogre::vector<GameDataReference>::type* races);// public RVA = 0x6E28B0
    void closeCharacterEditor();// public RVA = 0x6E1870
    bool isCharacterEditorMode();// public RVA = 0x47A180
    void showCharacterStatsWindow(const hand& owner);// public RVA = 0x6E50C0
    void closeCharacterStatsWindow(const hand& owner);// public RVA = 0x6E52B0
    void closeAllCharacterStatsWindows();// public RVA = 0x6E2E20
    bool isStatsWindowOpen() const;// public RVA = 0x6E23E0
    void toggleStatsWindow(const hand& owner);// public RVA = 0x6E5460
    void toggleCharacterStatsWindowPermanent(const hand& owner);// public RVA = 0x6E2400
    bool characterStatsWindowVisible();// public RVA = 0x91BEE0
    void selectedObjectsChanged();// public RVA = 0x6E8350
    void updateToOrdersPanel();// public RVA = 0x6E1610
    void notifyTownDeleted(Town* t);// public RVA = 0x6E1630
    void update();// public RVA = 0x6E91E0
    void showTradeWindow(const hand& a, const hand& b, TradeWindowType type);// public RVA = 0x7905D0
    void closeTradeWindow();// public RVA = 0x790630
    InventoryGUI* showInventory(const hand& owner, bool autoPosition, float x, float y);// public RVA = 0x6E5990
    InventoryGUI* showTraderInventory(const hand& owner);// public RVA = 0x6E5C60
    InventoryGUI* showInventoryBuilding(const hand& owner);// public RVA = 0x6E57B0
    InventoryGUI* showInventoryNPC(const hand& owner);// public RVA = 0x6E5F30
    void closeInventory(const hand& owner);// public RVA = 0x6E4920
    void closeInventoryBuilding();// public RVA = 0x6E5960
    void closeAllInventories();// public RVA = 0x6E4C90
    void closeAllWindows();// public RVA = 0x6E5660
    InventoryGUI* getInventoryWindow(const hand& who);// public RVA = 0x6E43D0
    InventoryGUI* toggleInventory(const hand& owner, InventoryGUI* parentInv, bool attached);// public RVA = 0x6E6080
    int getNumOpenInventoryWindows() const;// public RVA = 0x6E23B0
    InventoryGUI* getInventoryWindowWithMouse();// public RVA = 0x6E2C90
    bool hasInventoryWindowOpen(const hand& obj);// public RVA = 0x6E4310
    bool isAnyInventoryWindowOpen();// public RVA = 0x6E23C0
    void toggleInventoryWindowPermanent(const hand& owner);// public RVA = 0x6E56F0
    void toggleHelpWindow();// public RVA = 0x6E14E0
    const hand& getSelectedObject() const;// public RVA = 0x569BE0
    const hand& getSelectedPlayerCharacter() const;// public RVA = 0x32D8F0
    void addWindowToUpdateList(GUIWindow* win);// public RVA = 0x6E2730
    void removeWindowFromUpdateList(GUIWindow* win);// public RVA = 0x6E2780
    void addScreenLabel(ScreenLabelInterface* label);// public RVA = 0x6E8FF0
    void addDatapanelToUpdateList(DatapanelGUI* datapanel);// public RVA = 0x6E27F0
    void removeDatapanelFromUpdateList(DatapanelGUI* datapanel);// public RVA = 0x6E2840
    void destroyWidget(MyGUI::Widget* widget);// public RVA = 0x6E15B0
    void destroyWidgets(MyGUI::Enumerator<std::vector<MyGUI::Widget*> >& widgets);// public RVA = 0x6E15F0
    void destroyWidgets(const std::vector<MyGUI::Widget*>& widgets);// public RVA = 0x6E15D0
    bool widgetHasMouse(MyGUI::Widget* w);// public RVA = 0x6E2340
    bool hasMouse; // 0x8 Member
    MainBarGUI* mainbar; // 0x10 Member
    ToolTip* tooltip; // 0x18 Member
    DialogueWindow* dialogue; // 0x20 Member
    TutorialGUI* tutorial; // 0x28 Member
    TutorialpediaGUI* tutorialpedia; // 0x30 Member
    CursorType currentCursor; // 0x38 Member
    void setup(Ogre::Root* mRoot, Ogre::RenderWindow* window);// public RVA = 0x6EA510
    void changeMouseCursor(CursorType cursor, const hand& player, const hand& target);// public RVA = 0x6E3D70
    void changeMouseCursor(CursorType cursor);// public RVA = 0x6E3320
    void setMouseCursorVisible(bool visible);// public RVA = 0x6E14B0
    MyGUI::Gui* getGuiManager();// public RVA = 0x6EE0D0
    // no_addr class MyGUI::OgrePlatform * getGuiPlatform();// public
    Ogre::Vector2 Scale; // 0x3C Member
    MyGUI::Gui* manager; // 0x48 Member
    MyGUI::OgrePlatform* guiPlatform; // 0x50 Member
    void _showTradeWindow(RootObject* a, RootObject* b, TradeWindowType tradeType);// private RVA = 0x790640
    void setInventoryPosition(const hand& owner, InventoryGUI* inventory);// private RVA = 0x6E3EB0
    InventoryGUI* createInventoryWindow(const hand& owner, Inventory* inventory, InventoryLayout* layout, RootObject* trader);// private RVA = 0x6E47B0
    InventoryGUI* createInventoryWindow(const hand& owner, InventoryLayout* layout);// private RVA = 0x6E55F0
    void inventoriesSelectedObjectUpdate(const hand& selectedObject);// private RVA = 0x6E8160
    TradeWindowType _showTradeWindowMsg; // 0x58 Member
    bool _closeTradeWindowMsg; // 0x5C Member
    hand tradeA; // 0x60 Member
    hand tradeB; // 0x80 Member
    ogre_unordered_map<hand, InventoryGUI*>::type inventoryWindowsOpen; // 0xA0 Member
    hand inventoryWindowBuilding; // 0xE0 Member
    hand inventoryWindowCharacter; // 0x100 Member
    hand inventoryWindowTrader; // 0x120 Member
    hand inventoryWindowNPC; // 0x140 Member
    ogre_unordered_set<hand>::type inventoryWindowsPermanent; // 0x160 Member
    Ogre::vector<InventoryGUI*>::type inventoryWindowsKillList; // 0x1A0 Member
    CharacterEditWindow* characterEditor; // 0x1C0 Member
    Ogre::vector<CharacterStatsWindow*>::type characterStatsWindows; // 0x1C8 Member
    Ogre::vector<CharacterStatsWindow*>::type characterStatsWindowsKillList; // 0x1E8 Member
    lektor<GUIWindow*> guiWindows; // 0x208 Member
    lektor<DatapanelGUI*> guiDatapanels; // 0x220 Member
    boost::shared_mutex guiScreenLabelsMutex; // 0x238 Member
    lektor<ScreenLabelInterface*> guiScreenLabels; // 0x258 Member
    lektor<ScreenLabelInterface*> guiScreenLabelsToAdd; // 0x270 Member
    lektor<ScreenLabelInterface*> guiScreenLabelsToRemove; // 0x288 Member
    hand selectedObject; // 0x2A0 Member
    hand selectedPlayerCharacter; // 0x2C0 Member
    DatapanelGUI* cursorPanel; // 0x2E0 Member
    bool cursorAction; // 0x2E8 Member
    hand cursorActionPlayer; // 0x2F0 Member
    hand cursorActionTarget; // 0x310 Member
    bool visible; // 0x330 Member
    bool created; // 0x331 Member
    void keepWindownOnScreen(MyGUI::Widget* win);// public RVA = 0x6E13D0
    const std::string& getDataLineColor(int actualVal, int normalVal) const;// public RVA = 0x73EA30
    // no_addr const class std::basic_string<char,std::char_traits<char>,std::allocator<char> > & getDataLineColor(float _a1, float _a2);// public missing arg names
    MyGUI::Window* createPanel(const std::string& name, float top, float left, float width, float height, std::string layer, std::string skin);// public RVA = 0x73F050
    MyGUI::Window* createPanelAbs(const std::string& name, float top, float left, float width, float height, std::string layer, std::string skin);// public RVA = 0x73F140
    MyGUI::ImageBox* createImage(MyGUI::Widget* parent, float top, float left, float width, float height, const std::string& _image);// public RVA = 0x73F3C0
    MyGUI::ImageBox* createImageAbs(MyGUI::Widget* parent, int top, int left, int width, int height, const std::string& _image);// public RVA = 0x73F260
    MyGUI::ImageBox* createRotatableImageAbs(MyGUI::Widget* parent, int top, int left, int width, int height, const std::string& _image);// public RVA = 0x73F560
    MyGUI::TabControl* createTabPanel(const std::string& name, float top, float left, float width, float height, const std::string& layer, const std::string& skin);// public RVA = 0x73EA60
    MyGUI::Button* createButtonAbs(MyGUI::Widget* parent, int top, int left, int width, int height, const std::string& name, const std::string& text, const std::string& skin);// public RVA = 0x73EAE0
    MyGUI::Button* createButton(MyGUI::Widget* parent, float top, float left, float width, float height, const std::string& name, const std::string& text, const std::string& skin);// public RVA = 0x73EBD0
    MyGUI::ComboBox* createDropBox(MyGUI::Widget* parent, const std::string& name, float top, float left, float width, float height);// public RVA = 0x73F6D0
    MyGUI::ComboBox* createDropBoxAbs(MyGUI::Widget* parent, const std::string& name, int top, int left, int width, int height);// public RVA = 0x73F820
    MyGUI::ProgressBar* createProgressBar(MyGUI::Widget* parent, float top, float left, float width, float height, const std::string& type);// public RVA = 0x740A00
    MyGUI::ProgressBar* createProgressBarAbs(MyGUI::Widget* parent, float top, float left, float width, float height, const std::string& type);// public RVA = 0x740AE0
    MyGUI::Window* createFloatingLabel(float top, float left, float width, float height, const std::string& text, MyGUI::Align a, std::string layer);// public RVA = 0x740240
    MyGUI::EditBox* createLabel(MyGUI::Widget* parent, float top, float left, float width, float height, const std::string& text, const std::string& skin);// public RVA = 0x7405A0
    MyGUI::TextBox* createLabel(MyGUI::Widget* parent, float top, float left, float width, float height, const std::string& text, MyGUI::Align a);// public RVA = 0x7403E0
    MyGUI::TextBox* createLabelAbs(MyGUI::Widget* parent, int top, int left, int width, int height, const std::string& labelText, MyGUI::Align align);// public RVA = 0x740880
    MyGUI::EditBox* createLabelAbs(MyGUI::Widget* parent, int top, int left, int width, int height, const std::string& text, const std::string& skin);// public RVA = 0x740720
    MyGUI::EditBox* createValueEdit(MyGUI::Widget* parent, float top, float left, float width, float height, const std::string& name);// public RVA = 0x73F940
    MyGUI::EditBox* createEditBox(MyGUI::Widget* parent, float top, float left, float width, float height, const std::string& name, const std::string& skin, bool multiLine);// public RVA = 0x73ECF0
    MyGUI::EditBox* createEditBox(MyGUI::Widget* parent, float top, float left, float width, float height, const std::string& name, bool multiLine);// public RVA = 0x73FA80
    MyGUI::EditBox* createEditBoxAbs(MyGUI::Widget* parent, int top, int left, int width, int height, const std::string& name, const std::string& skin, bool multiLine);// public RVA = 0x73ED90
    MyGUI::EditBox* createEditBoxAbs(MyGUI::Widget* parent, int top, int left, int width, int height, const std::string& name, bool multiLine);// public RVA = 0x73FBB0
    MyGUI::ScrollBar* createScrollBarAbs(MyGUI::Widget* parent, int top, int left, int width, int height, int range, int value);// public RVA = 0x73FCD0
    MyGUI::Button* createCheckbox(MyGUI::Widget* parent, int left, int top, int width, int height, const std::string& caption);// public RVA = 0x73FE20
    MyGUI::EditBox* createValueEditTransparent(MyGUI::Widget* parent, float top, float left, float width, float height, const std::string& name);// public RVA = 0x73FF90
    MyGUI::ListBox* createListbox(MyGUI::Window* parent, float top, float left, float width, float height);// public RVA = 0x7400B0
    MyGUI::Window* createFloatingImage(const std::string& image, float top, float left, float width, float height, std::string layer);// public RVA = 0x740BC0
    MyGUI::Window* createFloatingImageAbs(const std::string& image, float top, float left, float width, float height, std::string layer);// public RVA = 0x740D90
    FloatingProgressBar* createFloatingProgressBar();// public RVA = 0x73E8E0
    ScreenLabel* createScreenLabel(const std::string& text, const MyGUI::Colour& textColour, ScreenLabel::LabelSize textSize, ScreenLabel::RisingSpeed risingSpeed);// public RVA = 0x73E920
    ScreenLabelDebug* createScreenLabelD(const std::string& text, float time);// public RVA = 0x73E9A0
    // no_addr class ForgottenGUI & operator=(const class ForgottenGUI & _a1);// public missing arg names
    // no_addr void __local_vftable_ctor_closure();// public
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};