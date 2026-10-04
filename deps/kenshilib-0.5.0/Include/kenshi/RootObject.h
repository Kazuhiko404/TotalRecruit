#pragma once

#include <kenshi/RootObjectBase.h>
#include <string>
#include <ogre/OgreQuaternion.h>
#include <kenshi/util/StringPair.h>

class InstanceID;
class ZoneMap;
class RaceData;
class DatapanelGUI;
class InventoryLayout;
class Damages;
class CombatTechniqueData;
class PlatoonAI;
class RootObjectContainer;
class Inventory;

class RootObject : public RootObjectBase
{
public:
    // RootObjectBase offset = 0x0, length = 0x78
    // no_addr void RootObject(const class RootObject & _a1);// public missing arg names
    RootObject(GameData* d, Faction* ownr, hand _h);// public RVA = 0x594080
    RootObject* _CONSTRUCTOR(GameData* d, Faction* ownr, hand _h);// public RVA = 0x594080
    virtual ~RootObject();// public RVA = 0x5941C0 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x5941C0 vtable offset = 0x0
    virtual bool needsSaving(const std::string& s) const;// public RVA = 0xD1E90 vtable offset = 0xA8
    bool _NV_needsSaving(const std::string& s) const;// public RVA = 0xD1E90 vtable offset = 0xA8
    virtual InstanceID* getInstanceID();// public RVA = 0xD1EA0 vtable offset = 0xB0
    InstanceID* _NV_getInstanceID();// public RVA = 0xD1EA0 vtable offset = 0xB0
    virtual const std::string& getLayoutInstanceID();// public RVA = 0xD1EB0 vtable offset = 0xB8
    const std::string& _NV_getLayoutInstanceID();// public RVA = 0xD1EB0 vtable offset = 0xB8
    virtual Ogre::Quaternion getOrientation() const;// public RVA = 0xD1EC0 vtable offset = 0xC0
    Ogre::Quaternion _NV_getOrientation() const;// public RVA = 0xD1EC0 vtable offset = 0xC0
    virtual ZoneMap* getZoneMapLocation();// public RVA = 0x593B90 vtable offset = 0xC8
    ZoneMap* _NV_getZoneMapLocation();// public RVA = 0x593B90 vtable offset = 0xC8
    virtual RaceData* getRace() const;// public RVA = 0xD1EF0 vtable offset = 0xD0
    RaceData* _NV_getRace() const;// public RVA = 0xD1EF0 vtable offset = 0xD0
    // no_addr class Ogre::Matrix3 getMatrix();// public
    virtual void threadedUpdate();// public RVA = 0xD1F00 vtable offset = 0xD8
    void _NV_threadedUpdate();// public RVA = 0xD1F00 vtable offset = 0xD8
    virtual void update();// public RVA = 0x593B00 vtable offset = 0xE0
    void _NV_update();// public RVA = 0x593B00 vtable offset = 0xE0
    virtual void periodicUpdate();// public RVA = 0x593B70 vtable offset = 0xE8
    void _NV_periodicUpdate();// public RVA = 0x593B70 vtable offset = 0xE8
    virtual const Ogre::Aabb& getAABB() const;// public RVA = 0xD1F10 vtable offset = 0xF0
    const Ogre::Aabb& _NV_getAABB() const;// public RVA = 0xD1F10 vtable offset = 0xF0
    virtual bool isPhysical() const = 0;// public vtable offset = 0xF8
    virtual void setVisible(bool _a1) = 0;// public vtable offset = 0x100 missing arg names
    virtual bool getVisible() const;// public RVA = 0xD5290 vtable offset = 0x108
    bool _NV_getVisible() const;// public RVA = 0xD5290 vtable offset = 0x108
    virtual bool isDisabled();// public RVA = 0xD1F20 vtable offset = 0x110
    bool _NV_isDisabled();// public RVA = 0xD1F20 vtable offset = 0x110
    virtual void setFaction(Faction* p, ActivePlatoon* a) override;// public RVA = 0x593C60 vtable offset = 0x0
    void _NV_setFaction(Faction* p, ActivePlatoon* a);// public RVA = 0x593C60 vtable offset = 0x0
    virtual int amInsideTownWalls();// public RVA = 0x594030 vtable offset = 0x118
    int _NV_amInsideTownWalls();// public RVA = 0x594030 vtable offset = 0x118
    virtual void setInsideTownWalls(int s);// public RVA = 0xD1F30 vtable offset = 0x120
    void _NV_setInsideTownWalls(int s);// public RVA = 0xD1F30 vtable offset = 0x120
    virtual int getFloor() const override;// public RVA = 0xD1F40 vtable offset = 0x0
    int _NV_getFloor() const;// public RVA = 0xD1F40 vtable offset = 0x0
    virtual void setFloor(int f);// public RVA = 0xD1F50 vtable offset = 0x128
    void _NV_setFloor(int f);// public RVA = 0xD1F50 vtable offset = 0x128
    virtual void select();// public RVA = 0xD1F60 vtable offset = 0x130
    void _NV_select();// public RVA = 0xD1F60 vtable offset = 0x130
    virtual void unselect();// public RVA = 0xD1F70 vtable offset = 0x138
    void _NV_unselect();// public RVA = 0xD1F70 vtable offset = 0x138
    virtual void getGUIData(DatapanelGUI* _a1, int category);// public RVA = 0xD52A0 vtable offset = 0x140
    void _NV_getGUIData(DatapanelGUI* _a1, int category);// public RVA = 0xD52A0 vtable offset = 0x140
    virtual void getGUIDataCategories(lektor<StringPair>& out);// public RVA = 0xD1F80 vtable offset = 0x148
    void _NV_getGUIDataCategories(lektor<StringPair>& out);// public RVA = 0xD1F80 vtable offset = 0x148
    virtual void setStandingOrder(MessageForB::StandingOrder _a1);// public RVA = 0xD1F90 vtable offset = 0x150 missing arg names
    void _NV_setStandingOrder(MessageForB::StandingOrder _a1);// public RVA = 0xD1F90 vtable offset = 0x150 missing arg names
    virtual void getOrders(lektor<std::string >& _a1);// public RVA = 0xD1FA0 vtable offset = 0x158 missing arg names
    void _NV_getOrders(lektor<std::string >& _a1);// public RVA = 0xD1FA0 vtable offset = 0x158 missing arg names
    virtual Inventory* getInventory() const;// public RVA = 0x38F960 vtable offset = 0x160
    Inventory* _NV_getInventory() const;// public RVA = 0x38F960 vtable offset = 0x160
    virtual bool giveItem(Item* item, bool dropOnFail, bool destroyOnFail);// public RVA = 0x5939C0 vtable offset = 0x168
    bool _NV_giveItem(Item* item, bool dropOnFail, bool destroyOnFail);// public RVA = 0x5939C0 vtable offset = 0x168
    virtual bool hasRoomForItem(GameData* item);// public RVA = 0x593A40 vtable offset = 0x170
    bool _NV_hasRoomForItem(GameData* item);// public RVA = 0x593A40 vtable offset = 0x170
    virtual bool hasItem(GameData* item);// public RVA = 0x593AA0 vtable offset = 0x178
    bool _NV_hasItem(GameData* item);// public RVA = 0x593AA0 vtable offset = 0x178
    virtual InventoryLayout* createInventoryLayout();// public RVA = 0xD1FB0 vtable offset = 0x180
    InventoryLayout* _NV_createInventoryLayout();// public RVA = 0xD1FB0 vtable offset = 0x180
    virtual bool ImStealingDoYouNotice(RootObject* thief);// public RVA = 0xD1FC0 vtable offset = 0x188
    bool _NV_ImStealingDoYouNotice(RootObject* thief);// public RVA = 0xD1FC0 vtable offset = 0x188
    virtual bool stolenGoodsDetectionCheck(Item* item, RootObject* thief);// public RVA = 0xD1FD0 vtable offset = 0x190
    bool _NV_stolenGoodsDetectionCheck(Item* item, RootObject* thief);// public RVA = 0xD1FD0 vtable offset = 0x190
    virtual void equipItem(const std::string& _a1, Item* _a2);// public RVA = 0xD1FE0 vtable offset = 0x198 missing arg names
    void _NV_equipItem(const std::string& _a1, Item* _a2);// public RVA = 0xD1FE0 vtable offset = 0x198 missing arg names
    virtual void unequipItem(const std::string& _a1, Item* _a2);// public RVA = 0xD1FF0 vtable offset = 0x1A0 missing arg names
    void _NV_unequipItem(const std::string& _a1, Item* _a2);// public RVA = 0xD1FF0 vtable offset = 0x1A0 missing arg names
    virtual void dropItem(RootObject* _a1);// public RVA = 0xD2000 vtable offset = 0x1A8 missing arg names
    void _NV_dropItem(RootObject* _a1);// public RVA = 0xD2000 vtable offset = 0x1A8 missing arg names
    virtual bool takeMoney(int _a1);// public RVA = 0xD2010 vtable offset = 0x1B0 missing arg names
    bool _NV_takeMoney(int _a1);// public RVA = 0xD2010 vtable offset = 0x1B0 missing arg names
    virtual int getMoney() const;// public RVA = 0xD2020 vtable offset = 0x1B8
    int _NV_getMoney() const;// public RVA = 0xD2020 vtable offset = 0x1B8
    virtual void say(const std::string& _a1);// public RVA = 0xD2030 vtable offset = 0x1C0 missing arg names
    void _NV_say(const std::string& _a1);// public RVA = 0xD2030 vtable offset = 0x1C0 missing arg names
    virtual void notifyIndoors(const hand& in);// public RVA = 0x593B10 vtable offset = 0x1C8
    void _NV_notifyIndoors(const hand& in);// public RVA = 0x593B10 vtable offset = 0x1C8
    virtual void notifyOutdoors();// public RVA = 0x593B50 vtable offset = 0x1D0
    void _NV_notifyOutdoors();// public RVA = 0x593B50 vtable offset = 0x1D0
    virtual const hand& isIndoors() const;// public RVA = 0xD2040 vtable offset = 0x1D8
    const hand& _NV_isIndoors() const;// public RVA = 0xD2040 vtable offset = 0x1D8
    virtual bool isOnARoof();// public RVA = 0xD2050 vtable offset = 0x1E0
    bool _NV_isOnARoof();// public RVA = 0xD2050 vtable offset = 0x1E0
    virtual float getIntendedAggression();// public RVA = 0xD2060 vtable offset = 0x1E8
    float _NV_getIntendedAggression();// public RVA = 0xD2060 vtable offset = 0x1E8
    virtual HitMaterialType hitByMeleeAttack(CutDirection dir, Damages& damage, Character* who, CombatTechniqueData* attack, int comboID);// public RVA = 0xD2070 vtable offset = 0x1F0
    HitMaterialType _NV_hitByMeleeAttack(CutDirection dir, Damages& damage, Character* who, CombatTechniqueData* attack, int comboID);// public RVA = 0xD2070 vtable offset = 0x1F0
    virtual PlatoonAI* getPlatoonAI();// public RVA = 0xD2080 vtable offset = 0x1F8
    PlatoonAI* _NV_getPlatoonAI();// public RVA = 0xD2080 vtable offset = 0x1F8
    virtual bool createPhysical() = 0;// public vtable offset = 0x200
    virtual void destroyPhysical() = 0;// public vtable offset = 0x208
    virtual void setIsInsideBuilding(const hand& h);// public RVA = 0xD2090 vtable offset = 0x210
    void _NV_setIsInsideBuilding(const hand& h);// public RVA = 0xD2090 vtable offset = 0x210
    virtual void notifyEffect(EffectType::Enum type, WeatherAffecting what, float strength);// public RVA = 0xD20D0 vtable offset = 0x218
    void _NV_notifyEffect(EffectType::Enum type, WeatherAffecting what, float strength);// public RVA = 0xD20D0 vtable offset = 0x218
    RootObjectContainer* container; // 0x78 Member
    hand isInsideBuilding; // 0x80 Member
    int isInsideTownWalls; // 0xA0 Member
    int floorNum; // 0xA4 Member
    unsigned int spacialKey; // 0xA8 Member
    int outdoorDelayNotification_timer; // 0xAC Member
    Ogre::Quaternion rot; // 0xB0 Member
    virtual void loadUnloadCheck();// protected RVA = 0x593BD0 vtable offset = 0x220
    void _NV_loadUnloadCheck();// protected RVA = 0x593BD0 vtable offset = 0x220
    // no_addr class RootObject & operator=(const class RootObject & _a1);// public missing arg names
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};

class Serialisable;
class RootObjectContainer;

class DataObjectContainer
{
public:
    // VTable         : (none)
    enum GroupType
    {
        TYPE_UNKNOWN,
        TYPE_PLATOON,
        TYPE_FACTION,
        TYPE_TOWNLIST,
        TYPE_ZONEMAP,
        TYPE_BUILDING_INTERIOR
    };

    // no_addr void DataObjectContainer(const class DataObjectContainer & _a1);// public missing arg names
    DataObjectContainer(RootObjectContainer* from);// public RVA = 0x7EB070
    DataObjectContainer* _CONSTRUCTOR(RootObjectContainer* from);// public RVA = 0x7EB070
    DataObjectContainer(DataObjectContainer::GroupType type);// public RVA = 0x575C50
    DataObjectContainer* _CONSTRUCTOR(DataObjectContainer::GroupType type);// public RVA = 0x575C50
    virtual ~DataObjectContainer();// public RVA = 0x502B10 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x502B10 vtable offset = 0x0
    DataObjectContainer::GroupType getType() const;// public RVA = 0x588E20
    void setupDataFile(const std::string& filename, GameDataContainer* container);// public RVA = 0x5062A0
    // no_addr void serialiseToDisk_selfContained();// public
    void destroyObjectDatas();// public RVA = 0x4FE0B0
    bool isStored; // 0x8 Member
    GameDataContainer* objectDatas; // 0x10 Member
    std::string datasFile; // 0x18 Member
    virtual bool loadFromDisk(bool force, Serialisable* extraData);// protected RVA = 0x36B0B0 vtable offset = 0x8
    bool _NV_loadFromDisk(bool force, Serialisable* extraData);// protected RVA = 0x36B0B0 vtable offset = 0x8
    DataObjectContainer::GroupType selfType; // 0x40 Member
    // no_addr class DataObjectContainer & operator=(const class DataObjectContainer & _a1);// public missing arg names
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};

class FactoryCallbackInterface;

class RootObjectContainer : public DataObjectContainer, public Ogre::GeneralAllocatedObject
{
public:
    // DataObjectContainer offset = 0x0, length = 0x48
    // Ogre::AllocatedObject<Ogre::CategorisedAllocPolicy<0> > offset = 0x49, length = 0x1
    // no_addr void RootObjectContainer(const class RootObjectContainer & _a1);// public missing arg names
    RootObjectContainer(DataObjectContainer* from);// public RVA = 0x7EB120
    RootObjectContainer* _CONSTRUCTOR(DataObjectContainer* from);// public RVA = 0x7EB120
    RootObjectContainer(DataObjectContainer::GroupType type);// public RVA = 0x575CC0
    RootObjectContainer* _CONSTRUCTOR(DataObjectContainer::GroupType type);// public RVA = 0x575CC0
    virtual ~RootObjectContainer();// public RVA = 0x502BD0 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x502BD0 vtable offset = 0x0
    virtual bool addActiveObject(RootObject* b);// public RVA = 0x502D90 vtable offset = 0x10
    bool _NV_addActiveObject(RootObject* b);// public RVA = 0x502D90 vtable offset = 0x10
    virtual bool removeObject(RootObject* c);// public RVA = 0x502CC0 vtable offset = 0x18
    bool _NV_removeObject(RootObject* c);// public RVA = 0x502CC0 vtable offset = 0x18
    virtual bool update();// public RVA = 0x502D20 vtable offset = 0x20
    bool _NV_update();// public RVA = 0x502D20 vtable offset = 0x20
    RootObject* getThing(int id);// public RVA = 0x278BC0
    int getNumThings() const;// public RVA = 0x278BE0
    lektor<RootObject*>* getThings();// public RVA = 0x286760
    virtual void getSelectedObjects(lektor<RootObject*>& out, itemType type, bool selectedOnly);// public RVA = 0x6B9890 vtable offset = 0x28
    void _NV_getSelectedObjects(lektor<RootObject*>& out, itemType type, bool selectedOnly);// public RVA = 0x6B9890 vtable offset = 0x28
    // no_addr void serialiseToContainer(class GameDataContainer * _a1);// public missing arg names
    void serialiseThings(const lektor<RootObject*>& _things, GameData* outputToInstanceCollectionOfSomeKind, GameDataContainer* source, PosRotPair* offsetPosToSubtract, const std::string& mod);// public RVA = 0x36C4C0
    void serialiseThings(GameData* outputToInstanceCollectionOfSomeKind, GameDataContainer* source, PosRotPair* offsetPosToSubtract, const std::string& mod);// public RVA = 0x36D890
    class SpecificItemLoadFirst
    {
    public:
        // VTable         : (none)
        // no_addr void SpecificItemLoadFirst(const class RootObjectContainer::SpecificItemLoadFirst & _a1);// public missing arg names
        SpecificItemLoadFirst(itemType BaseItemType, itemType _stateEnum, const std::string& _specificProperty, bool _desiredSpecificProperty);// public RVA = 0x7C9850
        SpecificItemLoadFirst* _CONSTRUCTOR(itemType BaseItemType, itemType _stateEnum, const std::string& _specificProperty, bool _desiredSpecificProperty);// public RVA = 0x7C9850
        // no_addr void SpecificItemLoadFirst();// public
        itemType baseTypes; // 0x8 Member
        itemType stateEnum; // 0xC Member
        std::string specificProperty; // 0x10 Member
        bool desiredSpecificProperty; // 0x38 Member
        virtual bool shouldSkip(GameSaveState* state);// public RVA = 0x36AC10 vtable offset = 0x0
        bool _NV_shouldSkip(GameSaveState* state);// public RVA = 0x36AC10 vtable offset = 0x0
        void flip();// public RVA = 0x37D700
        ~SpecificItemLoadFirst();// public RVA = 0x7C6380
        void _DESTRUCTOR();// public RVA = 0x7C6380
        // no_addr class RootObjectContainer::SpecificItemLoadFirst & operator=(const class RootObjectContainer::SpecificItemLoadFirst & _a1);// public missing arg names
        // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
    };
    lektor<RootObject*> things; // 0x50 Member
    virtual void loadToReality(bool skipSaveState, const Ogre::Vector3& positionMoved, const Ogre::Quaternion& rotOffset, FactoryCallbackInterface* callback, const std::string& specificSID, GameDataContainer* externalContainer, RootObjectContainer::SpecificItemLoadFirst* skipperClass);// protected RVA = 0x36D2C0 vtable offset = 0x30
    void _NV_loadToReality(bool skipSaveState, const Ogre::Vector3& positionMoved, const Ogre::Quaternion& rotOffset, FactoryCallbackInterface* callback, const std::string& specificSID, GameDataContainer* externalContainer, RootObjectContainer::SpecificItemLoadFirst* skipperClass);// protected RVA = 0x36D2C0 vtable offset = 0x30
    virtual void loadInstance(GameSaveState& state, bool skipSaveState, const Ogre::Vector3& pos, const Ogre::Quaternion& rot, FactoryCallbackInterface* callback, const Ogre::Vector3& positionMoved);// protected RVA = 0x36AC20 vtable offset = 0x38
    void _NV_loadInstance(GameSaveState& state, bool skipSaveState, const Ogre::Vector3& pos, const Ogre::Quaternion& rot, FactoryCallbackInterface* callback, const Ogre::Vector3& positionMoved);// protected RVA = 0x36AC20 vtable offset = 0x38
    // no_addr class RootObjectContainer & operator=(const class RootObjectContainer & _a1);// public missing arg names
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};