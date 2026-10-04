#pragma once

#include "ProductionBuilding.h"

class TortureBuilding : public ProductionBuilding
{
public:
    // ProductionBuilding offset = 0x0, length = 0x490
    // no_addr void TortureBuilding(const class TortureBuilding & _a1);// public missing arg names
    TortureBuilding(GameData* data, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, Faction* participant, const hand& town, const hand& handle, Layout* isfurnitureOf, Building* indoors);// protected RVA = 0x589020
    TortureBuilding* _CONSTRUCTOR(GameData* data, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, Faction* participant, const hand& town, const hand& handle, Layout* isfurnitureOf, Building* indoors);// protected RVA = 0x589020
    virtual StorageBuilding* getFunctionStuff() override;// protected RVA = 0x5890A0 vtable offset = 0x0
    StorageBuilding* _NV_getFunctionStuff();// protected RVA = 0x5890A0 vtable offset = 0x0
    virtual void update() override;// protected RVA = 0x29AAE0 vtable offset = 0x0
    void _NV_update();// protected RVA = 0x29AAE0 vtable offset = 0x0
    float timer; // 0x490 Member
    virtual ~TortureBuilding();// public RVA = 0x5890B0 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x5890B0 vtable offset = 0x0
    // no_addr class TortureBuilding & operator=(const class TortureBuilding & _a1);// public missing arg names
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};