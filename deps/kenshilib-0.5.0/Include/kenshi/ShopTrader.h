#pragma once

#include "RootObject.h"

class ShopTrader : public RootObject, public Ogre::GeneralAllocatedObject
{
public:
    // RootObject offset = 0x0, length = 0xC0
    // Ogre::AllocatedObject<Ogre::CategorisedAllocPolicy<0> > offset = 0xC0, length = 0x1
    // no_addr void ShopTrader(const class ShopTrader & _a1);// public missing arg names
    ShopTrader(Character* trader);// public RVA = 0x9520A0
    ShopTrader* _CONSTRUCTOR(Character* trader);// public RVA = 0x9520A0
    virtual ~ShopTrader();// public RVA = 0x950DA0 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x950DA0 vtable offset = 0x0
    virtual void getGUIData(DatapanelGUI* datapanel, int category) override;// public RVA = 0x953050 vtable offset = 0x0
    void _NV_getGUIData(DatapanelGUI* datapanel, int category);// public RVA = 0x953050 vtable offset = 0x0
    virtual void equipItem(const std::string& _a1, Item* _a2) override;// public RVA = 0x95C930 vtable offset = 0x0 missing arg names
    void _NV_equipItem(const std::string& _a1, Item* _a2);// public RVA = 0x95C930 vtable offset = 0x0 missing arg names
    virtual void unequipItem(const std::string& _a1, Item* _a2) override;// public RVA = 0x95C940 vtable offset = 0x0 missing arg names
    void _NV_unequipItem(const std::string& _a1, Item* _a2);// public RVA = 0x95C940 vtable offset = 0x0 missing arg names
    virtual Inventory* getInventory() const override;// public RVA = 0x950E10 vtable offset = 0x0
    Inventory* _NV_getInventory() const;// public RVA = 0x950E10 vtable offset = 0x0
    virtual bool takeMoney(int money) override;// public RVA = 0x950F10 vtable offset = 0x0
    bool _NV_takeMoney(int money);// public RVA = 0x950F10 vtable offset = 0x0
    virtual int getMoney() const override;// public RVA = 0x950F30 vtable offset = 0x0
    int _NV_getMoney() const;// public RVA = 0x950F30 vtable offset = 0x0
    virtual Ogre::Quaternion getOrientation() const override;// public RVA = 0x950E20 vtable offset = 0x0
    Ogre::Quaternion _NV_getOrientation() const;// public RVA = 0x950E20 vtable offset = 0x0
    virtual Ogre::Vector3 getPosition() override;// public RVA = 0x950E50 vtable offset = 0x0
    Ogre::Vector3 _NV_getPosition();// public RVA = 0x950E50 vtable offset = 0x0
    virtual const hand& isIndoors() const override;// public RVA = 0x950E80 vtable offset = 0x0
    const hand& _NV_isIndoors() const;// public RVA = 0x950E80 vtable offset = 0x0
    virtual int getFloor() const override;// public RVA = 0x950EA0 vtable offset = 0x0
    int _NV_getFloor() const;// public RVA = 0x950EA0 vtable offset = 0x0
    virtual TownBase* getCurrentTownLocation() override;// public RVA = 0x950EC0 vtable offset = 0x0
    TownBase* _NV_getCurrentTownLocation();// public RVA = 0x950EC0 vtable offset = 0x0
    virtual itemType getDataType() const override;// public RVA = 0x95C950 vtable offset = 0x0
    itemType _NV_getDataType() const;// public RVA = 0x95C950 vtable offset = 0x0
    virtual GameSaveState serialise(GameDataContainer* container, GameData* refList, PosRotPair* offsetPosToSubtract) override;// public RVA = 0x951950 vtable offset = 0x0
    GameSaveState _NV_serialise(GameDataContainer* container, GameData* refList, PosRotPair* offsetPosToSubtract);// public RVA = 0x951950 vtable offset = 0x0
    virtual void loadFromSerialise(GameSaveState* state) override;// public RVA = 0x950EE0 vtable offset = 0x0
    void _NV_loadFromSerialise(GameSaveState* state);// public RVA = 0x950EE0 vtable offset = 0x0
    Character* getTrader() const;// public RVA = 0x70B9D0
    virtual bool isPhysical() const override;// private RVA = 0x95C960 vtable offset = 0x0
    bool _NV_isPhysical() const;// private RVA = 0x95C960 vtable offset = 0x0
    virtual void setVisible(bool on) override;// private RVA = 0x95C970 vtable offset = 0x0
    void _NV_setVisible(bool on);// private RVA = 0x95C970 vtable offset = 0x0
    virtual bool createPhysical() override;// private RVA = 0x95C980 vtable offset = 0x0
    bool _NV_createPhysical();// private RVA = 0x95C980 vtable offset = 0x0
    virtual void destroyPhysical() override;// private RVA = 0x95C990 vtable offset = 0x0
    void _NV_destroyPhysical();// private RVA = 0x95C990 vtable offset = 0x0
    void updateInventory();// private RVA = 0x950EF0
    Character* trader; // 0xC0 Member
    Inventory* inventory; // 0xC8 Member
    // no_addr class ShopTrader & operator=(const class ShopTrader & _a1);// public missing arg names
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};