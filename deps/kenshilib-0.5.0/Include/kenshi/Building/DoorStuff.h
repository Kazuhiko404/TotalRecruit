#pragma once

#include "Building.h"

class Faction;
class DatapanelGUI;
class Damages;
class CombatTechniqueData;

enum DoorState
{
    DOORSTATE_CLOSED,
    DOORSTATE_OPEN,
    DOORSTATE_OPENING,
    DOORSTATE_CLOSING
};

class DoorStuff : public Building
{
public:
    // Building offset = 0x0, length = 0x360
    enum DoorStateInitial
    {
        CLOSED,
        OPEN,
        LOCKED,
        BROKEN
    };

    // no_addr void DoorStuff(const class DoorStuff & _a1);// public missing arg names
    DoorStuff(GameData* dat, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, Faction* _participant, const hand& town, const hand& _handle, Layout* __isfurnitureOf, Building* _indoors, Building* par);// public RVA = 0x29F690
    DoorStuff* _CONSTRUCTOR(GameData* dat, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, Faction* _participant, const hand& town, const hand& _handle, Layout* __isfurnitureOf, Building* _indoors, Building* par);// public RVA = 0x29F690
    virtual ~DoorStuff();// public RVA = 0x2AD6D0 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x2AD6D0 vtable offset = 0x0
    bool setupPhysicalUT();// public RVA = 0x29C7E0
    virtual Faction* getFaction() const override;// public RVA = 0x2AD6F0 vtable offset = 0x0
    Faction* _NV_getFaction() const;// public RVA = 0x2AD6F0 vtable offset = 0x0
    NxActor* _getPhysActorUT();// public RVA = 0x298830
    virtual void getGUIData(DatapanelGUI* datapanel, int category) override;// public RVA = 0x303C70 vtable offset = 0x0
    void _NV_getGUIData(DatapanelGUI* datapanel, int category);// public RVA = 0x303C70 vtable offset = 0x0
    virtual bool needsUpdate() override;// public RVA = 0x9F7D80 vtable offset = 0x0
    bool _NV_needsUpdate();// public RVA = 0x9F7D80 vtable offset = 0x0
    unsigned short getGateCode();// public RVA = 0x2E9D50
    void setGateCode(int code);// public RVA = 0x2E9D60
    virtual GatewayBuilding* isGate() override;// public RVA = 0x2AD710 vtable offset = 0x0
    GatewayBuilding* _NV_isGate();// public RVA = 0x2AD710 vtable offset = 0x0
    virtual bool isPublic() const override;// public RVA = 0x2AD730 vtable offset = 0x0
    bool _NV_isPublic() const;// public RVA = 0x2AD730 vtable offset = 0x0
    virtual CursorType getMouseCursor() override;// public RVA = 0x547320 vtable offset = 0x0
    CursorType _NV_getMouseCursor();// public RVA = 0x547320 vtable offset = 0x0
    virtual TaskType getDefaultTask() override;// public RVA = 0x2AD750 vtable offset = 0x0
    TaskType _NV_getDefaultTask();// public RVA = 0x2AD750 vtable offset = 0x0
    virtual void setHandle(const hand& h) override;// public RVA = 0x556A60 vtable offset = 0x0
    void _NV_setHandle(const hand& h);// public RVA = 0x556A60 vtable offset = 0x0
    virtual float getReachRange() const override;// public RVA = 0x2AD770 vtable offset = 0x0
    float _NV_getReachRange() const;// public RVA = 0x2AD770 vtable offset = 0x0
    virtual DoorStuff* getDoor() override;// public RVA = 0x2AD7B0 vtable offset = 0x0
    DoorStuff* _NV_getDoor();// public RVA = 0x2AD7B0 vtable offset = 0x0
    virtual std::string getName() const override;// public RVA = 0x2AD880 vtable offset = 0x0
    std::string _NV_getName() const;// public RVA = 0x2AD880 vtable offset = 0x0
    virtual DoorLock* getDoorLock() override;// public RVA = 0x2AD7C0 vtable offset = 0x0
    DoorLock* _NV_getDoorLock();// public RVA = 0x2AD7C0 vtable offset = 0x0
    virtual bool hasDoorLock() const override;// public RVA = 0x2AD7D0 vtable offset = 0x0
    bool _NV_hasDoorLock() const;// public RVA = 0x2AD7D0 vtable offset = 0x0
    virtual HitMaterialType hitByMeleeAttack(CutDirection dir, Damages& damage, Character* who, CombatTechniqueData* attack, int comboID) override;// public RVA = 0x29D1E0 vtable offset = 0x0
    HitMaterialType _NV_hitByMeleeAttack(CutDirection dir, Damages& damage, Character* who, CombatTechniqueData* attack, int comboID);// public RVA = 0x29D1E0 vtable offset = 0x0
    const Ogre::Vector3& getActorClosedPosition() const;// public RVA = 0x3D2150
    void updateGateCodeState() const;// public RVA = 0x296DE0
    virtual void onBuildingLoaded() override;// public RVA = 0x296D90 vtable offset = 0x0
    void _NV_onBuildingLoaded();// public RVA = 0x296D90 vtable offset = 0x0
    bool isSetupComplete; // 0x360 Member
    Building* parent; // 0x368 Member
    DoorLock* doorLock; // 0x370 Member
    int gateCode; // 0x378 Member
    float doorOpenAmount; // 0x37C Member
    DoorState state; // 0x380 Member
    bool wantsToLock; // 0x384 Member
    float doorSpeed; // 0x388 Member
    float doorDistance; // 0x38C Member
    int doorMoveAxis; // 0x390 Member
    float doorComesOut; // 0x394 Member
    const Ogre::Vector3 nodeClosedPosition; // 0x398 Member
    Ogre::Vector3 actorClosedPosition; // 0x3A4 Member
    Ogre::Vector3 doorClosedPosition; // 0x3B0 Member
    Ogre::Vector3 doorClosedPositionOut; // 0x3BC Member
    Ogre::Vector3 doorClosedPositionIn; // 0x3C8 Member
    Ogre::Vector3 axisH; // 0x3D4 Member
    Ogre::Vector3 axisF; // 0x3E0 Member
    void setDoorOpenAmount(float amount, bool forceUnsafe);// protected RVA = 0x298860
    bool _isBroken; // 0x3EC Member
    virtual bool isBroken() const override;// public RVA = 0x2AD7E0 vtable offset = 0x0
    bool _NV_isBroken() const;// public RVA = 0x2AD7E0 vtable offset = 0x0
    virtual void setBroken(bool on) override;// public RVA = 0x2AD7F0 vtable offset = 0x0
    void _NV_setBroken(bool on);// public RVA = 0x2AD7F0 vtable offset = 0x0
    virtual bool isDisabled() override;// public RVA = 0x2AD800 vtable offset = 0x0
    bool _NV_isDisabled();// public RVA = 0x2AD800 vtable offset = 0x0
    bool isLocked() const;// public RVA = 0x2E9D70
    void lockDoor();// public RVA = 0x296540
    void unlockDoor();// public RVA = 0x569C50
    float getDoorOpenAmount() const;// public RVA = 0x595D90
    void setDoorState(DoorState what);// public RVA = 0x298B50
    DoorState getDoorState() const;// public RVA = 0x337650
    bool openDoor();// public RVA = 0x296B90
    bool closeDoor();// public RVA = 0x2987A0
    bool _forceDoorClosedUT();// public RVA = 0x298D10
    bool _forceDoorOpenUT();// public RVA = 0x298D50
    virtual void update() override;// public RVA = 0x298B80 vtable offset = 0x0
    void _NV_update();// public RVA = 0x298B80 vtable offset = 0x0
    const Ogre::Vector3& getDoorPosition() const;// public RVA = 0x3323A0
    const Ogre::Vector3& getDoorPosOutside() const;// public RVA = 0x296580
    Ogre::Vector3 getDoorPosOutside_extraFarOut(float mult) const;// public RVA = 0x296590
    const Ogre::Vector3& getDoorPosInside() const;// public RVA = 0x3381C0
    Ogre::Vector3 getDoorPosInside_extraFarIn(float mult) const;// public RVA = 0x2E9DC0
    bool isOpen() const;// public RVA = 0x2E9E40
    void openButton(DataPanelLine* _a1);// public RVA = 0x546830 missing arg names
    void lockButton(DataPanelLine* _a1);// public RVA = 0x5468E0 missing arg names
    virtual void _serialise(GameData* save, const std::string& id);// public RVA = 0x29FE70 vtable offset = 0x4A0
    void _NV__serialise(GameData* save, const std::string& id);// public RVA = 0x29FE70 vtable offset = 0x4A0
    virtual void _loadFromSerialise(GameData* save, const std::string& id, bool firstTime);// public RVA = 0x2A01F0 vtable offset = 0x4A8
    void _NV__loadFromSerialise(GameData* save, const std::string& id, bool firstTime);// public RVA = 0x2A01F0 vtable offset = 0x4A8
    virtual DoorStuff* doorStuff() override;// public RVA = 0x2AD810 vtable offset = 0x0
    DoorStuff* _NV_doorStuff();// public RVA = 0x2AD810 vtable offset = 0x0
    virtual Building* doorParentBuilding() const override;// public RVA = 0x2AD820 vtable offset = 0x0
    Building* _NV_doorParentBuilding() const;// public RVA = 0x2AD820 vtable offset = 0x0
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};