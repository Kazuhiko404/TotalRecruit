#pragma once

#include "InventoryGUI.h"   

class TraderInventoryLayout : public InventoryLayout
{
public:
    // InventoryLayout offset = 0x0, length = 0x3B8
    // no_addr void TraderInventoryLayout(const class TraderInventoryLayout & _a1);// public missing arg names
    TraderInventoryLayout();// public RVA = 0x155F30
    TraderInventoryLayout* _CONSTRUCTOR();// public RVA = 0x155F30
    virtual void setupSections(InventoryGUI* inventoryGUI, std::map<std::string, InventorySectionGUI*, std::less<std::string >, Ogre::STLAllocator<std::pair<std::string const, InventorySectionGUI*>, Ogre::GeneralAllocPolicy > >& sections, Inventory* inventory) override;// public RVA = 0x150450 vtable offset = 0x0
    void _NV_setupSections(InventoryGUI* inventoryGUI, std::map<std::string, InventorySectionGUI*, std::less<std::string >, Ogre::STLAllocator<std::pair<std::string const, InventorySectionGUI*>, Ogre::GeneralAllocPolicy > >& sections, Inventory* inventory);// public RVA = 0x150450 vtable offset = 0x0
    void resize(std::map<std::string, InventorySectionGUI*, std::less<std::string >, Ogre::STLAllocator<std::pair<std::string const, InventorySectionGUI*>, Ogre::GeneralAllocPolicy > >& sections, Inventory* inventory);// public RVA = 0x150B80
    void notifyMouseWheel(MyGUI::Widget* _sender, int _rel);// protected RVA = 0x14D9E0
    MyGUI::ScrollView* scrollBackpack; // 0x3B8 Member
    virtual ~TraderInventoryLayout();// public RVA = 0x161F90 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x161F90 vtable offset = 0x0
    // no_addr class TraderInventoryLayout & operator=(const class TraderInventoryLayout & _a1);// public missing arg names
    // no_addr void __local_vftable_ctor_closure();// public
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};

class InventoryTraderGUI : public InventoryGUI
{
public:
    // InventoryGUI offset = 0x0, length = 0x88
    // no_addr void InventoryTraderGUI(const class InventoryTraderGUI & _a1);// public missing arg names
    InventoryTraderGUI(Inventory* inv, InventoryLayout* layout, RootObject* callback);// public RVA = 0x715BC0
    InventoryTraderGUI* _CONSTRUCTOR(Inventory* inv, InventoryLayout* layout, RootObject* callback);// public RVA = 0x715BC0
    virtual ~InventoryTraderGUI();// public RVA = 0x70FC70 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x70FC70 vtable offset = 0x0
    virtual Inventory* getInventory() override;// public RVA = 0x70C2B0 vtable offset = 0x0
    Inventory* _NV_getInventory();// public RVA = 0x70C2B0 vtable offset = 0x0
    virtual Character* getCallbackCharacter() override;// public RVA = 0x70C2D0 vtable offset = 0x0
    Character* _NV_getCallbackCharacter();// public RVA = 0x70C2D0 vtable offset = 0x0
    virtual RootObject* getCallbackObject() const override;// public RVA = 0x70C2F0 vtable offset = 0x0
    RootObject* _NV_getCallbackObject() const;// public RVA = 0x70C2F0 vtable offset = 0x0
    virtual void refreshSection(InventorySection* section) override;// protected RVA = 0x712280 vtable offset = 0x0
    void _NV_refreshSection(InventorySection* section);// protected RVA = 0x712280 vtable offset = 0x0
    int sectionWidth; // 0x88 Member
    int sectionHeight; // 0x8C Member
    // no_addr class InventoryTraderGUI & operator=(const class InventoryTraderGUI & _a1);// public missing arg names
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};