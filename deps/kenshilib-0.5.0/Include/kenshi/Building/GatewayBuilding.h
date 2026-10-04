#pragma once

#include "Building.h"

class GatewayBuilding : public Building
{
public:
    // Building offset = 0x0, length = 0x360
    virtual ~GatewayBuilding();// public RVA = 0x5629A0 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x5629A0 vtable offset = 0x0
    // no_addr void GatewayBuilding(const class GatewayBuilding & _a1);// public missing arg names
    GatewayBuilding(GameData* dat, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, Faction* _participant, const hand& town, const hand& _handle);// public RVA = 0x5629C0
    GatewayBuilding* _CONSTRUCTOR(GameData* dat, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, Faction* _participant, const hand& town, const hand& _handle);// public RVA = 0x5629C0
    void reAnnounceGateToPathfinder();// public RVA = 0x547050
    virtual void postCreationPathfinderSetupStuff() override;// public RVA = 0x555AC0 vtable offset = 0x0
    void _NV_postCreationPathfinderSetupStuff();// public RVA = 0x555AC0 vtable offset = 0x0
    virtual void setHandle(const hand& h) override;// public RVA = 0x547210 vtable offset = 0x0
    void _NV_setHandle(const hand& h);// public RVA = 0x547210 vtable offset = 0x0
    virtual Ogre::Vector3 getPositionForWaypoint(const Ogre::Vector3& from) override;// public RVA = 0x596210 vtable offset = 0x0
    Ogre::Vector3 _NV_getPositionForWaypoint(const Ogre::Vector3& from);// public RVA = 0x596210 vtable offset = 0x0
    virtual UseableStuff* getUseableStuff() override;// public RVA = 0x57B700 vtable offset = 0x0
    UseableStuff* _NV_getUseableStuff();// public RVA = 0x57B700 vtable offset = 0x0
    virtual GatewayBuilding* isGate() override;// public RVA = 0x57B710 vtable offset = 0x0
    GatewayBuilding* _NV_isGate();// public RVA = 0x57B710 vtable offset = 0x0
    virtual bool isForSale() override;// public RVA = 0x57B720 vtable offset = 0x0
    bool _NV_isForSale();// public RVA = 0x57B720 vtable offset = 0x0
    virtual void setup() override;// public RVA = 0x552A50 vtable offset = 0x0
    void _NV_setup();// public RVA = 0x552A50 vtable offset = 0x0
    int getGateCodeAt(const Ogre::Vector3& p);// public RVA = 0x547060
    int getOutsideGateCode();// public RVA = 0x296D00
    bool separatesAreas();// public RVA = 0x5471B0
    // no_addr class GatewayBuilding & operator=(const class GatewayBuilding & _a1);// public missing arg names
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};