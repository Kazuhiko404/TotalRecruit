#pragma once

#include "StorageBuilding.h"

class BuildInventoryLayout : public InventoryLayout
{
public:
    // InventoryLayout offset = 0x0, length = 0x3B8
    // no_addr void BuildInventoryLayout(const class BuildInventoryLayout & _a1);// public missing arg names
    BuildInventoryLayout(const std::string& title, int ins, int outs, bool hasQueue);// public RVA = 0x155FE0
    BuildInventoryLayout* _CONSTRUCTOR(const std::string& title, int ins, int outs, bool hasQueue);// public RVA = 0x155FE0
    virtual void setupSections(InventoryGUI* inventoryGUI, std::map<std::string, InventorySectionGUI*, std::less<std::string >, Ogre::STLAllocator<std::pair<std::string const, InventorySectionGUI*>, Ogre::GeneralAllocPolicy > >& sections, Inventory* inventory) override;// public RVA = 0x150D00 vtable offset = 0x0
    void _NV_setupSections(InventoryGUI* inventoryGUI, std::map<std::string, InventorySectionGUI*, std::less<std::string >, Ogre::STLAllocator<std::pair<std::string const, InventorySectionGUI*>, Ogre::GeneralAllocPolicy > >& sections, Inventory* inventory);// public RVA = 0x150D00 vtable offset = 0x0
    void setInput(int inputIndex, const std::string& name, const std::string& status);// public RVA = 0x14DA40
    void setOutput(const std::string& name);// public RVA = 0x14DB50
    void setInputProgress(int inputIdx, float progress);// public RVA = 0x14DBC0
    void setInputEnabled(int inputIdx, bool value);// public RVA = 0x14E910
    void setOutputProgress(float progress);// public RVA = 0x14DC40
    void setInputItem(int inputIdx, Item* item, bool visible);// public RVA = 0x14DDB0
    void setOutputItem(Item* item, bool visible);// public RVA = 0x14E040
    MyGUI::TextBox* input1NameText; // 0x3B8 Member
    MyGUI::TextBox* input2NameText; // 0x3C0 Member
    MyGUI::TextBox* input1StatusText; // 0x3C8 Member
    MyGUI::TextBox* input2StatusText; // 0x3D0 Member
    MyGUI::ImageBox* input1ItemIcon; // 0x3D8 Member
    MyGUI::ImageBox* input2ItemIcon; // 0x3E0 Member
    MyGUI::TextBox* outputNameText; // 0x3E8 Member
    MyGUI::ImageBox* outputItemIcon; // 0x3F0 Member
    MyGUI::Widget* input1Panel; // 0x3F8 Member
    MyGUI::Widget* input2Panel; // 0x400 Member
    MyGUI::ProgressBar* input1Progress; // 0x408 Member
    MyGUI::ProgressBar* input2Progress; // 0x410 Member
    MyGUI::ProgressBar* outputProgress; // 0x418 Member
    int inputs; // 0x420 Member
    int outputs; // 0x424 Member
    Item* input1Item; // 0x428 Member
    Item* input2Item; // 0x430 Member
    Item* outputItem; // 0x438 Member
    virtual ~BuildInventoryLayout();// public RVA = 0x161FA0 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x161FA0 vtable offset = 0x0
    // no_addr class BuildInventoryLayout & operator=(const class BuildInventoryLayout & _a1);// public missing arg names
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};

class ProductionInventoryLayout : public BuildInventoryLayout
{
public:
    // BuildInventoryLayout offset = 0x0, length = 0x440
    // no_addr void ProductionInventoryLayout(const class ProductionInventoryLayout & _a1);// public missing arg names
    ProductionInventoryLayout(const std::string& title, int ins, int outs);// public RVA = 0x157060
    ProductionInventoryLayout* _CONSTRUCTOR(const std::string& title, int ins, int outs);// public RVA = 0x157060
    virtual ~ProductionInventoryLayout();// public RVA = 0x161FB0 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x161FB0 vtable offset = 0x0
    // no_addr class ProductionInventoryLayout & operator=(const class ProductionInventoryLayout & _a1);// public missing arg names
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};

class ProductionBuilding : public StorageBuilding
{
public:
    // StorageBuilding offset = 0x0, length = 0x468
    enum ProductionState
    {
        PRODUCTION_STARVED,
        PRODUCTION_FULL,
        PRODUCTION_NORMAL,
        PRODUCTION_IMPOSSIBLE
    };

    // no_addr void ProductionBuilding(const class ProductionBuilding & _a1);// public missing arg names
    ProductionBuilding(GameData* _data, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, Faction* _participant, const hand& town, const hand& _handle, Layout* __isfurnitureOf, Building* _indoors);// public RVA = 0x2A32A0
    ProductionBuilding* _CONSTRUCTOR(GameData* _data, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, Faction* _participant, const hand& town, const hand& _handle, Layout* __isfurnitureOf, Building* _indoors);// public RVA = 0x2A32A0
    virtual ~ProductionBuilding();// public RVA = 0x299480 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x299480 vtable offset = 0x0
    virtual ProductionBuilding* getProductionBuilding() override;// public RVA = 0xF6D80 vtable offset = 0x0
    ProductionBuilding* _NV_getProductionBuilding();// public RVA = 0xF6D80 vtable offset = 0x0
    virtual InventoryLayout* createInventoryLayout() override;// public RVA = 0x299D20 vtable offset = 0x0
    InventoryLayout* _NV_createInventoryLayout();// public RVA = 0x299D20 vtable offset = 0x0
    virtual void update() override;// public RVA = 0x29A8E0 vtable offset = 0x0
    void _NV_update();// public RVA = 0x29A8E0 vtable offset = 0x0
    virtual bool needsUpdate() override;// public RVA = 0x547A80 vtable offset = 0x0
    bool _NV_needsUpdate();// public RVA = 0x547A80 vtable offset = 0x0
    virtual void operate(Character* _a1, float amount) override;// public RVA = 0x298250 vtable offset = 0x0
    void _NV_operate(Character* _a1, float amount);// public RVA = 0x298250 vtable offset = 0x0
    virtual void getGUIData(DatapanelGUI* datapanel, int category) override;// public RVA = 0x2FFA00 vtable offset = 0x0
    void _NV_getGUIData(DatapanelGUI* datapanel, int category);// public RVA = 0x2FFA00 vtable offset = 0x0
    virtual void getGUIToolTipForGroundResourceEfficiency(lektor<StringPair>& out) override;// public RVA = 0x309C80 vtable offset = 0x0
    void _NV_getGUIToolTipForGroundResourceEfficiency(lektor<StringPair>& out);// public RVA = 0x309C80 vtable offset = 0x0
    virtual GameSaveState serialise(GameDataContainer* container, GameData* refList, PosRotPair* offsetPosToSubtract) override;// public RVA = 0x2A1AF0 vtable offset = 0x0
    GameSaveState _NV_serialise(GameDataContainer* container, GameData* refList, PosRotPair* offsetPosToSubtract);// public RVA = 0x2A1AF0 vtable offset = 0x0
    virtual void loadFromSerialise(GameSaveState* wholeState) override;// public RVA = 0x2A1EF0 vtable offset = 0x0
    void _NV_loadFromSerialise(GameSaveState* wholeState);// public RVA = 0x2A1EF0 vtable offset = 0x0
    virtual float getProductionMult() override;// public RVA = 0x546550 vtable offset = 0x0
    float _NV_getProductionMult();// public RVA = 0x546550 vtable offset = 0x0
    virtual float getProductionMultForGUI() override;// public RVA = 0x546590 vtable offset = 0x0
    float _NV_getProductionMultForGUI();// public RVA = 0x546590 vtable offset = 0x0
    virtual void setupMiningResourceLevel() override;// public RVA = 0x29C1C0 vtable offset = 0x0
    void _NV_setupMiningResourceLevel();// public RVA = 0x29C1C0 vtable offset = 0x0
    virtual float getMiningResourceLevel() const override;// public RVA = 0x296940 vtable offset = 0x0
    float _NV_getMiningResourceLevel() const;// public RVA = 0x296940 vtable offset = 0x0
    virtual CursorType getMouseCursor() override;// public RVA = 0x5465D0 vtable offset = 0x0
    CursorType _NV_getMouseCursor();// public RVA = 0x5465D0 vtable offset = 0x0
    virtual TaskType getDefaultTask() override;// public RVA = 0x546610 vtable offset = 0x0
    TaskType _NV_getDefaultTask();// public RVA = 0x546610 vtable offset = 0x0
    virtual float getInputValue(Item* item);// public RVA = 0xF6D90 vtable offset = 0x5A0
    float _NV_getInputValue(Item* item);// public RVA = 0xF6D90 vtable offset = 0x5A0
    virtual void getResourcesNeededBecauseEmpty(lektor<GameData*>& out) override;// public RVA = 0x299850 vtable offset = 0x0
    void _NV_getResourcesNeededBecauseEmpty(lektor<GameData*>& out);// public RVA = 0x299850 vtable offset = 0x0
    virtual void getResourcesNeededBecauseNotFull(lektor<GameData*>& out) override;// public RVA = 0x2996B0 vtable offset = 0x0
    void _NV_getResourcesNeededBecauseNotFull(lektor<GameData*>& out);// public RVA = 0x2996B0 vtable offset = 0x0
    virtual bool isAnyInputsEmpty() const override;// public RVA = 0x2986C0 vtable offset = 0x0
    bool _NV_isAnyInputsEmpty() const;// public RVA = 0x2986C0 vtable offset = 0x0
    virtual bool isAnyInputsInvalidType() const;// public RVA = 0xF6DA0 vtable offset = 0x5A8
    bool _NV_isAnyInputsInvalidType() const;// public RVA = 0xF6DA0 vtable offset = 0x5A8
    virtual bool isAnyInputsFull() override;// public RVA = 0x298630 vtable offset = 0x0
    bool _NV_isAnyInputsFull();// public RVA = 0x298630 vtable offset = 0x0
    virtual bool isProductionFull() override;// public RVA = 0x296B00 vtable offset = 0x0
    bool _NV_isProductionFull();// public RVA = 0x296B00 vtable offset = 0x0
    virtual bool isProductionEmpty() override;// public RVA = 0x296B40 vtable offset = 0x0
    bool _NV_isProductionEmpty();// public RVA = 0x296B40 vtable offset = 0x0
    virtual bool canHaveSomeOfThese(GameData* these) override;// public RVA = 0x298500 vtable offset = 0x0
    bool _NV_canHaveSomeOfThese(GameData* these);// public RVA = 0x298500 vtable offset = 0x0
    virtual float getOutputBasedRotationSpeedMult() override;// public RVA = 0x547AC0 vtable offset = 0x0
    float _NV_getOutputBasedRotationSpeedMult();// public RVA = 0x547AC0 vtable offset = 0x0
    virtual void getItemsWeWantRidOf(lektor<GameData*>& out, bool loot) override;// public RVA = 0x299650 vtable offset = 0x0
    void _NV_getItemsWeWantRidOf(lektor<GameData*>& out, bool loot);// public RVA = 0x299650 vtable offset = 0x0
    float getOutput() const;// public RVA = 0x546630
    virtual int getNumConsumtionItems() const override;// public RVA = 0xF7080 vtable offset = 0x0
    int _NV_getNumConsumtionItems() const;// public RVA = 0xF7080 vtable offset = 0x0
    virtual StorageBuilding::ConsumptionItem* getConsumtionItems(int id) override;// public RVA = 0x547B40 vtable offset = 0x0
    StorageBuilding::ConsumptionItem* _NV_getConsumtionItems(int id);// public RVA = 0x547B40 vtable offset = 0x0
    virtual void setProductionItem(GameData* _itemData, int stack, float progress01);// public RVA = 0x2A37E0 vtable offset = 0x5B0
    void _NV_setProductionItem(GameData* _itemData, int stack, float progress01);// public RVA = 0x2A37E0 vtable offset = 0x5B0
    ProductionBuilding::ProductionState productionState; // 0x468 Member
    virtual void setupFromData() override;// protected RVA = 0x29E300 vtable offset = 0x0
    void _NV_setupFromData();// protected RVA = 0x29E300 vtable offset = 0x0
    virtual void updateInventoryWindow() override;// protected RVA = 0x298360 vtable offset = 0x0
    void _NV_updateInventoryWindow();// protected RVA = 0x298360 vtable offset = 0x0
    virtual void updateInputs(float rate);// protected RVA = 0x299EC0 vtable offset = 0x5B8
    void _NV_updateInputs(float rate);// protected RVA = 0x299EC0 vtable offset = 0x5B8
    virtual void updateOutput(float rate);// protected RVA = 0x298000 vtable offset = 0x5C0
    void _NV_updateOutput(float rate);// protected RVA = 0x298000 vtable offset = 0x5C0
    virtual bool limitInputsOutputRate(float& rate);// protected RVA = 0x29A060 vtable offset = 0x5C8
    bool _NV_limitInputsOutputRate(float& rate);// protected RVA = 0x29A060 vtable offset = 0x5C8
    virtual void getGUIFertility(DatapanelGUI* datapanel, int category);// protected RVA = 0x309590 vtable offset = 0x5D0
    void _NV_getGUIFertility(DatapanelGUI* datapanel, int category);// protected RVA = 0x309590 vtable offset = 0x5D0
    virtual void getGUIState(DatapanelGUI* datapanel, int category) override;// protected RVA = 0x2FFBE0 vtable offset = 0x0
    void _NV_getGUIState(DatapanelGUI* datapanel, int category);// protected RVA = 0x2FFBE0 vtable offset = 0x0
    float _resourceMiningLevel; // 0x46C Member
    InventorySection* outSection; // 0x470 Member
    lektor<StorageBuilding::ConsumptionItem> consumptionItems; // 0x478 Member
    // no_addr class ProductionBuilding & operator=(const class ProductionBuilding & _a1);// public missing arg names
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};