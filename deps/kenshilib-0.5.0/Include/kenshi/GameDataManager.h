#pragma once

#include "Enums.h"
#include "util/lektor.h"

#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <ogre/OgreMemoryAllocatorConfig.h>
#include <kenshi/util/OgreUnordered.h>

class GameData;
class Serialisable;

class GameDataContainer : public Ogre::GeneralAllocatedObject
{
public:
    // Ogre::AllocatedObject<Ogre::CategorisedAllocPolicy<0> > offset = 0x8, length = 0x1
    // VTable         : (none)
    void addNewData(GameData* dat, const std::string& forceID);// public RVA = 0x6BED40
    void addNewData(const lektor<GameData*>& dat);// protected RVA = 0x6BF070
    // no_addr void GameDataContainer(const class GameDataContainer & _a1);// public missing arg names
    GameDataContainer();// public RVA = 0x388000
    GameDataContainer* _CONSTRUCTOR();// public RVA = 0x388000
    virtual ~GameDataContainer();// public RVA = 0x6BFC60 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x6BFC60 vtable offset = 0x0
    GameData* createNewData(itemType type, const std::string& forceID, const std::string& name);// public RVA = 0x6BFD10
    GameData* getData(const std::string& sid);// public RVA = 0x6BCAA0
    GameData* getData(const std::string& sid, itemType category);// public RVA = 0x6BF420
    GameData* getData(int id);// public RVA = 0x6BBCD0
    GameData* getDataByName(const std::string& dataName, itemType category);// public RVA = 0x6BF360
    void getDataOfType(lektor<GameData*>& list, itemType type);// public RVA = 0x6BF9E0
    const ogre_unordered_map<int, GameData*>::type& _getAllData() const;// public RVA = 0x36ABF0
    // no_addr class lektor<GameData *> getInstancesFrom(class GameData * _a1, enum itemType _a2);// public missing arg names
    void renameData(GameData* data, const std::string& n);// public RVA = 0x6BF130
    void removeData(GameData* dat);// public RVA = 0x6BEC40
    void removeDuplicatesOf(GameData* dat);// public RVA = 0x6BFAA0
    void removeAllDataOfType(itemType type);// public RVA = 0x6BFBA0
    bool loadGameDataReturn(std::string file, bool isActive, bool readOnly, Serialisable* moreData);// public RVA = 0x6C1F60
    bool load(const std::string& filename, const std::string& modName, int modIndex, Serialisable* moreData, bool keepDeletedInstances);// public RVA = 0x6C0110
    bool save(const std::string& filename, Serialisable* moreData);// public RVA = 0x6BCB30
    void clearInstances();// public RVA = 0x6BFDC0
    void clearButDontDestroy();// public RVA = 0x6BE760
    void clearButDontDestroyPlatoons();// public RVA = 0x6BEB80
    void clearAndDestroy();// public RVA = 0x6BEAD0
    int getNewID();// public RVA = 0x6BBB20
    static void flushKillList();// public RVA = 0x6BB950
    static void destroyHomelessData(GameData* dat);// public RVA = 0x6BBF80
    void destroyData(GameData* dat);// public RVA = 0x6BF260
    void preLoadAllReferencePtrs();// public RVA = 0x6BD3C0
    void clearAllReferencePtrs();// public RVA = 0x6BB7F0
    void setName(const std::string& name);// public RVA = 0x3851C0
    void findAllDataThatReferencesThis(lektor<GameData*>& out, GameData* what, itemType ofType, const std::string& listname);// public RVA = 0x6BF6E0
    int currentID; // 0x8 Member
    void checkForDuplicates(GameData* dat);// protected RVA = 0x6BBBE0
    static ogre_unordered_set<GameData*>::type killList; // RVA = 0x1F0FC90 Static Member
    static void _addToKillList(GameData* d);// protected RVA = 0x6BBF30
    boost::unordered::unordered_map<int, boost::unordered::unordered_map<std::string, GameData*, boost::hash<std::string >, std::equal_to<std::string >, Ogre::STLAllocator<std::pair<std::string const, GameData*>, Ogre::GeneralAllocPolicy > >, boost::hash<int>, std::equal_to<int>, Ogre::STLAllocator<std::pair<int const, boost::unordered::unordered_map<std::string, GameData*, boost::hash<std::string >, std::equal_to<std::string >, Ogre::STLAllocator<std::pair<std::string const, GameData*>, Ogre::GeneralAllocPolicy > > >, Ogre::GeneralAllocPolicy > > gamedataCatName; // 0x10 Member
    ogre_unordered_map<int, GameData*>::type gamedataID; // 0x50 Member
    boost::unordered::unordered_map<int, boost::unordered::unordered_map<std::string, GameData*, boost::hash<std::string >, std::equal_to<std::string >, Ogre::STLAllocator<std::pair<std::string const, GameData*>, Ogre::GeneralAllocPolicy > >, boost::hash<int>, std::equal_to<int>, Ogre::STLAllocator<std::pair<int const, boost::unordered::unordered_map<std::string, GameData*, boost::hash<std::string >, std::equal_to<std::string >, Ogre::STLAllocator<std::pair<std::string const, GameData*>, Ogre::GeneralAllocPolicy > > >, Ogre::GeneralAllocPolicy > > gamedataCatSID; // 0x90 Member
    boost::unordered::unordered_map<std::string, GameData*, boost::hash<std::string >, std::equal_to<std::string >, Ogre::STLAllocator<std::pair<std::string const, GameData*>, Ogre::GeneralAllocPolicy > > gamedataSID; // 0xD0 Member
    ogre_unordered_set<GameData*>::type mainList; // 0x110 Member
    // no_addr bool saveGameData(class boost::unordered::unordered_map<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,GameData *,boost::hash<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::equal_to<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,Ogre::STLAllocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,GameData *>,Ogre::CategorisedAllocPolicy<0> > > & _a1, const class std::basic_string<char,std::char_traits<char>,std::allocator<char> > & _a2, bool _a3, class Serialisable * _a4);// protected missing arg names
    std::string name; // 0x150 Member
    bool isBaseDatafile; // 0x178 Member
    bool readOnly; // 0x179 Member
    // no_addr class GameDataContainer & operator=(const class GameDataContainer & _a1);// public missing arg names
    // no_addr void __local_vftable_ctor_closure();// public
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};

class GameDataManager : public GameDataContainer
{
public:
    // GameDataContainer offset = 0x0, length = 0x180
    // no_addr void GameDataManager(const class GameDataManager & _a1);// public missing arg names
    GameDataManager();// public RVA = 0x48B2B0
    GameDataManager* _CONSTRUCTOR();// public RVA = 0x48B2B0
    bool reloadGameData(std::string filename, bool isActive, bool _baseData);// public RVA = 0x6C32E0
    void postProcessingTheDatas();// public RVA = 0x6C2AB0
    void getBuildings(lektor<GameData*>& list, const std::string& _category);// public RVA = 0x6BF4F0
    GameData* getMapSector(int x, int y);// public RVA = 0x6C2000
    void updateDatasOfType(GameDataContainer* fromcontainer, itemType type, bool isMod);// private RVA = 0x6C2F50
    void updateData(GameData* from, bool isMod);// private RVA = 0x6BFE00
    virtual ~GameDataManager();// public RVA = 0x47A130 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x47A130 vtable offset = 0x0
    // no_addr class GameDataManager & operator=(const class GameDataManager & _a1);// public missing arg names
    // no_addr void __local_vftable_ctor_closure();// public
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};