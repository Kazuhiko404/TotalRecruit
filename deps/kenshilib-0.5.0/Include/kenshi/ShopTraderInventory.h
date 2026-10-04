#pragma once

#include "Inventory.h"

class ShopTraderInventorySection : public InventorySection
{
public:
    // InventorySection offset = 0x0, length = 0xD8
    // no_addr void ShopTraderInventorySection(const class ShopTraderInventorySection & _a1);// public missing arg names
    ShopTraderInventorySection(const std::string& name, int _w, int _h, Inventory* inv);// public RVA = 0x951030
    ShopTraderInventorySection* _CONSTRUCTOR(const std::string& name, int _w, int _h, Inventory* inv);// public RVA = 0x951030
    virtual bool addItem(Item* item, int quantity) override;// public RVA = 0x9533F0 vtable offset = 0x0
    bool _NV_addItem(Item* item, int quantity);// public RVA = 0x9533F0 vtable offset = 0x0
    virtual void autoArrange() override;// public RVA = 0x951090 vtable offset = 0x0
    void _NV_autoArrange();// public RVA = 0x951090 vtable offset = 0x0
    virtual ~ShopTraderInventorySection();// public RVA = 0x95C9A0 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x95C9A0 vtable offset = 0x0
    // no_addr class ShopTraderInventorySection & operator=(const class ShopTraderInventorySection & _a1);// public missing arg names
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};

class ShopTraderInventory : public Inventory
{
public:
    // Inventory offset = 0x0, length = 0x98
    // no_addr void ShopTraderInventory(const class ShopTraderInventory & _a1);// public missing arg names
    ShopTraderInventory(RootObject* owner, Ogre::vector<InventorySection*>::type& inventoriesList);// public RVA = 0x951980
    ShopTraderInventory* _CONSTRUCTOR(RootObject* owner, Ogre::vector<InventorySection*>::type& inventoriesList);// public RVA = 0x951980
    virtual ~ShopTraderInventory();// public RVA = 0x953280 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x953280 vtable offset = 0x0
    virtual void updateInventory();// public RVA = 0x951120 vtable offset = 0x70
    void _NV_updateInventory();// public RVA = 0x951120 vtable offset = 0x70
    virtual void dropItem(Item* item) override;// public RVA = 0x950F70 vtable offset = 0x0
    void _NV_dropItem(Item* item);// public RVA = 0x950F70 vtable offset = 0x0
    virtual void refreshGui() override;// public RVA = 0x950F50 vtable offset = 0x0
    void _NV_refreshGui();// public RVA = 0x950F50 vtable offset = 0x0
    virtual InventorySection* initialiseNewSection(const std::string& name, int w, int h, AttachSlot _a4, bool equipCallbacks, bool _a6, bool _a7, int _a8) override;// public RVA = 0x9547A0 vtable offset = 0x0
    InventorySection* _NV_initialiseNewSection(const std::string& name, int w, int h, AttachSlot _a4, bool equipCallbacks, bool _a6, bool _a7, int _a8);// public RVA = 0x9547A0 vtable offset = 0x0
    virtual bool _addItem(Item* item, int quantity);// protected RVA = 0x950F80 vtable offset = 0x78
    bool _NV__addItem(Item* item, int quantity);// protected RVA = 0x950F80 vtable offset = 0x78
    bool _addItemToInventories(Item* item);// protected RVA = 0x951230
    void _removeItemFromInventories(Item* item, int quantity);// protected RVA = 0x9512C0
    virtual void _sectionAddItemCallback(Item* item) override;// protected RVA = 0x9514C0 vtable offset = 0x0
    void _NV__sectionAddItemCallback(Item* item);// protected RVA = 0x9514C0 vtable offset = 0x0
    virtual void _sectionUpdateItemCallback(Item* item, int prevQuantity) override;// protected RVA = 0x951540 vtable offset = 0x0
    void _NV__sectionUpdateItemCallback(Item* item, int prevQuantity);// protected RVA = 0x951540 vtable offset = 0x0
    virtual void _sectionRemoveItemCallback(Item* item) override;// protected RVA = 0x9515F0 vtable offset = 0x0
    void _NV__sectionRemoveItemCallback(Item* item);// protected RVA = 0x9515F0 vtable offset = 0x0
    ogre_unordered_map<hand, InventorySection*>::type inventories; // 0x98 Member
    ShopTraderInventorySection* section; // 0xD8 Member
    static bool Updating; // RVA = 0x2132458 Static Member
    // no_addr class ShopTraderInventory & operator=(const class ShopTraderInventory & _a1);// public missing arg names
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};
