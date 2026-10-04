#pragma once

#include "UseableStuff.h"

class GunClassTurret;

class TurretBuilding : public UseableStuff
{
public:
    // UseableStuff offset = 0x0, length = 0x440
    // no_addr void TurretBuilding(const class TurretBuilding & _a1);// public missing arg names
    TurretBuilding(GameData* _data, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, Faction* _participant, const hand& town, const hand& _handle, Layout* __isfurnitureOf, Building* _indoors);// public RVA = 0x43C630
    TurretBuilding* _CONSTRUCTOR(GameData* _data, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, Faction* _participant, const hand& town, const hand& _handle, Layout* __isfurnitureOf, Building* _indoors);// public RVA = 0x43C630
    virtual ~TurretBuilding();// public RVA = 0x434390 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x434390 vtable offset = 0x0
    virtual TaskType getDefaultTask() override;// public RVA = 0x43FDD0 vtable offset = 0x0
    TaskType _NV_getDefaultTask();// public RVA = 0x43FDD0 vtable offset = 0x0
    virtual void setup() override;// public RVA = 0x556970 vtable offset = 0x0
    void _NV_setup();// public RVA = 0x556970 vtable offset = 0x0
    virtual void update() override;// public RVA = 0x4343F0 vtable offset = 0x0
    void _NV_update();// public RVA = 0x4343F0 vtable offset = 0x0
    virtual bool needsUpdate() override;// public RVA = 0x549160 vtable offset = 0x0
    bool _NV_needsUpdate();// public RVA = 0x549160 vtable offset = 0x0
    virtual void operate(Character* who, float _a2) override;// public RVA = 0x4345A0 vtable offset = 0x0
    void _NV_operate(Character* who, float _a2);// public RVA = 0x4345A0 vtable offset = 0x0
    StatsEnumerated getStatUsed() const;// public RVA = 0x33CE00
    virtual float getProductionMultForGUI() override;// public RVA = 0x297700 vtable offset = 0x0
    float _NV_getProductionMultForGUI();// public RVA = 0x297700 vtable offset = 0x0
    virtual void getGUIToolTipForGroundResourceEfficiency(lektor<StringPair>& out) override;// public RVA = 0x30C690 vtable offset = 0x0
    void _NV_getGUIToolTipForGroundResourceEfficiency(lektor<StringPair>& out);// public RVA = 0x30C690 vtable offset = 0x0
    void aimAt(const Ogre::Vector3& who);// public RVA = 0x43FAE0
    virtual void setVisible(bool on) override;// public RVA = 0x434720 vtable offset = 0x0
    void _NV_setVisible(bool on);// public RVA = 0x434720 vtable offset = 0x0
    void setMountedBuilding(Building* what);// public RVA = 0x297D70
    virtual const hand& getMountedBuilding() const override;// public RVA = 0x43FDE0 vtable offset = 0x0
    const hand& _NV_getMountedBuilding() const;// public RVA = 0x43FDE0 vtable offset = 0x0
    virtual int amInsideTownWalls() override;// public RVA = 0x297A80 vtable offset = 0x0
    int _NV_amInsideTownWalls();// public RVA = 0x297A80 vtable offset = 0x0
    virtual void notifyConstructionComplete() override;// public RVA = 0x4363B0 vtable offset = 0x0
    void _NV_notifyConstructionComplete();// public RVA = 0x4363B0 vtable offset = 0x0
    virtual bool createPhysical() override;// public RVA = 0x43AF10 vtable offset = 0x0
    bool _NV_createPhysical();// public RVA = 0x43AF10 vtable offset = 0x0
    virtual void destroyPhysical() override;// public RVA = 0x434670 vtable offset = 0x0
    void _NV_destroyPhysical();// public RVA = 0x434670 vtable offset = 0x0
    virtual void onBuildingLoaded() override;// public RVA = 0x4362D0 vtable offset = 0x0
    void _NV_onBuildingLoaded();// public RVA = 0x4362D0 vtable offset = 0x0
    void addToBuildingsMananger(Ogre::MovableObject*& obj);// public RVA = 0x546D10
    void removeFromBuildingsMananger(Ogre::MovableObject*& obj);// public RVA = 0x546D50
    virtual void getGUIState(DatapanelGUI* datapanel, int category) override;// public RVA = 0x2FDF80 vtable offset = 0x0
    void _NV_getGUIState(DatapanelGUI* datapanel, int category);// public RVA = 0x2FDF80 vtable offset = 0x0
    virtual void getGUIData(DatapanelGUI* datapanel, int category) override;// public RVA = 0x30BDF0 vtable offset = 0x0
    void _NV_getGUIData(DatapanelGUI* datapanel, int category);// public RVA = 0x30BDF0 vtable offset = 0x0
    virtual GameSaveState serialise(GameDataContainer* container, GameData* refList, PosRotPair* offsetPosToSubtract) override;// public RVA = 0x2A17D0 vtable offset = 0x0
    GameSaveState _NV_serialise(GameDataContainer* container, GameData* refList, PosRotPair* offsetPosToSubtract);// public RVA = 0x2A17D0 vtable offset = 0x0
    virtual void loadFromSerialise(GameSaveState* wholeState) override;// public RVA = 0x2A1980 vtable offset = 0x0
    void _NV_loadFromSerialise(GameSaveState* wholeState);// public RVA = 0x2A1980 vtable offset = 0x0
    float calculatePowerMult();// public RVA = 0x2FD990
    void _teleport(const Ogre::Vector3& p);// public RVA = 0x297020
    virtual void clearTownBuildingsManagerPtr() override;// public RVA = 0x54AD20 vtable offset = 0x0
    void _NV_clearTownBuildingsManagerPtr();// public RVA = 0x54AD20 vtable offset = 0x0
    GunClassTurret* gunClass; // 0x440 Member
    float aimDistInDegrees; // 0x448 Member
    Ogre::Vector3 currentAimDir; // 0x44C Member
    Ogre::Vector3 rootAimDir; // 0x458 Member
    float currentAimSpeed; // 0x464 Member
    Ogre::Vector3 currentAimTarget; // 0x468 Member
    virtual void setPartVisible(PhysicalEntity* part, bool on) override;// private RVA = 0x434770 vtable offset = 0x0
    void _NV_setPartVisible(PhysicalEntity* part, bool on);// private RVA = 0x434770 vtable offset = 0x0
    hand mountedBuilding; // 0x478 Member
    PhysicsCollection::RotatingEnt* hingePart; // 0x498 Member
    PhysicsCollection::RotatingEnt* gunPart; // 0x4A0 Member
    Ogre::Vector3 aimTargetPos; // 0x4A8 Member
    int rotating; // 0x4B4 Member
    // no_addr class TurretBuilding & operator=(const class TurretBuilding & _a1);// public missing arg names
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};