#pragma once
#include "util/OgreUnordered.h"

#include <string>

class GameData;
class Faction;

enum WorldStateEnum
{
    DEAD,
    ALIVE,
    IMPRISONED
};

class WorldEventStateQuery
{
public:
    enum WarStateEnum
    {
        PEACE,
        WAR,
        ALLIANCE
    };

    static ogre_unordered_map<GameData*, WorldEventStateQuery*>::type statesData; // RVA = 0x2094420 Static Member
    // no_addr void WorldEventStateQuery(const class WorldEventStateQuery & _a1);// public missing arg names
    WorldEventStateQuery();// private RVA = 0x9AF250
    WorldEventStateQuery* _CONSTRUCTOR();// private RVA = 0x9AF250
    ogre_unordered_map<GameData*, WorldStateEnum>::type uniqueNPCsAre; // 0x0 Member
    ogre_unordered_map<GameData*, WorldStateEnum>::type uniqueNPCsAreNot; // 0x40 Member
    ogre_unordered_map<GameData*, WorldStateEnum>::type towns; // 0x80 Member
    ogre_unordered_map<Faction*, bool>::type isAllyOf; // 0xC0 Member
    ogre_unordered_map<Faction*, bool>::type isEnemyOf; // 0x100 Member
    bool playerInvolvement; // 0x140 Member
    static WorldEventStateQuery* getFromData(GameData* d);// public RVA = 0x9A7DC0
    static bool checkAllStatesInObject(GameData* what, const std::string& listname);// public RVA = 0x9A82C0
    bool isTrue() const;// public RVA = 0x9A6650
    // no_addr void ~WorldEventStateQuery();// public
    // no_addr class WorldEventStateQuery & operator=(const class WorldEventStateQuery & _a1);// public missing arg names
    // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
};

class WorldEventStateQueryList
{
public:
    bool setupFrom(GameData* d, const std::string& listname);// public RVA = 0x9A8E20
    void reset();// public RVA = 0xA05FB0
    ogre_unordered_map<WorldEventStateQuery*, bool>::type statesList; // 0x0 Member
    bool isTrue() const;// public RVA = 0x69DC70
    // no_addr void WorldEventStateQueryList(const class WorldEventStateQueryList & _a1);// public missing arg names
    WorldEventStateQueryList();// public RVA = 0x6A5880
    WorldEventStateQueryList* _CONSTRUCTOR();// public RVA = 0x6A5880
    ~WorldEventStateQueryList();// public RVA = 0x80ECF0
    void _DESTRUCTOR();// public RVA = 0x80ECF0
    // no_addr class WorldEventStateQueryList & operator=(const class WorldEventStateQueryList & _a1);// public missing arg names
    // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
};