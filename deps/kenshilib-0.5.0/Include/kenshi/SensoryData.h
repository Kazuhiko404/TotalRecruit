#pragma once

#include <ogre/OgreMemoryAllocatorConfig.h>
#include <ogre/OgreVector3.h>
#include <kenshi/util/OgreUnordered.h>
#include <kenshi/util/hand.h>
#include <string>
#include <kenshi/util/TimeOfDay.h>
#include <kenshi/util/lektor.h>
#include <kenshi/util/YesNoMaybe.h>
#include <kenshi/util/TagsClass.h>
#include <kenshi/Enums.h>

class Character;
class Faction;

// TODO move?
enum SenseType
{
    SENSE_ALLY,
    SENSE_NEUTRAL,
    SENSE_ENEMY,
    SENSE_KO,
    SENSE_DEAD,
    SENSE_SLAVE,
    SENSE_ESCAPED_SLAVE,
    SENSE_AUTHORITY_FIGURE,
    SENSE_CANT_SEE,
    SENSE_IN_CAGE,
    SENSE_SAME_FACTION,
    SENSE_ROBOTS,
    SENSE_ENEMY_OF_MY_SLAVEMASTER,
    SENSE_PLAYER,
    SENSE_CRAWLING,
    SENSE_CARRIED
};


// TODO move?
class SeenSomeone
{
public:
    // no_addr void SeenSomeone(const class SeenSomeone & _a1);// public missing arg names
    SeenSomeone();// private RVA = 0x85C570
    SeenSomeone* _CONSTRUCTOR();// private RVA = 0x85C570
    Ogre::Vector3 lastPosition; // 0x0 Member
    TimeOfDay lastSeenTime; // 0x10 Member
    Faction* percievedFaction; // 0x18 Member
    SlaveStateEnum percievedSlave; // 0x20 Member
    float FOVScore; // 0x24 Member
    TagsClass<SenseType> type; // 0x28 Member
    float lastSeenInSeconds();// public RVA = 0x850BC0
    bool canSee; // 0x38 Member
    bool canHear; // 0x39 Member
    float alarmState; // 0x3C Member
    Ogre::Vector3 getPosition();// public RVA = 0x331A00
    Faction* getFaction();// public RVA = 0x336350
    // no_addr enum SlaveStateEnum isSlave();// public
    // no_addr class SeenSomeone & operator=(const class SeenSomeone & _a1);// public missing arg names
};

// TODO move?
class SenseItr
{
public:
    // no_addr class hand getHandle();// public
    Character* getCharacter() const;// public RVA = 0x290500
    SeenSomeone* getData() const;// public RVA = 0x342570
    bool ended() const;// public RVA = 0x28DBA0
    // no_addr void operator++(int _a1);// public missing arg names
    void operator++();// public RVA = 0x28D8A0
    void increment();// public RVA = 0x852110
    SenseItr(unsigned int fany, unsigned int fnot, ogre_unordered_map<hand, SeenSomeone*>::type* list);// protected RVA = 0x8523D0
    SenseItr* _CONSTRUCTOR(unsigned int fany, unsigned int fnot, ogre_unordered_map<hand, SeenSomeone*>::type* list);// protected RVA = 0x8523D0
    boost::unordered::iterator_detail::c_iterator<boost::unordered::detail::ptr_node<std::pair<hand const, SeenSomeone*> > > it; // 0x0 Member
    boost::unordered::iterator_detail::c_iterator<boost::unordered::detail::ptr_node<std::pair<hand const, SeenSomeone*> > > _end; // 0x8 Member
    unsigned int flagsAny; // 0x10 Member
    unsigned int flagsNot; // 0x14 Member
};

class VisibleObjectInfo
{
public:
    VisibleObjectInfo(const VisibleObjectInfo& __that);// public RVA = 0x85C930
    VisibleObjectInfo* _CONSTRUCTOR(const VisibleObjectInfo& __that);// public RVA = 0x85C930
    // no_addr void VisibleObjectInfo();// public
    VisibleObjectInfo(const hand& h, float dist, bool enemy);// public RVA = 0x85C520
    VisibleObjectInfo* _CONSTRUCTOR(const hand& h, float dist, bool enemy);// public RVA = 0x85C520
    hand handle; // 0x0 Member
    float range; // 0x20 Member
    bool isEnemy; // 0x24 Member
    // no_addr float timePassedSinceLastSeen();// public
    VisibleObjectInfo& operator=(const VisibleObjectInfo& __that);// public RVA = 0x2E98A0
    // no_addr const class VisibleObjectInfo & operator=(const int _a1);// public missing arg names
};

class StateBroadcastData;
class DatapanelGUI;
class FloatingProgressBar;

class SensoryData
{
public:
    float nearestEnemy; // 0x0 Member
    // no_addr void SensoryData(const class SensoryData & _a1);// public missing arg names
    SensoryData(Character* _me);// public RVA = 0x640AB0
    SensoryData* _CONSTRUCTOR(Character* _me);// public RVA = 0x640AB0
    ~SensoryData();// public RVA = 0x854A60
    void _DESTRUCTOR();// public RVA = 0x854A60
    void periodicUpdate(float time);// public RVA = 0x859340
    void periodicUpdate_KOed();// public RVA = 0x857C90
    float getVisionRange(bool inTown);// public RVA = 0x850630
    void update(float frameTime);// public RVA = 0x852850
    void notifyKO();// public RVA = 0x5E7C00
    ogre_unordered_map<hand, SeenSomeone*>::type seen; // 0x8 Member
    SenseItr getIterator(unsigned int tagsAny, unsigned int tagsNOT);// public RVA = 0x28D8B0
    float getNearestEnemyDistanceSq() const;// public RVA = 0x595CB0
    int numUnconsciousAllies; // 0x48 Member
    int numConsciousAllies; // 0x4C Member
    bool canISeeThisGuy(Character* who);// public RVA = 0x8542A0
    bool canIHearThisGuy(Character* who);// public RVA = 0x854260
    bool amIAwareOfThisGuy(Character* who, bool needToSeeOrHear);// public RVA = 0x8543D0
    Ogre::Vector3 getLastKnownPositionOf(Character* who);// public RVA = 0x8542E0
    SeenSomeone* getDataFor(Character* who);// public RVA = 0x854180
    // no_addr class SeenSomeone * getDataAndReassessIfNeeded(class Character * _a1);// public missing arg names
    void notifyCriminalThreat(const hand& h);// public RVA = 0x5E3E10
    lektor<hand> threats; // 0x50 Member
    Ogre::vector<VisibleObjectInfo>::type flockingList; // 0x68 Member
    float getFactionRelation(RootObjectBase* c);// public RVA = 0x596F60
    StateBroadcastData* getStateBroadcast() const;// public RVA = 0x790A90
    StateBroadcastData* getStateBroadcastOf(const hand& who);// public RVA = 0x796410
    // no_addr float getStrengthOfHisGroup(const class hand & _a1, bool _a2);// public missing arg names
    float totalThreatLevelPersonal; // 0x88 Member
    float totalThreatLevelAllies; // 0x8C Member
    float totalIntendedThreatLevelGeneral; // 0x90 Member
    int numEnemies; // 0x94 Member
    int numNeutrals; // 0x98 Member
    float lastThreat; // 0x9C Member
    bool _shareSensesCheck(Character* leader, lektor<RootObject*>& list, float time);// public RVA = 0x8591E0
    void getGUIData(DatapanelGUI* panel, int cat);// public RVA = 0x796F50
    float _calculateFOVScore(Character* who);// public RVA = 0x8519E0
    bool _traceLineOfSightCheck(Character* who);// public RVA = 0x851E10
    Character* getCharacter() const;// public RVA = 0x790AB0
    void dialogAssessmentUpdate(float frameTime, bool inDepth);// public RVA = 0x859060
    void noticeThisPerson(Character* c, bool alarmed);// public RVA = 0x8579B0
    SeenSomeone* add(Character* c, bool see, bool hear);// private RVA = 0x856B40
    void remove(Character* c);// private RVA = 0x857970
    void decay(Character* c);// private RVA = 0x857A80
    bool canSee(Character* c, SeenSomeone* sees);// private RVA = 0x856580
    bool canHear(Character* c, SeenSomeone* see);// private RVA = 0x855F70
    bool disguiseMods(Character* c, float FOV);// private RVA = 0x855970
    void assessCrimes(Character* c);// private RVA = 0x853780
    void assessKidnapping(Character* c);// private RVA = 0x853460
    std::map<hand, float, std::less<hand>, std::allocator<std::pair<hand const, float> > > hearTestTimers; // 0xA0 Member
    void processKillList();// private RVA = 0x854DA0
    ogre_unordered_set<hand>::type killList; // 0xC0 Member
    void assessNeutral(Character* who, bool inDepth);// private RVA = 0x8580F0
    int currentAssessIndex; // 0x100 Member
    int currentAssessList; // 0x104 Member
    ogre_unordered_set<hand>::type assessList; // 0x108 Member
    Character* me; // 0x148 Member
    class SpottingPeopleMgr
    {
    public:
        void spot(const hand& h, float timelimit);// public RVA = 0x8548F0
        float getTime(const hand& h);// public RVA = 0x854940
        void update(float time);// public RVA = 0x852510
        void cantSeeAnymore(const hand& h);// public RVA = 0x861F90
        bool has(const hand& h) const;// public RVA = 0x860790
        void resetSeenFlags();// public RVA = 0x85F550
        class Spot
        {
        public:
            Spot();// public RVA = 0x850500
            Spot* _CONSTRUCTOR();// public RVA = 0x850500
            float timeSoFar; // 0x0 Member
            float timeLimitMax; // 0x4 Member
            bool stillSeen; // 0x8 Member
        };
        std::map<hand, SensoryData::SpottingPeopleMgr::Spot, std::less<hand>, Ogre::STLAllocator<std::pair<hand const, SensoryData::SpottingPeopleMgr::Spot>, Ogre::GeneralAllocPolicy > > spottedPeople; // 0x0 Member
        // no_addr void SpottingPeopleMgr(const class SensoryData::SpottingPeopleMgr & _a1);// public missing arg names
        SpottingPeopleMgr();// public RVA = 0x6401E0
        SpottingPeopleMgr* _CONSTRUCTOR();// public RVA = 0x6401E0
        ~SpottingPeopleMgr();// public RVA = 0x640200
        void _DESTRUCTOR();// public RVA = 0x640200
        // no_addr class SensoryData::SpottingPeopleMgr & operator=(const class SensoryData::SpottingPeopleMgr & _a1);// public missing arg names
        // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
    };
    SensoryData::SpottingPeopleMgr spottedSneakingPeople; // 0x150 Member
    SensoryData::SpottingPeopleMgr spottedSuspiciousPeople; // 0x178 Member
    FloatingProgressBar* progressBar; // 0x1A0 Member
    int progressBarStillNeeded; // 0x1A8 Member
    void setupMyProgressBar(const hand& who);// private RVA = 0x850C10
    void updateMyProgressBar(float progress, const std::string& text);// private RVA = 0x850C20
    hand spottedGuyIndexForProgressBar; // 0x1B0 Member
    YesNoMaybe canISeeThisGuyDoinSneakingOrSomething(Character* who, float FOVScore, SensoryData::SpottingPeopleMgr* memory, float spotTimeMinFOV, float spotTimeMaxFOV);// private RVA = 0x855720
    bool isIntruder_Base(Character* who);// private RVA = 0x850CB0
    bool isIntruder_Building(Character* who);// private RVA = 0x850DC0
    bool isInMyPrivate_Building(Character* who);// private RVA = 0x851030
    bool isEscapee(Character* who);// private RVA = 0x8555C0
    void reassessAll(lektor<RootObject*>& newTargets, float time);// private RVA = 0x858AE0
    SeenSomeone* reassess(Character* c, SeenSomeone* sees, float time);// private RVA = 0x857E40
    void addToThreatsAndFlockingList(Character* c);// private RVA = 0x852DE0
    bool amSharingThisFrame; // 0x1D0 Member
    void buildingSpotterUpdate();// private RVA = 0x853020
    // no_addr class SensoryData & operator=(const class SensoryData & _a1);// public missing arg names
    // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
};