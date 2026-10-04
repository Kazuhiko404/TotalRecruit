#pragma once

#include "UseableStuff.h"
#include "../gui/InventoryGUI.h"

class InventorySection;

// TODO move?
class BuildingContainerInventoryLayout : public GenericInventoryLayout
{
public:
    // GenericInventoryLayout offset = 0x0, length = 0x3C0
    // no_addr void BuildingContainerInventoryLayout(const class BuildingContainerInventoryLayout & _a1);// public missing arg names
    BuildingContainerInventoryLayout();// public RVA = 0x155920
    BuildingContainerInventoryLayout* _CONSTRUCTOR();// public RVA = 0x155920
    virtual void setupSections(InventoryGUI* inventoryGUI, std::map<std::string, InventorySectionGUI*, std::less<std::string >, Ogre::STLAllocator<std::pair<std::string const, InventorySectionGUI*>, Ogre::GeneralAllocPolicy > >& sections, Inventory* inventory) override;// public RVA = 0x14FAE0 vtable offset = 0x0
    void _NV_setupSections(InventoryGUI* inventoryGUI, std::map<std::string, InventorySectionGUI*, std::less<std::string >, Ogre::STLAllocator<std::pair<std::string const, InventorySectionGUI*>, Ogre::GeneralAllocPolicy > >& sections, Inventory* inventory);// public RVA = 0x14FAE0 vtable offset = 0x0
    void setCapacity(int value, bool full);// public RVA = 0x14EA40
    MyGUI::EditBox* capacityText; // 0x3C0 Member
    virtual ~BuildingContainerInventoryLayout();// public RVA = 0x161F60 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x161F60 vtable offset = 0x0
    // no_addr class BuildingContainerInventoryLayout & operator=(const class BuildingContainerInventoryLayout & _a1);// public missing arg names
    // no_addr void __local_vftable_ctor_closure();// public
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};

class StorageBuilding : public UseableStuff
{
public:
    // UseableStuff offset = 0x0, length = 0x440
    class ConsumptionItem
    {
    public:
        ConsumptionItem(float _rate, GameData* _item, InventorySection* sect, int cap);// public RVA = 0x297790
        ConsumptionItem* _CONSTRUCTOR(float _rate, GameData* _item, InventorySection* sect, int cap);// public RVA = 0x297790
        ConsumptionItem();// public RVA = 0x297760
        ConsumptionItem* _CONSTRUCTOR();// public RVA = 0x297760
        bool isFull();// public RVA = 0x2977C0
        bool isEmpty();// public RVA = 0x296A80
        float amount; // 0x0 Member
        float rate; // 0x4 Member
        int maxCapacity; // 0x8 Member
        GameData* item; // 0x10 Member
        InventorySection* inventorySection; // 0x18 Member
    };
    // no_addr void StorageBuilding(const class StorageBuilding & _a1);// public missing arg names
    StorageBuilding(GameData* _data, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, Faction* _participant, const hand& town, const hand& _handle, Layout* __isfurnitureOf, Building* _indoors);// public RVA = 0x2A2C20
    StorageBuilding* _CONSTRUCTOR(GameData* _data, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, Faction* _participant, const hand& town, const hand& _handle, Layout* __isfurnitureOf, Building* _indoors);// public RVA = 0x2A2C20
    virtual ~StorageBuilding();// public RVA = 0x299410 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x299410 vtable offset = 0x0
    virtual StorageBuilding* getFunctionStuff() override;// public RVA = 0xF6D60 vtable offset = 0x0
    StorageBuilding* _NV_getFunctionStuff();// public RVA = 0xF6D60 vtable offset = 0x0
    virtual UseableStuff* getUseableStuff() override;// public RVA = 0xF6D70 vtable offset = 0x0
    UseableStuff* _NV_getUseableStuff();// public RVA = 0xF6D70 vtable offset = 0x0
    virtual void update() override;// public RVA = 0x296B60 vtable offset = 0x0
    void _NV_update();// public RVA = 0x296B60 vtable offset = 0x0
    virtual TaskType getDefaultTask() override;// public RVA = 0x2AAF20 vtable offset = 0x0
    TaskType _NV_getDefaultTask();// public RVA = 0x2AAF20 vtable offset = 0x0
    virtual void getItemsWeWantRidOf(lektor<GameData*>& out, bool forceLooting);// public RVA = 0x2994E0 vtable offset = 0x548
    void _NV_getItemsWeWantRidOf(lektor<GameData*>& out, bool forceLooting);// public RVA = 0x2994E0 vtable offset = 0x548
    virtual GameData* getProductionItemData();// public RVA = 0x29B5B0 vtable offset = 0x550
    GameData* _NV_getProductionItemData();// public RVA = 0x29B5B0 vtable offset = 0x550
    StorageBuilding::ConsumptionItem* getProductionItem() const;// public RVA = 0x595DA0
    virtual int getCurrentProductionQuantity();// public RVA = 0x296AD0 vtable offset = 0x558
    int _NV_getCurrentProductionQuantity();// public RVA = 0x296AD0 vtable offset = 0x558
    virtual bool canHaveSomeOfThese(GameData* input) override;// public RVA = 0x298410 vtable offset = 0x0
    bool _NV_canHaveSomeOfThese(GameData* input);// public RVA = 0x298410 vtable offset = 0x0
    virtual void getResourcesNeededBecauseEmpty(lektor<GameData*>& out);// public RVA = 0x299780 vtable offset = 0x560
    void _NV_getResourcesNeededBecauseEmpty(lektor<GameData*>& out);// public RVA = 0x299780 vtable offset = 0x560
    virtual void getResourcesNeededBecauseNotFull(lektor<GameData*>& out);// public RVA = 0x29B600 vtable offset = 0x568
    void _NV_getResourcesNeededBecauseNotFull(lektor<GameData*>& out);// public RVA = 0x29B600 vtable offset = 0x568
    virtual bool isAnyInputsEmpty() const override;// public RVA = 0x29B730 vtable offset = 0x0
    bool _NV_isAnyInputsEmpty() const;// public RVA = 0x29B730 vtable offset = 0x0
    virtual bool isAnyInputsFull();// public RVA = 0x29B8A0 vtable offset = 0x570
    bool _NV_isAnyInputsFull();// public RVA = 0x29B8A0 vtable offset = 0x570
    virtual bool isProductionFull();// public RVA = 0x296AF0 vtable offset = 0x578
    bool _NV_isProductionFull();// public RVA = 0x296AF0 vtable offset = 0x578
    virtual bool isProductionEmpty();// public RVA = 0x296B20 vtable offset = 0x580
    bool _NV_isProductionEmpty();// public RVA = 0x296B20 vtable offset = 0x580
    virtual int getNumConsumtionItems() const;// public RVA = 0x2AAF30 vtable offset = 0x588
    int _NV_getNumConsumtionItems() const;// public RVA = 0x2AAF30 vtable offset = 0x588
    virtual StorageBuilding::ConsumptionItem* getConsumtionItems(int id);// public RVA = 0x298770 vtable offset = 0x590
    StorageBuilding::ConsumptionItem* _NV_getConsumtionItems(int id);// public RVA = 0x298770 vtable offset = 0x590
    void getConsumtionItems_inStock(lektor<StorageBuilding::ConsumptionItem*>& out);// public RVA = 0x299920
    bool limitedByType() const;// public RVA = 0x3376C0
    itemType specialItemTypesOnly; // 0x440 Member
    static ogre_unordered_map<itemType, GameData*>::type itemTypeTemplates; // RVA = 0x1C82A30 Static Member
    virtual void updateInventoryWindow();// protected RVA = 0x29A230 vtable offset = 0x598
    void _NV_updateInventoryWindow();// protected RVA = 0x29A230 vtable offset = 0x598
    bool endOfTheLine; // 0x444 Member
    StorageBuilding::ConsumptionItem* productionItem; // 0x448 Member
    lektor<StorageBuilding::ConsumptionItem*> manyLimitItems; // 0x450 Member
    // no_addr class StorageBuilding & operator=(const class StorageBuilding & _a1);// public missing arg names
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};