#pragma once

#include "UseableStuff.h"
#include <mygui/MyGUI.h>
#include "../gui/InventoryGUI.h"

class ResearchBuildingInventoryLayout : public GenericInventoryLayout
{
public:
    // GenericInventoryLayout offset = 0x0, length = 0x3C0
    // no_addr void ResearchBuildingInventoryLayout(const class ResearchBuildingInventoryLayout & _a1);// public missing arg names
    ResearchBuildingInventoryLayout();// public RVA = 0x155960
    ResearchBuildingInventoryLayout* _CONSTRUCTOR();// public RVA = 0x155960
    virtual void setupSections(InventoryGUI* inventoryGUI, std::map<std::string, InventorySectionGUI*, std::less<std::string >, Ogre::STLAllocator<std::pair<std::string const, InventorySectionGUI*>, Ogre::GeneralAllocPolicy > >& sections, Inventory* inventory) override;// public RVA = 0x14FFD0 vtable offset = 0x0
    void _NV_setupSections(InventoryGUI* inventoryGUI, std::map<std::string, InventorySectionGUI*, std::less<std::string >, Ogre::STLAllocator<std::pair<std::string const, InventorySectionGUI*>, Ogre::GeneralAllocPolicy > >& sections, Inventory* inventory);// public RVA = 0x14FFD0 vtable offset = 0x0
    MyGUI::Widget* getResearchButton() const;// public RVA = 0x14D9D0
    MyGUI::Button* researchButton; // 0x3C0 Member
    virtual ~ResearchBuildingInventoryLayout();// public RVA = 0x161F70 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x161F70 vtable offset = 0x0
    // no_addr class ResearchBuildingInventoryLayout & operator=(const class ResearchBuildingInventoryLayout & _a1);// public missing arg names
    // no_addr void __local_vftable_ctor_closure();// public
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};

class ResearchBuilding : public UseableStuff
{
public:
    // UseableStuff offset = 0x0, length = 0x440
    // no_addr void ResearchBuilding(const class ResearchBuilding & _a1);// public missing arg names
    ResearchBuilding(GameData* _data, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, Faction* _participant, const hand& town, const hand& _handle, Layout* __isfurnitureOf, Building* _indoors);// public RVA = 0x2A2330
    ResearchBuilding* _CONSTRUCTOR(GameData* _data, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, Faction* _participant, const hand& town, const hand& _handle, Layout* __isfurnitureOf, Building* _indoors);// public RVA = 0x2A2330
    virtual ~ResearchBuilding();// public RVA = 0x2AD910 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x2AD910 vtable offset = 0x0
    virtual bool dontNeedWorkRightNow() const override;// public RVA = 0x299130 vtable offset = 0x0
    bool _NV_dontNeedWorkRightNow() const;// public RVA = 0x299130 vtable offset = 0x0
    virtual TaskType getDefaultTask() override;// public RVA = 0x2AD930 vtable offset = 0x0
    TaskType _NV_getDefaultTask();// public RVA = 0x2AD930 vtable offset = 0x0
    virtual void update() override;// public RVA = 0x297150 vtable offset = 0x0
    void _NV_update();// public RVA = 0x297150 vtable offset = 0x0
    virtual void operate(Character* stats, float amount) override;// public RVA = 0x29B420 vtable offset = 0x0
    void _NV_operate(Character* stats, float amount);// public RVA = 0x29B420 vtable offset = 0x0
    virtual void notifyConstructionComplete() override;// public RVA = 0x29B950 vtable offset = 0x0
    void _NV_notifyConstructionComplete();// public RVA = 0x29B950 vtable offset = 0x0
    virtual void getGUIData(DatapanelGUI* datapanel, int category) override;// public RVA = 0x3091D0 vtable offset = 0x0
    void _NV_getGUIData(DatapanelGUI* datapanel, int category);// public RVA = 0x3091D0 vtable offset = 0x0
    virtual int getTechLevel();// public RVA = 0x2AD990 vtable offset = 0x548
    int _NV_getTechLevel();// public RVA = 0x2AD990 vtable offset = 0x548
    virtual InventoryLayout* createInventoryLayout() override;// public RVA = 0x29B340 vtable offset = 0x0
    InventoryLayout* _NV_createInventoryLayout();// public RVA = 0x29B340 vtable offset = 0x0
    void showResearchWindow(MyGUI::Widget* _a1);// protected RVA = 0x296E20 missing arg names
    // no_addr class ResearchBuilding & operator=(const class ResearchBuilding & _a1);// public missing arg names
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};