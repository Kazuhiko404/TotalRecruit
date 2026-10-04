#pragma once

#include "GUIWindow.h"
#include <mygui/common/baselayout/BaseLayout.h>
#include <kenshi/util/StringPair.h>
#include <kenshi/util/iVector2.h>
#include <kenshi/GameData.h>

class LockedArmour;
class Town;
class InventorySection;
class ContainerItem;
class Inventory;
class InventoryLayout;
class ToolTipInventory;

class InventoryIcon : private wraps::BaseLayout
{
public:
    // wraps::BaseLayout offset = 0x0, length = 0xA0
    // no_addr void InventoryIcon(const class InventoryIcon & _a1);// public missing arg names
    InventoryIcon(Item* item, const MyGUI::types::TPoint<int>& position, MyGUI::Widget* parent);// public RVA = 0x70FF90
    InventoryIcon* _CONSTRUCTOR(Item* item, const MyGUI::types::TPoint<int>& position, MyGUI::Widget* parent);// public RVA = 0x70FF90
    void setPosition(const MyGUI::types::TPoint<int>& position);// public RVA = 0x70BA20
    MyGUI::types::TSize<int> getSize() const;// public RVA = 0x70BA40
    void update();// public RVA = 0x70F0F0
    MyGUI::Widget* getWidget() const;// public RVA = 0x70BA90
    static void createIconImage(Item* item, std::string& name, iVector2& size);// public RVA = 0x70BA70
    static MyGUI::types::TSize<int> getItemSize(GameData* itemData);// public RVA = 0x70EFE0
    static MyGUI::types::TPoint<int> getItemPosition(int x, int y);// public RVA = 0x70C3A0
    Item* item; // 0xA0 Member
    MyGUI::ImageBox* image; // 0xA8 Member
    MyGUI::TextBox* quantityText; // 0xB0 Member
    MyGUI::Widget* chargesProgress; // 0xB8 Member
    virtual ~InventoryIcon();// public RVA = 0x71F9D0 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x71F9D0 vtable offset = 0x0
    // no_addr class InventoryIcon & operator=(const class InventoryIcon & _a1);// public missing arg names
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};

class InventorySectionGUI
{
public:
    // no_addr void InventorySectionGUI(const class InventorySectionGUI & _a1);// public missing arg names
    InventorySectionGUI(MyGUI::Widget* widget, const std::string& sectionName);// public RVA = 0x70D310
    InventorySectionGUI* _CONSTRUCTOR(MyGUI::Widget* widget, const std::string& sectionName);// public RVA = 0x70D310
    ~InventorySectionGUI();// public RVA = 0x70D1B0
    void _DESTRUCTOR();// public RVA = 0x70D1B0
    bool hasMouse();// public RVA = 0x70C300
    MyGUI::types::TPoint<int> getItemAbsolutePosition(int x, int y);// public RVA = 0x70C370
    MyGUI::Widget* getWidget();// public RVA = 0x70BA00
    MyGUI::types::TPoint<int> getPositionSlot(const MyGUI::types::TPoint<int>& position, InventorySection* section, bool round);// public RVA = 0x70C4C0
    bool getBestPositionSlot(const MyGUI::types::TPoint<int>& position, InventorySection* section, Item* item, MyGUI::types::TPoint<int>& slot);// public RVA = 0x70EDF0
    void setEnabled(bool value);// public RVA = 0x70BA10
    void refreshIcons(InventorySection* section);// public RVA = 0x7106B0
    void update();// public RVA = 0x70FD00
    MyGUI::Widget* widget; // 0x0 Member
    Ogre::vector<InventoryIcon*>::type itemsIcons; // 0x8 Member
    // no_addr class InventorySectionGUI & operator=(const class InventorySectionGUI & _a1);// public missing arg names
    // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
};

class InventoryGUI : public GUIWindow
{
public:
    // GUIWindow offset = 0x0, length = 0x30
    class TradeResult
    {
    public:
        enum Enum
        {
            OK,
            OUT_OF_RANGE,
            NO_ROOM,
            CANT_AFFORD,
            CANT_AFFORD_SHOPKEPPER,
            CANT_WEAR_ITEM,
            INCOMPATIBLE_ITEM,
            LOCKED,
            THIEF_DETECTED,
            SELLING_STOLEN_ITEM_DETECTED,
            ERROR_ITEM_POSITION,
            ERROR_INVALID,
            ERROR_THATS_MINE,
            ERROR_TARGET_CONSCIOUS,
            SMUGGLING_ONLY,
            ILLEGAL_GOODS,
            UNIFORMS,
            CONTAINER_NOT_EMPTY
        };

        TradeResult(Enum value);// public RVA = 0x70C280
        TradeResult* _CONSTRUCTOR(Enum value);// public RVA = 0x70C280
        bool operator==(const TradeResult& other) const;// public RVA = 0x70C290
        bool operator!=(const TradeResult& other) const;// public RVA = 0x70C2A0
        void showMessage();// public RVA = 0x70E570
        static void ShowMessage(TradeResult::Enum value);// public RVA = 0x70EC80
        Enum value; // 0x0 Member
    };
    class InventoryTradeData
    {
    public:
        InventoryTradeData(const InventoryTradeData& __that);// public RVA = 0x6F0840
        InventoryTradeData* _CONSTRUCTOR(const InventoryTradeData& __that);// public RVA = 0x6F0840
        InventoryTradeData(InventoryGUI* _inventory, bool _paymentRequired, bool _canDropItems, bool _player, const hand& h);// public RVA = 0x70C470
        InventoryTradeData* _CONSTRUCTOR(InventoryGUI* _inventory, bool _paymentRequired, bool _canDropItems, bool _player, const hand& h);// public RVA = 0x70C470
        InventoryTradeData();// public RVA = 0x70C250
        InventoryTradeData* _CONSTRUCTOR();// public RVA = 0x70C250
        bool isValid() const;// public RVA = 0x70E540
        InventoryGUI* inventory; // 0x0 Member
        bool paymentRequired; // 0x8 Member
        bool canDropItems; // 0x9 Member
        bool isPlayer; // 0xA Member
        hand owner; // 0x10 Member
        InventoryGUI::InventoryTradeData& operator=(const InventoryGUI::InventoryTradeData& __that);// public RVA = 0x718500
    };
    // Typedef        : InventoriesTradeDataMap
    static bool lockedItemCheck(LockedArmour* item);// public RVA = 0x70BE00
    static void setTradingTown(const hand& town);// public RVA = 0x70BEC0
    static Town* getTradingTown();// public RVA = 0x70BF00
    static void addTradePartner(InventoryGUI* _tradewith, bool payment, bool canDrop, bool isPlayer, const hand& who);// public RVA = 0x70E030
    static void removeTradePartner(InventoryGUI* _tradewith);// public RVA = 0x70D2D0
    static void clearTradePartners();// public RVA = 0x70C9B0
    static bool canDropMouseItemWithoutPaying(InventoryGUI* cameFrom);// public RVA = 0x70E0D0
    virtual void autoChangeSelectedObject(const hand& obj) override;// public RVA = 0x70C240 vtable offset = 0x0
    void _NV_autoChangeSelectedObject(const hand& obj);// public RVA = 0x70C240 vtable offset = 0x0
    virtual void update() override;// public RVA = 0x712640 vtable offset = 0x0
    void _NV_update();// public RVA = 0x712640 vtable offset = 0x0
    virtual void show(bool on) override;// public RVA = 0x70C3D0 vtable offset = 0x0
    void _NV_show(bool on);// public RVA = 0x70C3D0 vtable offset = 0x0
    virtual void setPositionReal(float x, float y) override;// public RVA = 0x70BB30 vtable offset = 0x0
    void _NV_setPositionReal(float x, float y);// public RVA = 0x70BB30 vtable offset = 0x0
    virtual bool isVisible() const override;// public RVA = 0x70BB40 vtable offset = 0x0
    bool _NV_isVisible() const;// public RVA = 0x70BB40 vtable offset = 0x0
    virtual void refreshSection(InventorySection* section);// protected RVA = 0x710E20 vtable offset = 0x68
    void _NV_refreshSection(InventorySection* section);// protected RVA = 0x710E20 vtable offset = 0x68
    void refreshSection();// public RVA = 0x70BBB0
    void autoArrangeButton(MyGUI::Widget* _sender);// public RVA = 0x70E4D0
    void openBackpackButton(MyGUI::Widget* _sender);// public RVA = 0x70CEB0
    void openLimbsInterface(MyGUI::Widget* _sender);// public RVA = 0x70BF70
    void showBackpack(bool on);// public RVA = 0x70CFD0
    ContainerItem* getBackpack();// public RVA = 0x70C840
    void windowButtonPressed(MyGUI::Window* _widget, const std::string& _name);// public RVA = 0x70CD70
    virtual Inventory* getInventory();// public RVA = 0x70E110 vtable offset = 0x70
    Inventory* _NV_getInventory();// public RVA = 0x70E110 vtable offset = 0x70
    // no_addr bool hasMouseFocus();// public
    bool hasSameOwner(const InventoryGUI* other) const;// public RVA = 0x70BAF0
    virtual Character* getCallbackCharacter();// public RVA = 0x70BD70 vtable offset = 0x78
    Character* _NV_getCallbackCharacter();// public RVA = 0x70BD70 vtable offset = 0x78
    virtual RootObject* getCallbackObject() const;// public RVA = 0x70BDA0 vtable offset = 0x80
    RootObject* _NV_getCallbackObject() const;// public RVA = 0x70BDA0 vtable offset = 0x80
    Item* getSelectedItem(const std::string& sectionName);// public RVA = 0x70FA80
    MyGUI::types::TCoord<int> getWindowCoord();// public RVA = 0x70BE90
    static StringPair stealingGUIInfo; // RVA = 0x1F36640 Static Member
    void stealingGUIInfoUpdate();// public RVA = 0x7116B0
    void getTrader1Trader2(InventoryGUI*& trader1, InventoryGUI*& trader2);// public RVA = 0x70CA60
    static Character* getNPCTrader();// public RVA = 0x70D060
    static RootObject* isTradingForMoney_static();// public RVA = 0x70FBA0
    static RootObject* isTradingAndStealing_static();// public RVA = 0x7115E0
    static float getTraderPriceMultiplier();// public RVA = 0x710480
    // Typedef        : StringInventorySectionGUIMap
    // no_addr void InventoryGUI(const class InventoryGUI & _a1);// public missing arg names
    InventoryGUI(Inventory* inv, InventoryLayout* layout, RootObject* callback);// protected RVA = 0x7157D0
    InventoryGUI* _CONSTRUCTOR(Inventory* inv, InventoryLayout* layout, RootObject* callback);// protected RVA = 0x7157D0
    virtual ~InventoryGUI();// protected RVA = 0x70F620 vtable offset = 0x0
    void _DESTRUCTOR();// protected RVA = 0x70F620 vtable offset = 0x0
    // no_addr void init();// protected
    bool pickupItemToMouse(const std::string sectionName);// protected RVA = 0x712730
    bool placeItemFromMouse(const std::string sectionName, const MyGUI::types::TPoint<int>& mousePos);// protected RVA = 0x7142F0
    Item* getMouseItem();// protected RVA = 0x710FE0
    Item* takeCertainAmountFrom(Item* baseItem, int amount);// protected RVA = 0x70DFD0
    InventoryGUI::TradeResult RClickAutoTrade(const std::string& invSectionName, int x, int y, InventoryGUI* sendingTo, bool thieveryChecks, bool first);// protected RVA = 0x712AB0
    InventoryGUI::TradeResult RClickAutoTradeAll(const std::string& invSectionName, int x, int y, InventoryGUI* sendingTo, bool thieveryChecks, bool first);// protected RVA = 0x7136C0
    bool isTradingForMoney(InventoryGUI* cameFrom);// protected RVA = 0x70ECA0
    bool isStealing(InventoryGUI* cameFrom);// protected RVA = 0x7104D0
    bool isWithinRangeToTrade(RootObject* otherOwner, bool stealing);// protected RVA = 0x70C020
    bool setItemToPlayerPortrait(Item* item, Character* character);// protected RVA = 0x711220
    void playSound(const char* eventId, Item* item);// protected RVA = 0x70BFC0
    struct FenceCallbackData
    {
        int mode; // 0x0 Member
        std::string sectionName; // 0x8 Member
        MyGUI::types::TPoint<int> point; // 0x30 Member
        // no_addr void FenceCallbackData(const struct InventoryGUI::FenceCallbackData & _a1);// public missing arg names
        FenceCallbackData();// public RVA = 0x71B720
        FenceCallbackData* _CONSTRUCTOR();// public RVA = 0x71B720
        ~FenceCallbackData();// public RVA = 0x71B740
        void _DESTRUCTOR();// public RVA = 0x71B740
        // no_addr struct InventoryGUI::FenceCallbackData & operator=(const struct InventoryGUI::FenceCallbackData & _a1);// public missing arg names
        // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
    };
    static InventoryGUI::FenceCallbackData fenceCallbackData; // RVA = 0x1F366C0 Static Member
    bool fencingConfirmation(Item* item, RootObject* thief, Character* fence, const std::string& sectionName, MyGUI::types::TPoint<int> mouse);// protected RVA = 0x714020
    void fencingConfirmationCallback(int b);// protected RVA = 0x713F50
    RootObject* getPlayerTradeCharacter();// protected RVA = 0x70E3B0
    void refreshAllSections();// protected RVA = 0x70E150
    void updateDatapanel();// protected RVA = 0x70BB50
    void rightClickAutoEquipping();// protected RVA = 0x7137B0
    bool tryToEquip(Item* item, bool replace);// protected RVA = 0x70D830
    void returnItem(Item* item, const std::string& sectionName);// protected RVA = 0x70C5F0
    InventorySection* getSection(const std::string& sectionName);// protected RVA = 0x70BF40
    InventorySection* getSection(InventorySectionGUI* sect);// protected RVA = 0x70CCB0
    bool hasMouse() const;// protected RVA = 0x70C420
    std::string getSectionWithMouseLocal();// protected RVA = 0x70D5F0
    iVector2 getSlotWithMouse();// protected RVA = 0x70F870
    void sectionMouseButtonPressed(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id);// protected RVA = 0x715590
    void sectionMouseButtonReleased(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id);// protected RVA = 0x715730
    void onWindowFocus(MyGUI::Widget* _sender, bool _focus);// protected RVA = 0x70BBC0
    void windowMoved(MyGUI::Window* _sender);// protected RVA = 0x70BC30
    static InventoryIcon* makeIconForItem(Item* item, int x, int y);// protected RVA = 0x710410
    InventoryLayout* layoutMgr; // 0x30 Member
    InventoryGUI* ownerInventory; // 0x38 Member
    InventoryGUI* childInventory; // 0x40 Member
    bool mouseFocus; // 0x48 Member
    RootObject* callbackObject; // 0x50 Member
    bool needItemsUpdate; // 0x58 Member
    bool visible; // 0x59 Member
    std::map<std::string, InventorySectionGUI*, std::less<std::string >, Ogre::STLAllocator<std::pair<std::string const, InventorySectionGUI*>, Ogre::GeneralAllocPolicy > > inventorySections; // 0x60 Member
    static ToolTipInventory* toolTip; // RVA = 0x212FAE0 Static Member
    static hand currentTradeTown; // RVA = 0x1F366A0 Static Member
    static std::map<InventoryGUI*, InventoryGUI::InventoryTradeData, std::less<InventoryGUI*>, Ogre::STLAllocator<std::pair<InventoryGUI* const, InventoryGUI::InventoryTradeData>, Ogre::GeneralAllocPolicy > > traders; // RVA = 0x212FB00 Static Member
    // no_addr class InventoryGUI & operator=(const class InventoryGUI & _a1);// public missing arg names
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// protected vtable offset = 0x0 missing arg names
};

class DatapanelGUI;
class GameDataCopyStandalone;

class InventoryLayout : public wraps::BaseLayout, public Ogre::GeneralAllocatedObject
{
public:
    // wraps::BaseLayout offset = 0x0, length = 0xA0
    // Ogre::AllocatedObject<Ogre::CategorisedAllocPolicy<0> > offset = 0xA1, length = 0x1
    // no_addr void InventoryLayout(const class InventoryLayout & _a1);// public missing arg names
    InventoryLayout(const std::string& file);// public RVA = 0x153710
    InventoryLayout* _CONSTRUCTOR(const std::string& file);// public RVA = 0x153710
    virtual ~InventoryLayout();// public RVA = 0x1538B0 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x1538B0 vtable offset = 0x0
    virtual void setupSections(InventoryGUI* _a1, std::map<std::string, InventorySectionGUI*, std::less<std::string >, Ogre::STLAllocator<std::pair<std::string const, InventorySectionGUI*>, Ogre::GeneralAllocPolicy > >& _a2, Inventory* _a3) = 0;// public vtable offset = 0x8 missing arg names
    MyGUI::Window* getWindow() const;// public RVA = 0x14D940
    MyGUI::Widget* getWidget(const std::string& name);// public RVA = 0x14E340
    virtual DatapanelGUI* getDatapanel() const;// public RVA = 0x14D950 vtable offset = 0x10
    DatapanelGUI* _NV_getDatapanel() const;// public RVA = 0x14D950 vtable offset = 0x10
    void setupDataPanelInfos(const std::string& name);// public RVA = 0x14DFF0
    DatapanelGUI* datapanel; // 0xA8 Member
    static void notifyCellSizeChanged();// public RVA = 0x14DEF0
    GameDataCopyStandalone dataPanelInfos; // 0xB0 Member
    MyGUI::Window* window; // 0x3B0 Member
    InventorySectionGUI* createSectionGUI(InventorySection* section);// protected RVA = 0x14E2B0
    void setSectionGUIDisabled(const std::string& sectionName, int width, int height);// protected RVA = 0x14E380
    MyGUI::types::TSize<int> resizeSection(InventorySection* section, InventorySectionGUI* sectionGUI);// protected RVA = 0x14DD30
    MyGUI::types::TSize<int> resizeSectionWidget(int width, int height, MyGUI::Widget* widget);// protected RVA = 0x14DCB0
    static MyGUI::types::TSize<int> CellSize; // RVA = 0x2125450 Static Member
    // no_addr class InventoryLayout & operator=(const class InventoryLayout & _a1);// public missing arg names
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};

// TODO move?
class GenericFixedInventoryLayout : public InventoryLayout
{
public:
    // InventoryLayout offset = 0x0, length = 0x3B8
    // no_addr void GenericFixedInventoryLayout(const class GenericFixedInventoryLayout & _a1);// public missing arg names
    GenericFixedInventoryLayout();// protected RVA = 0x154FD0
    GenericFixedInventoryLayout* _CONSTRUCTOR();// protected RVA = 0x154FD0
    virtual void setSize(int slotsW, int slotsH);// protected RVA = 0x14E750 vtable offset = 0x18
    void _NV_setSize(int slotsW, int slotsH);// protected RVA = 0x14E750 vtable offset = 0x18
    virtual ~GenericFixedInventoryLayout();// public RVA = 0x161F40 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x161F40 vtable offset = 0x0
    // no_addr class GenericFixedInventoryLayout & operator=(const class GenericFixedInventoryLayout & _a1);// public missing arg names
    // no_addr void __local_vftable_ctor_closure();// public
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};

// TODO move?
class BackpackInventoryLayout : public GenericFixedInventoryLayout
{
public:
    // GenericFixedInventoryLayout offset = 0x0, length = 0x3B8
    // no_addr void BackpackInventoryLayout(const class BackpackInventoryLayout & _a1);// public missing arg names
    BackpackInventoryLayout(bool standAlone);// public RVA = 0x155D80
    BackpackInventoryLayout* _CONSTRUCTOR(bool standAlone);// public RVA = 0x155D80
    virtual void setupSections(InventoryGUI* inventoryGUI, std::map<std::string, InventorySectionGUI*, std::less<std::string >, Ogre::STLAllocator<std::pair<std::string const, InventorySectionGUI*>, Ogre::GeneralAllocPolicy > >& sections, Inventory* inventory) override;// public RVA = 0x150200 vtable offset = 0x0
    void _NV_setupSections(InventoryGUI* inventoryGUI, std::map<std::string, InventorySectionGUI*, std::less<std::string >, Ogre::STLAllocator<std::pair<std::string const, InventorySectionGUI*>, Ogre::GeneralAllocPolicy > >& sections, Inventory* inventory);// public RVA = 0x150200 vtable offset = 0x0
    virtual ~BackpackInventoryLayout();// public RVA = 0x161F80 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x161F80 vtable offset = 0x0
    // no_addr class BackpackInventoryLayout & operator=(const class BackpackInventoryLayout & _a1);// public missing arg names
    // no_addr void __dflt_ctor_closure();// public
    // no_addr void __local_vftable_ctor_closure();// public
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};