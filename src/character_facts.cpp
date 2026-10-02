#include "common.h"
#include "character_facts.h"
#include "engine_reads.h"
#include "char_key_reader.h"
#include "config.h"
#include "log.h"
#include "body_rules.h"
#include "chance_rules.h"
#include "price_rules.h"
#include "temperament_map.h"
#include "defeats.h"
#include "insistence.h"
#include "hold_tracking.h"
#include "rescue_memory.h"
#include "surrender.h"
#include "route_decisions.h"

#include <kenshi/CharacterAnimal.h>
#include <kenshi/CharStats.h>

namespace totalrecruit
{
namespace
{
// The raw numbers behind the stress bar, kept apart from Condition because they
// are only ever read together.
struct MedicalSnapshot
{
    float worstDamage;
    float restedState;

    MedicalSnapshot() : worstDamage(0.0f), restedState(1.0f) {}
};

struct Condition
{
    bool dying;
    bool unconscious;
    bool starving;
    float hunger;
    float health;
    float blood;
    // Blood as a 0..1 fraction of this character's own maximum. See I3.
    float bloodFraction;
    // Diagnostics only. These are what the engine reports piecemeal; the
    // verdict never reads them, but the log does when a call looks wrong.
    bool rawUnconscious;
    bool rawTrauma;
    bool rawSub50KO;
    float rawKOTimer;

    Condition()
        : dying(false), unconscious(false), starving(false),
          hunger(0.0f), health(1.0f), blood(1.0f), bloodFraction(1.0f),
          rawUnconscious(false), rawTrauma(false), rawSub50KO(false),
          rawKOTimer(0.0f)
    {
    }
};

bool SafeReadBody(Character* character, MedicalSnapshot* out)
{
    TR_TRY
    {
        MedicalSystem* medical = character->getMedical();
        if (medical == NULL)
        {
            return false;
        }
        out->worstDamage = medical->worstDamage;
        out->restedState = medical->restedState;
        return true;
    }
    TR_CATCH { return false; }
}

bool SafeReadCondition(Character* character, Condition* out)
{
    TR_TRY
    {
        MedicalSystem* medical = character->getMedical();
        if (medical == NULL)
        {
            return false;
        }
        out->dying       = medical->isProbablyDying();
        // The engine's own answer, and the only one that means "actually out
        // cold". The four raw flags this used to OR together are each true for
        // reasons that are not unconsciousness -- sub50KO in particular stays
        // set for a crippled limb, so an NPC walking around with a lost arm
        // was reported as asleep and could never be recruited. _NV_ because
        // the virtual cannot be resolved from a plugin (see notes S11).
        // PS_KO is added because the engine's own predicate misses it: a level
        // 77 character measured isDown=yes, proneState=PS_KO and
        // isLiterallyUnconcious=no at the same time, so the mod quoted him a
        // price of 347k cats while he lay on the floor. Being knocked out is
        // the prone state saying so, whatever the predicate thinks.
        //
        // PS_PLAYING_DEAD stays out of this on purpose -- someone faking it is
        // awake, and treating the fake as real is how the player farms
        // recruits off people who never went down.
        out->unconscious = character->_NV_isLiterallyUnconciousNotPretending() ||
                           (character->getProneState() == PS_KO);
        // Kept only as corroboration for the log, never as the verdict.
        out->rawUnconscious = medical->isUnconcious();
        out->rawTrauma      = medical->isInBloodlossTrauma();
        out->rawKOTimer     = medical->knockoutTimer;
        out->rawSub50KO     = medical->sub50KO;
        out->hunger      = medical->hunger;
        out->health      = medical->getOverallHealthRating();
        out->blood       = medical->blood;
        // I3: the honest measure of how close to death someone is.
        // getOverallHealthRating reads 0 for anyone unconscious however intact,
        // so it cannot answer this -- blood over max blood can, and it keeps
        // working while they are down, which is exactly when it is asked.
        {
            const float maxBlood = medical->getMaxBlood();
            out->bloodFraction = (maxBlood > 0.0f)
                                     ? (medical->blood / maxBlood)
                                     : 1.0f;
            if (out->bloodFraction < 0.0f) out->bloodFraction = 0.0f;
            if (out->bloodFraction > 1.0f) out->bloodFraction = 1.0f;
        }
        out->starving    = medical->isReallyHungry() || medical->isHungerKO();
        return true;
    }
    TR_CATCH { return false; }
}

bool SafeIsUnique(Character* character, bool* out)
{
    TR_TRY { *out = character->isUnique(); return true; }
    TR_CATCH { return false; }
}

bool SafeRoughLevel(Character* character, int* out)
{
    TR_TRY { *out = character->getRoughLevel(); return true; }
    TR_CATCH { return false; }
}

bool SafePlayerMoney(Character* leader, int* out)
{
    TR_TRY { *out = leader->getMoney(); return true; }
    TR_CATCH { return false; }
}

bool SafeFactionRelation(Character* character, Faction* playerFaction, float* out)
{
    TR_TRY
    {
        Faction* theirs = character->getFaction();
        if (theirs == NULL || theirs->relations == NULL)
        {
            return false;
        }
        *out = theirs->relations->getFactionRelation(playerFaction);
        return true;
    }
    TR_CATCH { return false; }
}

// Whether this animal belongs to an organisation of people rather than roaming
// loose.
//
// Measured before being trusted: a bonedog in "Wild Dogs" read
// notARealFaction=yes, while a Bone Mutt in the Holy Nation Outlaws and a pack
// beast in the Viera Tribes both read no -- and the Bone Mutt had even
// inherited its owners' OT_MILITARY type.
//
// A later session narrowed what the flag actually means: "Matones de Bar",
// "Orphans" and "Rogue Traders" all read notARealFaction=yes and are entirely
// human. So it does not mark wildlife -- it marks a loose band with no standing
// organisation behind it. That still answers the question being asked here,
// because an animal in such a band has no owner either, but only because this
// is reached for animals alone. It is not a wildlife test and must not be
// reused as one.
//
// The player's own animals are excluded because they are already home.
bool SafeBelongsToRealFaction(Character* character, bool* out)
{
    TR_TRY
    {
        Faction* faction = character->getFaction();
        if (faction == NULL)
        {
            return false;
        }
        *out = !faction->isNotARealFaction() && !faction->isThePlayer();
        return true;
    }
    TR_CATCH { return false; }
}

// Reads false when the answer cannot be had: refusing to recruit on the strength
// of a pointer that would not resolve is the worse failure.
bool BelongsToSomeone(Character* character)
{
    bool owned = false;
    if (!GetConfig().animals.ownedNeedCage)
    {
        return false;
    }
    return SafeBelongsToRealFaction(character, &owned) && owned;
}

// How grown an animal is, 0..1 of its own lifespan. A pup and an old bull are not
// the same negotiation, and the engine tracks both numbers already.
bool SafeAnimalMaturity(Character* character, float* out)
{
    TR_TRY
    {
        CharacterAnimal* beast = character->isAnimal();
        if (beast == NULL || beast->lifespanInDays <= 0.0f)
        {
            return false;
        }
        float m = beast->age / beast->lifespanInDays;
        if (m < 0.0f) m = 0.0f;
        if (m > 1.0f) m = 1.0f;
        *out = m;
        return true;
    }
    TR_CATCH { return false; }
}

// What someone is actually good at, and how far past their own level it goes.
//
// getRoughLevel() is an average, so a level 30 assassin and a level 30 baker
// with a sword price identically -- which is exactly the complaint Recruit
// Prisoners for Cats answers by hand-pricing every NPC. Reading the stats gets
// there without a table, and without a patch per mod.
struct Talent
{
    // Best stat found, and how far above the character's own level it sits.
    // 1.0 means "as good as their level suggests"; 2.0 means twice that.
    float edge;
    // Which family that best stat belongs to: how rare that kind of skill is.
    TalentFamily family;

    Talent() : edge(1.0f), family(TalentFamily_Combat) {}
};

bool SafeReadTalent(Character* character, int level, Talent* out)
{
    TR_TRY
    {
        CharStats* stats = character->getStats();
        if (stats == NULL || level < 1)
        {
            return false;
        }

        // Three families, because they are three different reasons to want
        // somebody. Combat is what every other mod prices; the other two are
        // what Kenshi actually makes scarce.
        const StatsEnumerated combat[5] = {
            STAT_MELEE_ATTACK, STAT_MELEE_DEFENCE, STAT_DODGE,
            STAT_TOUGHNESS, STAT_MARTIALARTS
        };
        const StatsEnumerated shadow[4] = {
            STAT_STEALTH, STAT_ASSASSINATION, STAT_THIEVING, STAT_LOCKPICKING
        };
        const StatsEnumerated craft[8] = {
            STAT_SCIENCE, STAT_ENGINEERING, STAT_ROBOTICS, STAT_SMITHING_WEAPON,
            STAT_SMITHING_ARMOUR, STAT_MEDIC, STAT_FARMING, STAT_LABOURING
        };

        float best = 0.0f;
        TalentFamily family = TalentFamily_Combat;

        for (int i = 0; i < 5; ++i)
        {
            const float v = stats->getStat(combat[i], false);
            if (v > best) { best = v; family = TalentFamily_Combat; }
        }
        for (int i = 0; i < 4; ++i)
        {
            const float v = stats->getStat(shadow[i], false);
            if (v > best) { best = v; family = TalentFamily_Shadow; }
        }
        for (int i = 0; i < 8; ++i)
        {
            const float v = stats->getStat(craft[i], false);
            if (v > best) { best = v; family = TalentFamily_Craft; }
        }

        out->edge = best / (float)level;
        out->family = family;
        return true;
    }
    TR_CATCH { return false; }
}

RelationFact ReadRelation(Character* character, Faction* playerFaction)
{
    RelationFact fact;
    float value = 0.0f;
    fact.known = SafeFactionRelation(character, playerFaction, &value);
    fact.value = fact.known ? value : 0.0f;
    return fact;
}

PurseFact ReadPurse()
{
    PurseFact fact;
    Character* leader = NULL;
    int purse = 0;
    if (SafePlayerLeader(&leader) && SafePlayerMoney(leader, &purse))
    {
        fact.known = true;
        fact.cats = purse;
    }
    return fact;
}

// The key a roll is seeded from. Without an identity there is no stable answer to
// give, and inventing one that changes per call is worse than refusing, so a
// character whose key cannot be read simply cannot pass the roll.
RollFact MintRollKey(Character* character)
{
    RollFact fact;
    fact.haveKey = SafeCharKey(character, &fact.key);
    return fact;
}

// Faction relation only ever shifts the price, and only in Vanilla+: as a gate it
// would make half the world unrecruitable, which is exactly the problem this mod
// exists to solve. The arithmetic (and its clamps) is price_rules.cpp.
float FactionMultiplier(const RelationFact& relation, const Config& config)
{
    if (config.preset != Preset_VanillaPlus || !relation.known)
    {
        return 1.0f;
    }
    return FactionPriceMultiplier(relation.value, config.prices);
}

// Reads everything that decides the quote; price_rules.cpp does the arithmetic. The
// player's own level is deliberately not an input: the price is what this person is
// worth, and how hard they are to win over is already the level gap's job in
// JoinChance. Pricing both would charge twice for one fact and make the same
// prisoner cost different amounts depending on who is asking.
PriceInputs GatherPrice(Character* character, const CoreFacts& core,
                        const RelationFact& relation)
{
    const Config& config = GetConfig();

    PriceInputs inputs;
    inputs.level = core.npcLevel < 1 ? 1 : core.npcLevel;
    inputs.personality = core.temperament;

    // An edge of 1.0 is somebody exactly as good as their level suggests and
    // changes nothing; the multiplier only bites for people who are unusually good
    // at something.
    Talent talent;
    if (SafeReadTalent(character, inputs.level, &talent) && talent.edge > 1.0f)
    {
        // A craftsman is worth more than a swordsman of the same level: Kenshi
        // hands you fighters constantly and engineers almost never.
        inputs.talentMultiplier = TalentMultiplier(
            talent.edge, TalentFamilyWeight(talent.family, config.prices), config.prices);
    }

    // A named character is not just a strong one: they are the only one there is.
    // The reference mod puts its uniques at 100-150k against ~80k for the strongest
    // ordinary fighter, and that gap is the whole point of a name.
    inputs.unique = core.unique;
    inputs.factionMultiplier = FactionMultiplier(relation, config);
    return inputs;
}

AnimalExtra GatherAnimal(Character* character, const CoreFacts& core)
{
    AnimalExtra extra;
    // Only the open world refuses an owned animal; caged, it is already yours.
    extra.belongsToSomeone = !core.inMyCage && BelongsToSomeone(character);
    float maturity = 1.0f;
    SafeAnimalMaturity(character, &maturity);
    extra.maturity = maturity;
    return extra;
}

Faction* PlayerFaction()
{
    Faction* faction = NULL;
    SafePlayerFaction(&faction);
    return faction;
}
} // namespace

bool SafeReadTraits(Character* character, Traits* out)
{
    TR_TRY
    {
        StateBroadcastData* broadcast = character->getStateBroadcast();
        if (broadcast == NULL)
        {
            return false;
        }
        out->npcType         = (int)broadcast->NPCType;
        out->personality     = (int)broadcast->myPersonality;
        out->temperament     = ToTemperament(out->personality);
        out->slaveState      = (int)broadcast->_slaveState;
        out->escapedPrisoner = broadcast->isEscapedPrisoner;
        return true;
    }
    TR_CATCH { return false; }
}

void GatherCoreFacts(Character* character, CoreFacts* out)
{
    const Config& config = GetConfig();
    CoreFacts f;

    bool animal = false;
    SafeIsAnimal(character, &animal);
    f.animal = animal;

    Traits traits;
    if (!SafeReadTraits(character, &traits))
    {
        f.traitsReadable = false;
        *out = f;
        return;
    }
    f.traitsReadable = true;
    f.temperament = traits.temperament;
    f.archetype = ToArchetype(traits.npcType);
    f.enslaved = (traits.slaveState == IS_SLAVE || traits.slaveState == ESCAPING_SLAVE ||
                  traits.npcType == OT_SLAVE);

    Condition condition;
    SafeReadCondition(character, &condition);
    f.unconscious = condition.unconscious;
    f.dying = condition.dying;
    f.hunger = condition.hunger;
    f.health = condition.health;
    f.bloodFraction = condition.bloodFraction;
    f.wounded = IsWounded(condition.unconscious, condition.dying, condition.health,
                          config.prices.woundedThreshold);
    f.starving = IsStarving(condition.starving, condition.hunger, config.prices.hungerThreshold);

    MedicalSnapshot body;
    SafeReadBody(character, &body);
    f.worstDamage = body.worstDamage;
    f.restedState = body.restedState;

    bool pretending = false;
    SafeIsPretending(character, &pretending);
    f.pretending = pretending;

    bool corpse = false;
    f.corpse = SafeIsDead(character, &corpse) && corpse;

    // Levels, read once for every route that compares them.
    Character* leader = NULL;
    SafePlayerLeader(&leader);
    int npcLevel = 1;
    SafeRoughLevel(character, &npcLevel);
    f.npcLevel = npcLevel;
    int playerLevel = 1;
    if (leader != NULL)
    {
        SafeRoughLevel(leader, &playerLevel);
    }
    f.playerLevel = playerLevel;

    bool unique = false;
    f.unique = SafeIsUnique(character, &unique) && unique;

    bool grudge = false;
    if (leader != NULL)
    {
        SafeMemoryTag(character, leader, LT_KILLED_MY_FRIEND, &grudge);
    }
    f.grudge = grudge;

    f.rememberedAsSaviour = RemembersPlayerAsSaviour(character);
    f.defeats = DefeatCount(character);
    f.inMyCage = IsInPlayerCage(character);
    f.surrendered = config.surrenderEnabled && !animal && IsSurrendered(character);

    *out = f;
}

CheatFacts GatherCheatFacts(Character* character)
{
    const Config& config = GetConfig();
    CheatFacts facts;
    facts.inMyCage = IsInPlayerCage(character);
    // Reading the hold also notes the sighting, so it is only done once it can
    // decide something.
    if (config.cheatNeedsCage && facts.inMyCage)
    {
        facts.hoursHeld = HoursHeld(character);
    }
    return facts;
}

RescueExtra GatherRescueExtra(Character* character)
{
    RescueExtra extra;
    extra.relation = ReadRelation(character, PlayerFaction());
    extra.roll = MintRollKey(character);
    return extra;
}

YieldExtra GatherYieldExtra(Character* character)
{
    YieldExtra extra;
    extra.roll = MintRollKey(character);
    return extra;
}

// Stops reading where the decision stops (another cage, a pestered NPC, an animal),
// so a click on a bystander does not run the whole talent survey for a price nobody
// is going to quote.
FreeExtra GatherFreeExtra(Character* character, const CoreFacts& core)
{
    const Config& config = GetConfig();
    FreeExtra extra;

    SafeInPrison(character, &extra.inAnyCage);
    if (extra.inAnyCage)
    {
        return extra;
    }

    extra.ignoringPlayer = IsIgnoringPlayer(character);
    if (extra.ignoringPlayer)
    {
        return extra;
    }

    if (core.animal)
    {
        extra.animal = GatherAnimal(character, core);
        return extra;
    }

    extra.relation = ReadRelation(character, PlayerFaction());
    extra.purse = ReadPurse();
    extra.price = GatherPrice(character, core, extra.relation);

    // The roll's key is only minted when the route can roll at all, so asking
    // about a bystander does not give them an id.
    if (FreeWorldMayRoll(core, extra.relation, config))
    {
        extra.roll = MintRollKey(character);
    }
    return extra;
}

CageExtra GatherCageExtra(Character* character, const CoreFacts& core)
{
    const Config& config = GetConfig();
    CageExtra extra;

    // The wait is only read when it can decide something: reading it notes the
    // sighting in the hold clock.
    if (config.captivityEnabled && config.prices.easyHoursCaged > 0.0f)
    {
        extra.hoursHeld = HoursHeld(character);
    }

    extra.roll = MintRollKey(character);
    extra.purse = ReadPurse();
    // The relation is only a price factor here (the cage chance ignores standing).
    extra.price = GatherPrice(character, core, ReadRelation(character, PlayerFaction()));
    if (core.animal)
    {
        extra.animal = GatherAnimal(character, core);
    }
    return extra;
}

// Why a character was judged down or dead, in the log.
void LogUnconsciousCondition(Character* character)
{
    Condition c;
    SafeReadCondition(character, &c);

    char buf[256] = { 0 };
    _snprintf_s(buf, sizeof(buf), _TRUNCATE,
                "[unconscious] literal=%d raw_unc=%d trauma=%d sub50KO=%d koTimer=%.1f "
                "blood=%.2f health=%.2f dying=%d",
                c.unconscious ? 1 : 0, c.rawUnconscious ? 1 : 0,
                c.rawTrauma ? 1 : 0, c.rawSub50KO ? 1 : 0, c.rawKOTimer,
                c.blood, c.health, c.dying ? 1 : 0);
    LogInfo(CharacterName(character) + " " + buf);
}
} // namespace totalrecruit
