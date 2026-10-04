#pragma once

#include "Enums.h"
#include "util/lektor.h"
#include "util/YesNoMaybe.h"
#include "util/StringPair.h"
#include "util/hand.h"
#include "FitnessSelector.h"

#include <ogre/OgreVector3.h>

class MedicalSystem;
class Character;
class CombatTechniqueData;
class GameData;
class DatapanelGUI;
class Item;
class Weapon;
class Building;
class Damages;

class CharStats : public Ogre::GeneralAllocatedObject
{
public:
    // Ogre::GeneralAllocatedObject offset = 0x8, length = 0x1
    // VTable         : (none)
    MedicalSystem* medical; // 0x8 Member
    Character* me; // 0x10 Member
    float athleticsMultiplier; // 0x18 Member
    float combatSpeedMultiplier; // 0x1C Member
    int _skillBonusAttack; // 0x20 Member
    int _skillBonusDefence; // 0x24 Member
    int skillBonusIndoors; // 0x28 Member
    int skillBonusUnarmed; // 0x2C Member
    int skillBonusPerception; // 0x30 Member
    float ageMult; // 0x34 Member
    float skillMultDodge; // 0x38 Member
    float skillMultStealth; // 0x3C Member
    float skillMultAssassin; // 0x40 Member
    float skillMultDexterity; // 0x44 Member
    float skillMultDamage; // 0x48 Member
    float fistInjuryEquipmentMult; // 0x4C Member
    float skillMultRanged; // 0x50 Member
    std::map<WeatherAffecting, float, std::less<WeatherAffecting>, Ogre::STLAllocator<std::pair<WeatherAffecting const, float>, Ogre::GeneralAllocPolicy > > _weatherProtections; // 0x58 Member
    float getWeatherProtection(WeatherAffecting w) const;// public RVA = 0x885560
    // no_addr void CharStats(const class CharStats & _a1);// public missing arg names
    CharStats();// public RVA = 0x885670
    CharStats* _CONSTRUCTOR();// public RVA = 0x885670
    int unarmedEncumbrancePenalty() const;// public RVA = 0x882E90
    int skillBonusUnarmed_forGUI(bool factorEnvironment) const;// public RVA = 0x882ED0
    int skillBonusAttack_melee(bool factorEnvironment) const;// public RVA = 0x882D10
    int skillBonusAttack_unarmed(bool factorEnvironment) const;// public RVA = 0x882DD0
    int skillBonusDefence(bool factorEnvironment) const;// public RVA = 0x882F60
    void serialise(GameData* data);// public RVA = 0x64A170
    void updateStats(GameData* statData);// public RVA = 0x64AF20
    virtual void init(GameData* data, MedicalSystem* _med, Character* charact);// public RVA = 0x64C3B0 vtable offset = 0x0
    void _NV_init(GameData* data, MedicalSystem* _med, Character* charact);// public RVA = 0x64C3B0 vtable offset = 0x0
    void _randomiseStats(float amount);// public RVA = 0x643570
    void getGUIData(DatapanelGUI* panel, int category);// public RVA = 0x893BA0
    void printStealthStats(std::string& out);// public RVA = 0x88E150
    void getStealthTooltip(lektor<StringPair>* dats);// public RVA = 0x88E1F0
    void printRunSpeedStatMax(std::string& out);// public RVA = 0x887800
    void getAthleticsTooltip(lektor<StringPair>* dats);// public RVA = 0x88CFD0
    void getGUIDataForMainInfo(DatapanelGUI* datapanel, int category, bool combatMode);// public RVA = 0x890970
    std::string formatWholeStatStringWithBonuses(int base, int current) const;// public RVA = 0x887F60
    std::string formatWholeStatStringWithBonuses(StatsEnumerated stat) const;// public RVA = 0x8A8810
    enum GUIStatsDisplayMode
    {
        GUI_STATS_NORMAL,
        GUI_STATS_MARTIALARTIST,
        GUI_STATS_RANGED
    };

    CharStats::GUIStatsDisplayMode getGUIStatsDisplayMode();// public RVA = 0x882920
    virtual void periodicUpdate();// public RVA = 0x8823C0 vtable offset = 0x8
    void _NV_periodicUpdate();// public RVA = 0x8823C0 vtable offset = 0x8
    float calculateStumbleThresholdDamageAmount();// public RVA = 0x882870
    void setEquipmentStatBonuses(float athlet, float combatSpd, int attack, int def, float _stealth, int _unarmed, float _dodge, float fistdamage, int perception, float ranged, float dexmult, float assmult, float damagemult);// public RVA = 0x882BC0
    float getOverallSkillLevel_0_100() const;// public RVA = 0x8838A0
    float& getStatRef(StatsEnumerated what);// public RVA = 0x8829E0
    float getStat(StatsEnumerated what, bool unmodified) const;// public RVA = 0x883310
    static std::string getStatName(StatsEnumerated what);// public RVA = 0x886C90
    float getStatMultiplier(StatsEnumerated st) const;// public RVA = 0x883AF0
    std::string getStatMultiplierForGUI(StatsEnumerated st);// public RVA = 0x8881E0
    bool getStatPenaltiesForGUI(const std::string& statName, StatsEnumerated stat, lektor<StringPair>& dats);// public RVA = 0x88F350
    int getStatPenaltiesTotalForGUI(const std::string& statName, StatsEnumerated stat);// public RVA = 0x8828B0
    void xpStat_timeBased(StatsEnumerated st);// public RVA = 0x8C5AA0
    void xpStat_eventBased(StatsEnumerated st, float amount);// public RVA = 0x8C5B60
    void xpDodgeEvent(float enemySkill, bool successful);// public RVA = 0x8C4BB0
    float strengthActual() const;// public RVA = 0x337BE0
    float _strength; // 0x80 Member
    virtual float strengthBase() const;// public RVA = 0x640D00 vtable offset = 0x10
    float _NV_strengthBase() const;// public RVA = 0x640D00 vtable offset = 0x10
    float fitness; // 0x84 Member
    float _dexterity; // 0x88 Member
    float dexterityBase() const;// public RVA = 0x53E5E0
    float dexterityActual() const;// public RVA = 0x881F40
    float perception; // 0x8C Member
    float _toughness; // 0x90 Member
    float toughness() const;// public RVA = 0x595D20
    float getToughnessMult();// public RVA = 0x643220
    float _athletics; // 0x94 Member
    float medic; // 0x98 Member
    float massCombat; // 0x9C Member
    float arrowDefence; // 0xA0 Member
    float stealth; // 0xA4 Member
    float swimming; // 0xA8 Member
    float thieving; // 0xAC Member
    float lockpicking; // 0xB0 Member
    float bluff; // 0xB4 Member
    float assassin; // 0xB8 Member
    float survival; // 0xBC Member
    float tracking; // 0xC0 Member
    float climbing; // 0xC4 Member
    float doctor; // 0xC8 Member
    float engineer; // 0xCC Member
    float weaponSmith; // 0xD0 Member
    float armourSmith; // 0xD4 Member
    float bowSmith; // 0xD8 Member
    float robotics; // 0xDC Member
    float science; // 0xE0 Member
    float labouring; // 0xE4 Member
    float farming; // 0xE8 Member
    float cooking; // 0xEC Member
    float dodging; // 0xF0 Member
    float friendlyFire; // 0xF4 Member
    float katanas; // 0xF8 Member
    float sabres; // 0xFC Member
    float hackers; // 0x100 Member
    float blunt; // 0x104 Member
    float heavyWeapons; // 0x108 Member
    float unarmed; // 0x10C Member
    float bows; // 0x110 Member
    float turrets; // 0x114 Member
    float polearms; // 0x118 Member
    float getRangedFriendlyFireAvoidanceChance();// public RVA = 0x43FB80
    float getRangedAccuracyMult(StatsEnumerated stat);// public RVA = 0x4349E0
    float getReloadSkill(StatsEnumerated stat);// public RVA = 0x884530
    float currentItemMaximumJuryRig; // 0x11C Member
    void animalRecruitReduceStats();// public RVA = 0x6A9FB0
    float __meleeAttack; // 0x120 Member
    float _meleeDefence; // 0x124 Member
    float& getMeleeAttackRef();// protected RVA = 0x8C4620
    void _chooseAttacks(FitnessSelector<CombatTechniqueData*>& possibleAttacks, float range, float weaponReach, CombatTechniqueData* lastAttack, bool opponentIsStationary, bool skipMedicals, WeaponCategory _weaponType, float _weaponSkill);// protected RVA = 0x885F20
    float getMeleeDefence(bool includeDefensiveMode) const;// public RVA = 0x884AE0
    float getDodge(bool bonuses) const;// public RVA = 0x8839E0
    float getDodgePenalty_encumbrance() const;// public RVA = 0x883910
    float getDodgePenalty_injuries() const;// public RVA = 0x883970
    float getDodgePenalty_gear() const;// public RVA = 0x882F40
    float getMeleeDefence_melee(bool includeDefensiveMode) const;// public RVA = 0x8837A0
    float getMeleeAttack() const;// public RVA = 0x883730
    float getMeleeAttack_unarmed(bool bonuses) const;// public RVA = 0x883650
    float getMeleeAttack_melee() const;// public RVA = 0x8836D0
    float _getMeleeAttackBase() const;// public RVA = 0x670A30
    float getMaxHealAmount(Item* equipment, Building* bed, bool isRobot);// public RVA = 0x643B90
    bool isDefensiveMode() const;// public RVA = 0x2B26A0
    bool _defensiveMode; // 0x128 Member
    bool rangedMode; // 0x129 Member
    bool tauntMode; // 0x12A Member
    bool holdPositionMode() const;// public RVA = 0x2B26E0
    bool _holdPositionMode; // 0x12B Member
    bool passiveCombatMode; // 0x12C Member
    Ogre::Vector3 holdLocation; // 0x130 Member
    void setHoldLocation(const Ogre::Vector3& v);// public RVA = 0x331C20
    void clearHoldLocation();// public RVA = 0x331C40
    // no_addr bool hasHoldLocation();// public
    float warriorSpirit; // 0x13C Member
    float derivedSpirit; // 0x140 Member
    float aggression; // 0x144 Member
    float goodness; // 0x148 Member
    float evilness; // 0x14C Member
    CombatTechniqueData* getBashAnimation(float range);// public RVA = 0x885C70
    CombatTechniqueData* chooseAttack(float range, float weaponReach, CombatTechniqueData* lastAttack, bool opponentIsStationary);// public RVA = 0x886880
    float longestReachingAttack; // 0x150 Member
    CombatTechniqueData* chooseBlock(CutDirection dir, float opponentAttackSkill, CutOrigination from, Character* opponent);// public RVA = 0x8861C0
    float calculateStumbleBlockTimer(float stumbleForce);// public RVA = 0x884B00
    const std::string& getPainAnim(GameData* anatomyHit) const;// public RVA = 0x646430
    float calculateTechniqueInegrityCheckTimer();// public RVA = 0x884B10
    float calculateToughnessDamageResistanceMult();// public RVA = 0x643FF0
    float calculateToughnessWoundDegenerationRate();// public RVA = 0x6434F0
    float getAttackChance();// public RVA = 0x884690
    float getAttackCuttingDamage();// public RVA = 0x8823F0
    float getAttackBluntPower();// public RVA = 0x8824B0
    float getAttackBleedDamageMult();// public RVA = 0x8824A0
    float getAttackPierceDamage();// public RVA = 0x882560
    Damages getTotalAttackDamageFor(Character* target);// public RVA = 0x885100
    virtual float getEquippedWeaponSkill() const;// public RVA = 0x8A7400 vtable offset = 0x18
    float _NV_getEquippedWeaponSkill() const;// public RVA = 0x8A7400 vtable offset = 0x18
    float getSkillDifferenceRatio(float mine, float his);// public RVA = 0x8C47F0
    enum DeadTimeState
    {
        ATTACK_HIT,
        ATTACK_WAS_BLOCKED,
        BLOCKED_IT,
        ATTACK_SLOT_OPEN,
        WAS_HIT,
        ATTACK_INTERRUPTED,
        ATTACK_MISSED
    };

    float calculateDeadTime(CharStats::DeadTimeState state);// public RVA = 0x882840
    float STUBBOURNESS; // 0x154 Member
    float PROFESSIONALNESS; // 0x158 Member
    float HOLDING_POSITION; // 0x15C Member
    float IMMEDIACY; // 0x160 Member
    float IMMEDIACY_MAX_RANGE; // 0x164 Member
    float MANLINESS; // 0x168 Member
    float MURDEROUS; // 0x16C Member
    void xpMelee(CharStats::DeadTimeState what, Character* target, const Damages& damage);// public RVA = 0x8C51D0
    void xpToughness_RagdollEvent();// public RVA = 0x8C4F70
    void xpToughness_GetUpEvent();// public RVA = 0x8C4FD0
    void xpToughness_PunchSomething(HitMaterialType mat);// public RVA = 0x8C5130
    void xpFirstAid(Character* patient, float time, StatsEnumerated medicStat);// public RVA = 0x8C4980
    void xpRunning(float time, float speed);// public RVA = 0x8C4EB0
    void xpStealth(float time, bool enemiesAbout, YesNoMaybe seen, bool isMoving);// public RVA = 0x8C4DF0
    void xpStealthHearCheckEvent(bool enemiesAbout, bool trespassing, bool seen);// public RVA = 0x8C5B80
    StringPair stealthXPMultForGUI();// public RVA = 0x8C5C30
    float _stealthXPMultForGUI; // 0x170 Member
    float stealthXPTooManyCooksPenalty; // 0x174 Member
    void xpMassCombat();// public RVA = 0x8C46D0
    void xpEngineering(float time);// public RVA = 0x8C4AA0
    void xpLockpicking(int lockLevel, bool success);// public RVA = 0x8C4B30
    void xpTraining(float time, float mult, float& stat, float upperLimit, StatsEnumerated whatStatIsIt);// public RVA = 0x8C4D40
    void xpGeneral(float time, float mult, StatsEnumerated what);// public RVA = 0x8C4C80
    float calculateAthleticsXPMult(float speed);// public RVA = 0x8C4710
    float calculateStrengthXPMultFromWalking();// public RVA = 0x8C4910
    void setWeapon(Weapon* _weapon);// public RVA = 0x896780
    bool isUnarmed() const;// public RVA = 0x333D20
    // no_addr float getWeaponWeightBonus();// public
    float getWeaponWeightXPBonus();// public RVA = 0x881FA0
    float getAttackSpeed();// public RVA = 0x2ADAF0
    float getBlockSpeed();// public RVA = 0x607A50
    float getMaxRunSpeed();// public RVA = 0x2ADB00
    float getCurrentWeaponLength() const;// public RVA = 0x607A60
    float getStealthSkill01(bool modded) const;// public RVA = 0x8830A0
    float getAttackSuccessChanceSkill();// public RVA = 0x607A70
    static float _convertWeaponWeightToBluntMultiplier(float weaponWeight);// public RVA = 0x8820F0
    static float _convertBluntMultiplierToWeaponWeight(float blunt);// public RVA = 0x882100
    float getEncumbranceMult() const;// public RVA = 0x2ADB10
    float calculateHungerMult();// public RVA = 0x881FF0
    void printExertionHungerMultTooltip(lektor<StringPair>* dats);// public RVA = 0x888B00
    float calculateTheoreticalIdealMaxRunSpeed();// public RVA = 0x882600
    float calculateWeaponWeightXPMult(float weaponWeight);// public RVA = 0x882130
    bool hasWeapon() const;// public RVA = 0x595D40
    // no_addr enum WeaponCategory getCurrentWeaponType();// public
    float calculateMaxStealthSpeed();// public RVA = 0x883170
    float _calculateMaxSwimSpeed();// public RVA = 0x882680
    float calculateSwimSpeed();// public RVA = 0x884700
    float currentOperatingMachineHungerRate; // 0x178 Member
    float calculateDodgeChance(float versus, bool stumbling);// public RVA = 0x8845D0
    void calculateMaxRunSpeed();// protected RVA = 0x884CB0
    float moveSpeed; // 0x17C Member
    float _calculateEncumberanceMult();// protected RVA = 0x882FD0
    void _calculateWeaponWeightSpeedMultiplier(float weaponWeight);// protected RVA = 0x8821F0
    LeftRight currentWeaponHand();// protected RVA = 0x882110
    float calculateAttackOrBlockSpeed(float weaponWeightSpeedMult, float attackOrBlockSkill, bool isBlock);// protected RVA = 0x8822D0
    float weaponWeightSpeedMult; // 0x180 Member
    float weaponWeightXPMult; // 0x184 Member
    float attackSpeed; // 0x188 Member
    float blockSpeed; // 0x18C Member
    float encumbranceMult; // 0x190 Member
    float _calculateBlockChance(float versus);// protected RVA = 0x884B20
    virtual void _recalculateStats();// protected RVA = 0x884F00 vtable offset = 0x20
    void _NV__recalculateStats();// protected RVA = 0x884F00 vtable offset = 0x20
    static void setupCombatTechniques();// protected RVA = 0x8874F0
    static lektor<CombatTechniqueData*> attacks; // RVA = 0x200EF78 Static Member
    static lektor<CombatTechniqueData*> blocks; // RVA = 0x200EF90 Static Member
    float xp; // 0x194 Member
    int freeAttributePoints; // 0x198 Member
    float cutDamageMult; // 0x19C Member
    float bluntDamageMult; // 0x1A0 Member
    float bleedDamageMult; // 0x1A4 Member
    float pierceDamageMult; // 0x1A8 Member
    float cutDamageMin; // 0x1AC Member
    float bluntDamageMin; // 0x1B0 Member
    float bonusRobots; // 0x1B4 Member
    float bonusHumans; // 0x1B8 Member
    float bonusAnimals; // 0x1BC Member
    float bonusArmourPenetration; // 0x1C0 Member
    // this is weird - I think the template might actually take a policy as an arg?
    std::map<GameData*, float, std::less<GameData*>, Ogre::STLAllocator<std::pair<GameData* const, float>, Ogre::GeneralAllocPolicy > > bonusRaces; // 0x1C8 Member
    WeaponCategory currentWeaponType; // 0x1F0 Member
    float* pCurrentWeaponSkill; // 0x1F8 Member
    float currentWeaponLength; // 0x200 Member
    virtual void setEquippedWeaponSkill(float v);// protected RVA = 0x8A7410 vtable offset = 0x28
    void _NV_setEquippedWeaponSkill(float v);// protected RVA = 0x8A7410 vtable offset = 0x28
    hand weapon; // 0x208 Member
    float weaponWeight; // 0x228 Member
    ~CharStats();// public RVA = 0x6404F0
    void _DESTRUCTOR();// public RVA = 0x6404F0
    // no_addr class CharStats & operator=(const class CharStats & _a1);// public missing arg names
    // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
};