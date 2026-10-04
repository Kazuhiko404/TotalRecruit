#pragma once

#include "Building.h"

class ZoneMapContent;

enum WallSectionLinkType
{
    WALLTYPE_NORMAL,
    WALLTYPE_CONNECTOR,
    WALLTYPE_LOWER_WEDGE,
    WALLTYPE_SINGLE,
    WALLTYPE_SHORT
};

class WallBuilding : public Building
{
public:
    // Building offset = 0x0, length = 0x360
    virtual ~WallBuilding();// public RVA = 0x591FF0 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x591FF0 vtable offset = 0x0
    // no_addr void WallBuilding(const class WallBuilding & _a1);// public missing arg names
    WallBuilding(GameData* dat, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, Faction* _participant, Layout* furnitureOf, const hand& town, const hand& _handle);// public RVA = 0x591DF0
    WallBuilding* _CONSTRUCTOR(GameData* dat, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, Faction* _participant, Layout* furnitureOf, const hand& town, const hand& _handle);// public RVA = 0x591DF0
    virtual UseableStuff* getUseableStuff() override;// public RVA = 0x591F80 vtable offset = 0x0
    UseableStuff* _NV_getUseableStuff();// public RVA = 0x591F80 vtable offset = 0x0
    virtual float getReachRange() const override;// public RVA = 0x591F90 vtable offset = 0x0
    float _NV_getReachRange() const;// public RVA = 0x591F90 vtable offset = 0x0
    virtual bool isForSale() override;// public RVA = 0x591FA0 vtable offset = 0x0
    bool _NV_isForSale();// public RVA = 0x591FA0 vtable offset = 0x0
    virtual bool createPhysical() override;// public RVA = 0x561AE0 vtable offset = 0x0
    bool _NV_createPhysical();// public RVA = 0x561AE0 vtable offset = 0x0
    virtual bool isDamaged() override;// public RVA = 0x591FB0 vtable offset = 0x0
    bool _NV_isDamaged();// public RVA = 0x591FB0 vtable offset = 0x0
    Ogre::Vector3 getPositionForWaypoint_outside();// public RVA = 0x296C00
    int getOutsideGateCode();// public RVA = 0x296CD0
    virtual WallBuilding* isAWall() override;// public RVA = 0x591FE0 vtable offset = 0x0
    WallBuilding* _NV_isAWall();// public RVA = 0x591FE0 vtable offset = 0x0
    virtual void upgrade(DataPanelLine* _a1) override;// public RVA = 0x2A5970 vtable offset = 0x0 missing arg names
    void _NV_upgrade(DataPanelLine* _a1);// public RVA = 0x2A5970 vtable offset = 0x0 missing arg names
    virtual void getGUIUpgrade(DatapanelGUI* datapanel, int category) override;// public RVA = 0x2FF2B0 vtable offset = 0x0
    void _NV_getGUIUpgrade(DatapanelGUI* datapanel, int category);// public RVA = 0x2FF2B0 vtable offset = 0x0
    virtual void getGUIDestroyButton(DatapanelGUI* datapanel, int category) override;// public RVA = 0x2FF9C0 vtable offset = 0x0
    void _NV_getGUIDestroyButton(DatapanelGUI* datapanel, int category);// public RVA = 0x2FF9C0 vtable offset = 0x0
    virtual GameData* canUpgrade() override;// public RVA = 0x2A4DC0 vtable offset = 0x0
    GameData* _NV_canUpgrade();// public RVA = 0x2A4DC0 vtable offset = 0x0
    virtual void dismantleButton_all(DataPanelLine* _a1);// public RVA = 0x29BCB0 vtable offset = 0x4A0 missing arg names
    void _NV_dismantleButton_all(DataPanelLine* _a1);// public RVA = 0x29BCB0 vtable offset = 0x4A0 missing arg names
    virtual void addConstructionProgress(float amount) override;// public RVA = 0x5597B0 vtable offset = 0x0
    void _NV_addConstructionProgress(float amount);// public RVA = 0x5597B0 vtable offset = 0x0
    virtual void setConstructionProgress(float amount) override;// public RVA = 0x559C10 vtable offset = 0x0
    void _NV_setConstructionProgress(float amount);// public RVA = 0x559C10 vtable offset = 0x0
    virtual void notifyConstructionComplete() override;// public RVA = 0x561D30 vtable offset = 0x0
    void _NV_notifyConstructionComplete();// public RVA = 0x561D30 vtable offset = 0x0
    virtual bool addDismantleProgress(float amount) override;// public RVA = 0x548B70 vtable offset = 0x0
    bool _NV_addDismantleProgress(float amount);// public RVA = 0x548B70 vtable offset = 0x0
    virtual void notifyConstructionDismantling() override;// public RVA = 0x549D70 vtable offset = 0x0
    void _NV_notifyConstructionDismantling();// public RVA = 0x549D70 vtable offset = 0x0
    virtual HitMaterialType hitByMeleeAttack(CutDirection dir, Damages& damage, Character* who, CombatTechniqueData* attack, int comboID) override;// public RVA = 0x29D0B0 vtable offset = 0x0
    HitMaterialType _NV_hitByMeleeAttack(CutDirection dir, Damages& damage, Character* who, CombatTechniqueData* attack, int comboID);// public RVA = 0x29D0B0 vtable offset = 0x0
    virtual Building::ConstructionState* getBuildState() override;// public RVA = 0x297B40 vtable offset = 0x0
    Building::ConstructionState* _NV_getBuildState();// public RVA = 0x297B40 vtable offset = 0x0
    void runLinkingCheck(ZoneMapContent* zone);// public RVA = 0x29A7A0
    bool isALittleWallPartLikeACornerOrSomething();// public RVA = 0x297B00
    bool isAShortWallPart();// public RVA = 0x297B20
    WallSectionLinkType wallSectionLinkType; // 0x360 Member
    bool letsShare(WallBuilding* who);// protected RVA = 0x29A3E0
    hand shareBuildStateOfAnother; // 0x368 Member
    lektor<hand> othersSharingMyBuildState; // 0x388 Member
    // no_addr class WallBuilding & operator=(const class WallBuilding & _a1);// public missing arg names
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};