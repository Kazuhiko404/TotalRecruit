#pragma once

#include <string>
#include <ogre/OgreMemoryAllocatorConfig.h>
#include <ogre/OgreVector3.h>
#include <ogre/OgreQuaternion.h>
#include <kenshi/util/OgreUnordered.h>
#include <kenshi/InstanceID.h>
#include <kenshi/Enums.h>

class GameSaveState
{
public:
    GameSaveState(const GameSaveState& __that);// public RVA = 0xF5C20
    GameSaveState* _CONSTRUCTOR(const GameSaveState& __that);// public RVA = 0xF5C20
    GameSaveState(GameDataContainer* source, GameData* _baseData, GameData* instanceCollectionSavingTo, const PosRotPair& _pos, PosRotPair* _offset, InstanceID* _instanceID);// public RVA = 0x386800
    GameSaveState* _CONSTRUCTOR(GameDataContainer* source, GameData* _baseData, GameData* instanceCollectionSavingTo, const PosRotPair& _pos, PosRotPair* _offset, InstanceID* _instanceID);// public RVA = 0x386800
    GameSaveState();// public RVA = 0xD5D80
    GameSaveState* _CONSTRUCTOR();// public RVA = 0xD5D80
    void generateNewInstanceID();// public RVA = 0x386250
    void createFromSerialisedInstanceData(GameDataContainer* _container, GameData::ObjectInstance* _instance, const std::string& id);// public RVA = 0x36C3F0
    std::string generateStateID(itemType type);// public RVA = 0x2AD480
    GameData* createState(itemType type);// public RVA = 0x3870F0
    void addState(GameData* state);// public RVA = 0x2AD580
    bool hasState(itemType type) const;// public RVA = 0xF3C60
    GameData* getState(itemType type);// public RVA = 0xF5340
    int numStates() const;// public RVA = 0x37F1F0
    // no_addr void clearStates();// public
    // no_addr enum itemType getObjectType();// public
    Ogre::Vector3 getPos() const;// public RVA = 0x37D620
    Ogre::Quaternion getRot() const;// public RVA = 0x37D640
    InstanceID getInstanceID() const;// public RVA = 0x5750F0
    const ogre_unordered_map<itemType, GameData*>::type& getAllStates() const;// public RVA = 0x6C5390
    GameData* baseData; // 0x0 Member
    GameDataContainer* dataSource; // 0x8 Member
    bool firstTime; // 0x10 Member
    operator bool() const;// public RVA = 0x37D670
    // no_addr const class GameSaveState & operator=(const class GameSaveState & _a1);// public missing arg names
    GameData::ObjectInstance* instance; // 0x18 Member
    GameData* getTheInstancesData();// public RVA = 0x37D690
    Ogre::Vector3 pos; // 0x20 Member
    Ogre::Quaternion rot; // 0x2C Member
    std::string instanceID; // 0x40 Member
    ogre_unordered_map<itemType, GameData*>::type states; // 0x68 Member
    ~GameSaveState();// public RVA = 0xD5E60
    void _DESTRUCTOR();// public RVA = 0xD5E60
    // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
};