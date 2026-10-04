#pragma once

#include "util/lektor.h"
#include "util/hand.h"

class GameData;
class Faction;
class DatapanelGUI;
class Town;
class UniquePlatoon;

class ProsperityManager
{
public:
    double actualProsperity; // 0x0 Member
    float maxProsperity; // 0x8 Member
    ProsperityManager();// public RVA = 0x2DD000
    ProsperityManager* _CONSTRUCTOR();// public RVA = 0x2DD000
    void setup(GameData* faction);// public RVA = 0x2E0600
    void load(GameData* state);// public RVA = 0x2E0420
    void save(GameData* state);// public RVA = 0x2E06E0
    float getProsperityMultiplier() const;// public RVA = 0x2DD020
    void getGUIData(DatapanelGUI* panel, int cat);// public RVA = 0x2DE6A0
    void update(float time);// public RVA = 0x2DD030
    void notifySquadDefeated(GameData* squadTemplate);// public RVA = 0x6B8D50
    void notifySpecialNPCDead(GameData* npc);// public RVA = 0x7BF380
};

class FactionUniqueSquadManager
{
public:
    // VTable         : (none)
    // no_addr void FactionUniqueSquadManager(const class FactionUniqueSquadManager & _a1);// public missing arg names
    FactionUniqueSquadManager();// private RVA = 0x2DF270
    FactionUniqueSquadManager* _CONSTRUCTOR();// private RVA = 0x2DF270
    virtual ~FactionUniqueSquadManager();// private RVA = 0x2E07B0 vtable offset = 0x0
    void _DESTRUCTOR();// private RVA = 0x2E07B0 vtable offset = 0x0
    void clearAndReset();// private RVA = 0x2DE930
    void initialiseNew(Faction* faction, GameData* mdat, Town* t);// private RVA = 0x2DD7E0
    GameData* chooseRandomSquadToSpawn(float time);// public RVA = 0x2DD150
    void periodicUpdate(float time);// public RVA = 0x2DD1D0
    void serialise(GameData* state);// public RVA = 0x2DE9F0
    void load(GameData* state, bool import);// public RVA = 0x2DEC10
    void getGUIData(DatapanelGUI* panel, int cat);// public RVA = 0x2DEEF0
    void endOfUniqueSquad(UniquePlatoon* who);// public RVA = 0x2DE500
    ProsperityManager prosperityMgr; // 0x8 Member
    Faction* me; // 0x18 Member
    GameData* squadListData; // 0x20 Member
    Town* homeTown; // 0x28 Member
    class UniqueSpawnData
    {
    public:
        // no_addr void UniqueSpawnData(const class FactionUniqueSquadManager::UniqueSpawnData & _a1);// public missing arg names
        UniqueSpawnData(GameData* d, int num);// public RVA = 0x2E0370
        UniqueSpawnData* _CONSTRUCTOR(GameData* d, int num);// public RVA = 0x2E0370
        GameData* squadTemplate; // 0x0 Member
        int desiredNumberToHave; // 0x8 Member
        float respawnTimer; // 0xC Member
        int currentNumber() const;// public RVA = 0x2E0070
        lektor<hand> existingSquadsList; // 0x10 Member
        ~UniqueSpawnData();// public RVA = 0x2E00C0
        void _DESTRUCTOR();// public RVA = 0x2E00C0
        // no_addr class FactionUniqueSquadManager::UniqueSpawnData & operator=(const class FactionUniqueSquadManager::UniqueSpawnData & _a1);// public missing arg names
        // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
    };
    lektor<FactionUniqueSquadManager::UniqueSpawnData*> squads; // 0x30 Member
    FactionUniqueSquadManager::UniqueSpawnData* getExistingSquadsFor(GameData* squad);// private RVA = 0x2E0080
    UniquePlatoon* spawnNewUniqueSquad(GameData* squadtemplate);// private RVA = 0x2DD1B0
    // no_addr class FactionUniqueSquadManager & operator=(const class FactionUniqueSquadManager & _a1);// public missing arg names
    // no_addr void __local_vftable_ctor_closure();// public
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// private vtable offset = 0x0 missing arg names
};