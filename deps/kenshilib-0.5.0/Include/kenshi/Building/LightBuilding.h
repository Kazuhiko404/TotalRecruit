#pragma once

#include "UseableStuff.h"

class LightBuilding : public UseableStuff
{
public:
    // UseableStuff offset = 0x0, length = 0x440
    // no_addr void LightBuilding(const class LightBuilding & _a1);// public missing arg names
    LightBuilding(GameData* data, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, Faction* _participant, const hand& town, const hand& _handle, Layout* __isfurnitureOf, Building* _indoors);// public RVA = 0x562900
    LightBuilding* _CONSTRUCTOR(GameData* data, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, Faction* _participant, const hand& town, const hand& _handle, Layout* __isfurnitureOf, Building* _indoors);// public RVA = 0x562900
    virtual GameSaveState serialise(GameDataContainer* container, GameData* refList, PosRotPair* offsetPosToSubtract) override;// public RVA = 0x2A1530 vtable offset = 0x0
    GameSaveState _NV_serialise(GameDataContainer* container, GameData* refList, PosRotPair* offsetPosToSubtract);// public RVA = 0x2A1530 vtable offset = 0x0
    virtual void loadFromSerialise(GameSaveState* wholeState) override;// public RVA = 0x2A16D0 vtable offset = 0x0
    void _NV_loadFromSerialise(GameSaveState* wholeState);// public RVA = 0x2A16D0 vtable offset = 0x0
    virtual bool createPhysical() override;// public RVA = 0x297BA0 vtable offset = 0x0
    bool _NV_createPhysical();// public RVA = 0x297BA0 vtable offset = 0x0
    virtual CursorType getMouseCursor() override;// public RVA = 0x547040 vtable offset = 0x0
    CursorType _NV_getMouseCursor();// public RVA = 0x547040 vtable offset = 0x0
    virtual bool needPowerRightNow() const override;// public RVA = 0x2973E0 vtable offset = 0x0
    bool _NV_needPowerRightNow() const;// public RVA = 0x2973E0 vtable offset = 0x0
    virtual const hand& getMountedBuilding() const override;// public RVA = 0x57B6E0 vtable offset = 0x0
    const hand& _NV_getMountedBuilding() const;// public RVA = 0x57B6E0 vtable offset = 0x0
    void setMountedBuilding(Building* what);// public RVA = 0x297E60
    virtual Ogre::Vector3 getPositionMarker(const Ogre::Vector3& currentPosition) override;// public RVA = 0x54B420 vtable offset = 0x0
    Ogre::Vector3 _NV_getPositionMarker(const Ogre::Vector3& currentPosition);// public RVA = 0x54B420 vtable offset = 0x0
    hand mountedBuilding; // 0x440 Member
    virtual ~LightBuilding();// public RVA = 0x57B6F0 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x57B6F0 vtable offset = 0x0
    // no_addr class LightBuilding & operator=(const class LightBuilding & _a1);// public missing arg names
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};