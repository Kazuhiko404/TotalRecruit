#pragma once

#include "ProductionBuilding.h"
#include "../gui/InventoryGUI.h"

class FurnaceInventoryLayout : public BuildInventoryLayout
{
public:
    // BuildInventoryLayout offset = 0x0, length = 0x440
    // no_addr void FurnaceInventoryLayout(const class FurnaceInventoryLayout & _a1);// public missing arg names
    FurnaceInventoryLayout(const std::string& title, int ins, int outs);// public RVA = 0x157090
    FurnaceInventoryLayout* _CONSTRUCTOR(const std::string& title, int ins, int outs);// public RVA = 0x157090
    virtual void setupSections(InventoryGUI* inventoryGUI, std::map<std::string, InventorySectionGUI*, std::less<std::string >, Ogre::STLAllocator<std::pair<std::string const, InventorySectionGUI*>, Ogre::GeneralAllocPolicy > >& sections, Inventory* inventory) override;// public RVA = 0x151910 vtable offset = 0x0
    void _NV_setupSections(InventoryGUI* inventoryGUI, std::map<std::string, InventorySectionGUI*, std::less<std::string >, Ogre::STLAllocator<std::pair<std::string const, InventorySectionGUI*>, Ogre::GeneralAllocPolicy > >& sections, Inventory* inventory);// public RVA = 0x151910 vtable offset = 0x0
    virtual ~FurnaceInventoryLayout();// public RVA = 0x161FC0 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x161FC0 vtable offset = 0x0
    // no_addr class FurnaceInventoryLayout & operator=(const class FurnaceInventoryLayout & _a1);// public missing arg names
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};

class FurnaceBuilding : public ProductionBuilding
{
public:
    // ProductionBuilding offset = 0x0, length = 0x490
    virtual InventoryLayout* createInventoryLayout() override;// public RVA = 0x29A2C0 vtable offset = 0x0
    InventoryLayout* _NV_createInventoryLayout();// public RVA = 0x29A2C0 vtable offset = 0x0
    virtual void setupFromData() override;// public RVA = 0x29F4D0 vtable offset = 0x0
    void _NV_setupFromData();// public RVA = 0x29F4D0 vtable offset = 0x0
    virtual void getGUIData(DatapanelGUI* datapanel, int category) override;// public RVA = 0x30B2A0 vtable offset = 0x0
    void _NV_getGUIData(DatapanelGUI* datapanel, int category);// public RVA = 0x30B2A0 vtable offset = 0x0
    float getInputValueTotal();// public RVA = 0x299370
    virtual float getInputValue(Item* item) override;// public RVA = 0x2A5A90 vtable offset = 0x0
    float _NV_getInputValue(Item* item);// public RVA = 0x2A5A90 vtable offset = 0x0
    virtual TaskType getDefaultTask() override;// public RVA = 0x2ADA50 vtable offset = 0x0
    TaskType _NV_getDefaultTask();// public RVA = 0x2ADA50 vtable offset = 0x0
    virtual void getResourcesNeededBecauseNotFull(lektor<GameData*>& out) override;// public RVA = 0x296A70 vtable offset = 0x0
    void _NV_getResourcesNeededBecauseNotFull(lektor<GameData*>& out);// public RVA = 0x296A70 vtable offset = 0x0
    virtual void getResourcesNeededBecauseEmpty(lektor<GameData*>& out) override;// public RVA = 0x29C3A0 vtable offset = 0x0
    void _NV_getResourcesNeededBecauseEmpty(lektor<GameData*>& out);// public RVA = 0x29C3A0 vtable offset = 0x0
    virtual bool canHaveSomeOfThese(GameData* input) override;// public RVA = 0x29C710 vtable offset = 0x0
    bool _NV_canHaveSomeOfThese(GameData* input);// public RVA = 0x29C710 vtable offset = 0x0
    void incinerate(DataPanelLine* _a1);// public RVA = 0x2992E0 missing arg names
    // no_addr void FurnaceBuilding(const class FurnaceBuilding & _a1);// public missing arg names
    FurnaceBuilding(GameData* data, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, Faction* _participant, const hand& town, const hand& _handle, Layout* __isfurnitureOf, Building* _indoors);// protected RVA = 0x2A5200
    FurnaceBuilding* _CONSTRUCTOR(GameData* data, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, Faction* _participant, const hand& town, const hand& _handle, Layout* __isfurnitureOf, Building* _indoors);// protected RVA = 0x2A5200
    virtual ~FurnaceBuilding();// protected RVA = 0x299A90 vtable offset = 0x0
    void _DESTRUCTOR();// protected RVA = 0x299A90 vtable offset = 0x0
    virtual void operate(Character* stats, float amount) override;// protected RVA = 0x299AF0 vtable offset = 0x0
    void _NV_operate(Character* stats, float amount);// protected RVA = 0x299AF0 vtable offset = 0x0
    virtual void updateInputs(float rate) override;// protected RVA = 0x297730 vtable offset = 0x0
    void _NV_updateInputs(float rate);// protected RVA = 0x297730 vtable offset = 0x0
    virtual void updateOutput(float rate) override;// protected RVA = 0x297740 vtable offset = 0x0
    void _NV_updateOutput(float rate);// protected RVA = 0x297740 vtable offset = 0x0
    virtual bool limitInputsOutputRate(float& rate) override;// protected RVA = 0x297750 vtable offset = 0x0
    bool _NV_limitInputsOutputRate(float& rate);// protected RVA = 0x297750 vtable offset = 0x0
    float getIronAmountInItem(GameData* data) const;// protected RVA = 0x2A52A0
    bool active; // 0x490 Member
    // no_addr class FurnaceBuilding & operator=(const class FurnaceBuilding & _a1);// public missing arg names
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// protected vtable offset = 0x0 missing arg names
};