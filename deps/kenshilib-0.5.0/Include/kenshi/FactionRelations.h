#pragma once

#include "util/OgreUnordered.h"
#include "Enums.h"

class Faction;
class GameData;
class DatapanelGUI;

class FactionRelations
{
public:
    // VTable         : (none)
    Faction* me; // 0x8 Member
    // no_addr void FactionRelations(const class FactionRelations & _a1);// public missing arg names
    FactionRelations();// public RVA = 0x811A50
    FactionRelations* _CONSTRUCTOR();// public RVA = 0x811A50
    void setupPhase1(Faction* f);// public RVA = 0x6B3540
    void setupPhase2();// public RVA = 0x6B3690
    bool isEnemyByDefault() const;// public RVA = 0x43FCE0
    void reset();// public RVA = 0x811920
    virtual void save(GameData* factionsList);// public RVA = 0x6B2D70 vtable offset = 0x0
    void _NV_save(GameData* factionsList);// public RVA = 0x6B2D70 vtable offset = 0x0
    virtual void load(GameData* gamestate_faction, bool playerOnly);// public RVA = 0x6B4380 vtable offset = 0x8
    void _NV_load(GameData* gamestate_faction, bool playerOnly);// public RVA = 0x6B4380 vtable offset = 0x8
    virtual void update();// public RVA = 0x6B1D90 vtable offset = 0x10
    void _NV_update();// public RVA = 0x6B1D90 vtable offset = 0x10
    bool _isAlly(Faction* c);// public RVA = 0x6B1D70
    bool _isEnemy(Faction* c);// public RVA = 0x6B1B90
    bool isEnemy(Faction* c);// public RVA = 0x6B1C90
    virtual void setEnemy(Faction* c);// public RVA = 0x6B2750 vtable offset = 0x18
    void _NV_setEnemy(Faction* c);// public RVA = 0x6B2750 vtable offset = 0x18
    bool isAlly(Faction* c);// public RVA = 0x6B1BF0
    bool isCoexisting(Faction* c);// public RVA = 0x6B1980
    float getFactionRelation(Faction* p);// public RVA = 0x6B19C0
    float getFactionRelationMultiplier(Faction* p);// public RVA = 0x6B1A20
    float getFactionRelationMultiplierInverse(Faction* p);// public RVA = 0x6B1A80
    void setRelation(Faction* who, float setTo);// public RVA = 0x6B4340
    enum FactionEvent
    {
        ATTACKED_US_DEFENSIVELY,
        ATTACKED_US_AGGRESSIVELY,
        DEFEATED_ONE_OF_US_DEFENSIVELY,
        DEFEATED_ONE_OF_US_AGGRESSIVELY,
        KILLED_ONE_OF_US_DIRECTLY,
        KILLED_ONE_OF_US_INDIRECTLY,
        EXECUTED_ONE_OF_US,
        AIDED_US_IN_BATTLE,
        FIRST_AIDED_US,
        TAKEN_TO_BED,
        DEFEATED_AN_ENEMY,
        KILLED_AN_ENEMY,
        CAPTURED_US
    };

    virtual void affectRelations(Faction* p, FactionEvent e, float mult);// public RVA = 0x6B22E0 vtable offset = 0x28
    void _NV_affectRelations(Faction* p, FactionEvent e, float mult);// public RVA = 0x6B22E0 vtable offset = 0x28
    virtual void affectRelations(Faction* p, float amount, float mult);// public RVA = 0x6B2460 vtable offset = 0x20
    void _NV_affectRelations(Faction* p, float amount, float mult);// public RVA = 0x6B2460 vtable offset = 0x20
    virtual void affectTrust(Faction* p, float amount, float mult);// public RVA = 0x6B1B20 vtable offset = 0x30
    void _NV_affectTrust(Faction* p, float amount, float mult);// public RVA = 0x6B1B20 vtable offset = 0x30
    virtual void setNoLongerEnemies(Faction* p);// public RVA = 0x6B2550 vtable offset = 0x38
    void _NV_setNoLongerEnemies(Faction* p);// public RVA = 0x6B2550 vtable offset = 0x38
    virtual void declareWar(Faction* p);// public RVA = 0x6B25C0 vtable offset = 0x40
    void _NV_declareWar(Faction* p);// public RVA = 0x6B25C0 vtable offset = 0x40
    virtual void affectReputation(Faction* p, float amount);// public RVA = 0x6B1AF0 vtable offset = 0x48
    void _NV_affectReputation(Faction* p, float amount);// public RVA = 0x6B1AF0 vtable offset = 0x48
    int playerRank; // 0x10 Member
    class RelationData
    {
    public:
        // no_addr void RelationData(const class RelationData & _a1);// public missing arg names
        RelationData(float rel, bool ally);// public RVA = 0x6B7FB0
        RelationData* _CONSTRUCTOR(float rel, bool ally);// public RVA = 0x6B7FB0
        RelationData();// public RVA = 0x6B7F70
        RelationData* _CONSTRUCTOR();// public RVA = 0x6B7F70
        void save(GameData* factionsList, const std::string& ID, Faction* who);// public RVA = 0x6B28C0
        void load(GameData* gamestate_faction, const std::string& ID, Faction* who);// public RVA = 0x6B2E90
        bool alliance; // 0x0 Member
        bool peaceTreaty; // 0x1 Member
        bool war; // 0x2 Member
        bool coexists; // 0x3 Member
        float relation; // 0x4 Member
        float trustPositives; // 0x8 Member
        float trustNegatives; // 0xC Member
        // no_addr float getTrust();// public
        float percievedStrength; // 0x10 Member
        // no_addr void setStateVar(const class std::basic_string<char,std::char_traits<char>,std::allocator<char> > & _a1, int _a2);// public missing arg names
        // no_addr int getStateVar(const class std::basic_string<char,std::char_traits<char>,std::allocator<char> > & _a1);// public missing arg names
        std::map<std::string, int, std::less<std::string >, Ogre::STLAllocator<std::pair<std::string const, int>, Ogre::GeneralAllocPolicy > > stateVariables; // 0x18 Member
        ~RelationData();// public RVA = 0x6B7DB0
        void _DESTRUCTOR();// public RVA = 0x6B7DB0
        RelationData& operator=(const RelationData& __that);// public RVA = 0x6B8290
        // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
    };
    float globalReputationTrust; // 0x14 Member
    float globalReputationForBadassery; // 0x18 Member
    virtual RelationData* getRelationData(Faction* p);// public RVA = 0x6B4220 vtable offset = 0x50
    RelationData* _NV_getRelationData(Faction* p);// public RVA = 0x6B4220 vtable offset = 0x50
    void getGUIData(DatapanelGUI* panel, int category);// public RVA = 0x7F32C0
    void getRelationsData(ogre_unordered_map<Faction*, float>::type& data);// public RVA = 0x6B1DF0
    // no_addr void setStateVar(class Faction * _a1, const class std::basic_string<char,std::char_traits<char>,std::allocator<char> > & _a2, int _a3);// public missing arg names
    // no_addr int getStateVar(class Faction * _a1, const class std::basic_string<char,std::char_traits<char>,std::allocator<char> > & _a2);// public missing arg names
    bool checkStateCondition(DialogConditionEnum conditionName, ComparisonEnum compareBy, int val, Faction* yourFaction);// public RVA = 0x670DB0
    ogre_unordered_map<Faction*, RelationData>::type _factionRelations; // 0x20 Member
    float defaultFactionRelation; // 0x60 Member
    ~FactionRelations();// public RVA = 0x811AC0
    void _DESTRUCTOR();// public RVA = 0x811AC0
    // no_addr class FactionRelations & operator=(const class FactionRelations & _a1);// public missing arg names
    // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
};