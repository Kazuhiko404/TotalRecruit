#include "common.h"
#include "rules.h"
#include "config.h"
#include "recruit_core.h"
#include "surrender.h"
#include "log.h"
#include "messages.h"

#include <kenshi/CharacterAnimal.h>
#include <kenshi/CharStats.h>

#include <sstream>
#include <map>

namespace totalrecruit
{
namespace
{
// Engine pointers can already be freed, and reading one access-violates rather
// than returning null, so every dereference is guarded. Each guard keeps a
// POD-only frame: C2712 counts every C++ object in the whole function, not just
// the ones inside the __try block.

bool SafePlayerFaction(Faction** out)
{
    TR_TRY
    {
        if (ou == NULL || ou->player == NULL)
        {
            return false;
        }
        *out = ou->player->participant;
        return (*out != NULL);
    }
    TR_CATCH { return false; }
}

bool SafePlayerLeader(Character** out)
{
    TR_TRY
    {
        if (ou == NULL || ou->player == NULL)
        {
            return false;
        }
        Platoon* platoon = ou->player->currentPlatoon;
        if (platoon == NULL || platoon->activePlatoon == NULL)
        {
            return false;
        }
        *out = platoon->getSquadLeader();
        return (*out != NULL);
    }
    TR_CATCH { return false; }
}

struct Traits
{
    int npcType;
    int personality;
    int slaveState;
    bool escapedPrisoner;

    Traits() : npcType(0), personality(0), slaveState(0), escapedPrisoner(false) {}
};

// Personality and archetype come from StateBroadcastData, which the engine
// populates for every Character -- including ones added by mods that have never
// heard of this plugin. That is what makes universal coverage possible.
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
        out->slaveState      = (int)broadcast->_slaveState;
        out->escapedPrisoner = broadcast->isEscapedPrisoner;
        return true;
    }
    TR_CATCH { return false; }
}

struct Condition
{
    bool dying;
    bool unconscious;
    bool starving;
    bool wounded;
    float hunger;
    float health;
    float blood;
    // Diagnostics only. These are what the engine reports piecemeal; the
    // verdict never reads them, but the log does when a call looks wrong.
    bool rawUnconscious;
    bool rawTrauma;
    bool rawSub50KO;
    float rawKOTimer;

    Condition()
        : dying(false), unconscious(false), starving(false), wounded(false),
          hunger(0.0f), health(1.0f), blood(1.0f),
          rawUnconscious(false), rawTrauma(false), rawSub50KO(false),
          rawKOTimer(0.0f)
    {
    }
};

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
        out->unconscious = character->_NV_isLiterallyUnconciousNotPretending();
        // Kept only as corroboration for the log, never as the verdict.
        out->rawUnconscious = medical->isUnconcious();
        out->rawTrauma      = medical->isInBloodlossTrauma();
        out->rawKOTimer     = medical->knockoutTimer;
        out->rawSub50KO     = medical->sub50KO;
        out->hunger      = medical->hunger;
        out->health      = medical->getOverallHealthRating();
        out->blood       = medical->blood;
        out->starving    = medical->isReallyHungry() || medical->isHungerKO();
        return true;
    }
    TR_CATCH { return false; }
}

// Why a character was judged down or dead, in the log. Written whenever a
// verdict turns on medical state, because those are the calls that have been
// wrong before and the headers cannot say which flag lied.
void LogCondition(Character* character, const Condition& c, const char* gate)
{
    char buf[256] = { 0 };
    _snprintf_s(buf, sizeof(buf), _TRUNCATE,
                "[%s] literal=%d raw_unc=%d trauma=%d sub50KO=%d koTimer=%.1f "
                "blood=%.2f health=%.2f dying=%d",
                gate, c.unconscious ? 1 : 0, c.rawUnconscious ? 1 : 0,
                c.rawTrauma ? 1 : 0, c.rawSub50KO ? 1 : 0, c.rawKOTimer,
                c.blood, c.health, c.dying ? 1 : 0);
    LogInfo(CharacterName(character) + " " + buf);
}

// PS_PLAYING_DEAD is a real prone state, so "unconscious" alone would let the
// player farm recruits off NPCs that are only pretending.
bool SafeIsPretending(Character* character, bool* out)
{
    TR_TRY
    {
        // PS_PLAYING_DEAD is the engine's own answer, so ask for it. The old
        // test -- down but not unconscious -- also caught anyone whose legs are
        // crippled: they cannot stand, so isDown() is true, and they are wide
        // awake, so the negation is true too. A one-legged NPC standing there
        // talking was being called a faker.
        *out = (character->getProneState() == PS_PLAYING_DEAD);
        return true;
    }
    TR_CATCH { return false; }
}

bool SafeIsUnique(Character* character, bool* out)
{
    TR_TRY { *out = character->isUnique(); return true; }
    TR_CATCH { return false; }
}

bool SafeIsDead(Character* character, bool* out)
{
    TR_TRY { *out = character->isDead(); return true; }
    TR_CATCH { return false; }
}

bool SafeIsAnimal(Character* character, bool* out)
{
    TR_TRY { *out = (character->isAnimal() != NULL); return true; }
    TR_CATCH { return false; }
}

// How grown an animal is, 0..1 of its own lifespan. A pup and an old bull are
// not the same negotiation, and the engine tracks both numbers already.
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

// Feeding a starving animal is the whole of taming when the animal is small,
// and only the opening when it is not.
//
// The shape borrowed from survival games that do this well: a young or weak
// creature bonds on the meal alone, because it has no other option and no
// reason to distrust you. A grown one weighs whether you can keep feeding it,
// which in Kenshi terms means whether you look stronger than it is.
//
// So the cost is one number -- meals -- and maturity and level decide how many.
int MealsToTame(int npcLevel, int playerLevel, float maturity)
{
    // A cub or a weakling: one meal and it follows you home.
    if (maturity < 0.25f || npcLevel <= 10)
    {
        return 1;
    }

    // Everyone else pays for their size, then discounts for your reputation as
    // something that can clearly feed itself.
    int meals = 2 + (int)(maturity * 3.0f);          // 2..5
    meals += (npcLevel > 30) ? 1 : 0;

    const int edge = playerLevel - npcLevel;
    if (edge >= 20)      meals -= 2;
    else if (edge >= 10) meals -= 1;

    return meals < 1 ? 1 : meals;
}

bool SafeRoughLevel(Character* character, int* out)
{
    TR_TRY { *out = character->getRoughLevel(); return true; }
    TR_CATCH { return false; }
}

// inSomething only reads IN_PRISON while the character is actually inside a
// cage object. A prisoner you carried in and dropped, or one wearing shackles,
// reads IN_NOTHING -- so captivity is the union of the three signals the engine
// offers rather than just that one.
bool SafeInPrison(Character* character, bool* out)
{
    TR_TRY
    {
        *out = (character->inSomething == IN_PRISON) ||
               character->isChainedMode() ||
               character->isKidnapped();
        return true;
    }
    TR_CATCH { return false; }
}

// The cage is resolved as RootObjectBase rather than Building: getFaction lives
// on the base, and including Building.h collides with Platoon.h over
// BuildingDesignation, which both define identically and neither guards.
bool SafeCageFaction(Character* character, Faction** out)
{
    TR_TRY
    {
        RootObjectBase* cage = character->inWhat.getRootObjectBase();
        if (cage == NULL)
        {
            return false;
        }
        *out = cage->getFaction();
        return (*out != NULL);
    }
    TR_CATCH { return false; }
}

// isMySlave / isMyFactionsSlave answer "does this belong to me" for a captive
// with no cage object to inspect.
bool SafeIsMyPrisoner(Character* leader, Character* captive, bool* out)
{
    TR_TRY
    {
        *out = leader->isMySlave(captive) || leader->isMyFactionsSlave(captive);
        return true;
    }
    TR_CATCH { return false; }
}

bool SafeMemoryTag(Character* character, Character* about,
                   CharacterPerceptionTags_LongTerm tag, bool* out)
{
    TR_TRY
    {
        *out = character->getCharacterMemoryTag(about, tag);
        return true;
    }
    TR_CATCH { return false; }
}

bool SafeRememberTag(Character* character, Character* about,
                     CharacterPerceptionTags_LongTerm tag)
{
    TR_TRY
    {
        character->rememberCharacter(about, tag);
        return true;
    }
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

// Faction relation runs -100..100, and the player reads it constantly without
// being taught: the cursor turns yellow over an ally. Naming the bands here
// means the rules and the dialogue always agree about what a number means.
const char* RelationBand(float relation)
{
    if (relation >= 75.0f)  return "aliado";
    if (relation >= 25.0f)  return "amistoso";
    if (relation >= 5.0f)   return "cordial";
    if (relation >= -5.0f)  return "neutral";
    if (relation >= -15.0f) return "receloso";
    if (relation >= -30.0f) return "hostil";
    return "enemigo";
}

// A chance the player cannot re-roll by clicking. PLAN.md settled that this mod
// has no dice, and the reason was save-scumming: a percentage rolled per click
// is really "click until it works". Seeding from the character means they see a
// real chance that stays decided once it is decided.
bool StableChance(Character* character, int percent)
{
    if (percent >= 100) return true;
    if (percent <= 0)   return false;

    const unsigned int seed = (unsigned int)((uintptr_t)character >> 4);
    // Knuth's multiplicative hash, so neighbouring pointers do not land in
    // neighbouring buckets.
    return (int)((seed * 2654435761u) % 100u) < percent;
}

// Being on the same side is worth something even when the player is the weaker
// one -- but not everything. Level gap decides how much.
int AllyJoinChance(int npcLevel, int playerLevel)
{
    const int gap = npcLevel - playerLevel;
    if (gap <= 0)
    {
        return 100;   // You are their equal or better; allies follow that.
    }
    // Falls off with the gap and floors at 10: an ally is never a flat no,
    // which is the whole difference between a modifier and a gate.
    const int chance = 100 - (gap * 6);
    return chance < 10 ? 10 : chance;
}

bool SafePlayerMoney(Character* leader, int* out)
{
    TR_TRY { *out = leader->getMoney(); return true; }
    TR_CATCH { return false; }
}

// How long this captive has been held, in in-game hours.
//
// The engine does not record when captivity began, so the first sighting is
// taken as the start. That undercounts a prisoner taken before the plugin ever
// saw them, which errs towards making the player wait -- the safe direction.
// It lives only in memory: a reload restarts the clock rather than freeing
// someone the player never actually held.
std::map<Character*, double> g_heldSince;

struct TalkState
{
    int attempts;
    double lastHour;
    double ignoredUntilHour;

    TalkState() : attempts(0), lastHour(0.0), ignoredUntilHour(0.0) {}
};

std::map<Character*, TalkState> g_talkStates;

bool SafeGameHours(double* out)
{
    TR_TRY
    {
        if (ou == NULL)
        {
            return false;
        }
        *out = ou->getTimeStamp_inGameHours().getTotalHours();
        return true;
    }
    TR_CATCH { return false; }
}

float HoursHeld(Character* character)
{
    double now = 0.0;
    if (!SafeGameHours(&now))
    {
        return 0.0f;
    }

    std::map<Character*, double>::iterator found = g_heldSince.find(character);
    if (found == g_heldSince.end())
    {
        g_heldSince[character] = now;
        return 0.0f;
    }

    const double held = now - found->second;
    return (held > 0.0) ? (float)held : 0.0f;
}

// Faction relation runs roughly -100..100. It only ever shifts the price, and
// only in Vanilla+: as a gate it would make half the world unrecruitable, which
// is exactly the problem this mod exists to solve.
float FactionMultiplier(Character* character, Faction* playerFaction)
{
    const Config& config = GetConfig();
    if (config.preset != Preset_VanillaPlus)
    {
        return 1.0f;
    }

    float relation = 0.0f;
    if (!SafeFactionRelation(character, playerFaction, &relation))
    {
        return 1.0f;
    }

    const float normalised = relation / 100.0f;
    float multiplier = 1.0f - (normalised * config.prices.factionWeight);
    if (multiplier < 0.25f)
    {
        multiplier = 0.25f;
    }
    if (multiplier > 3.0f)
    {
        multiplier = 3.0f;
    }
    return multiplier;
}

float PersonalityMultiplier(int personality)
{
    const Prices& prices = GetConfig().prices;
    switch (personality)
    {
    case PT_FEARFUL:
        return prices.fearfulMultiplier;
    case PT_BRAVE:
    case PT_COLD_CRUEL:
    case PT_MANIACAL:
        return prices.braveMultiplier;
    default:
        return 1.0f;
    }
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
    // Which family that best stat belongs to, for the line the player reads.
    const char* kind;

    Talent() : edge(1.0f), kind("") {}
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
        const char* kind = "";

        for (int i = 0; i < 5; ++i)
        {
            const float v = stats->getStat(combat[i], false);
            if (v > best) { best = v; kind = "combate"; }
        }
        for (int i = 0; i < 4; ++i)
        {
            const float v = stats->getStat(shadow[i], false);
            if (v > best) { best = v; kind = "sombra"; }
        }
        for (int i = 0; i < 8; ++i)
        {
            const float v = stats->getStat(craft[i], false);
            if (v > best) { best = v; kind = "oficio"; }
        }

        out->edge = best / (float)level;
        out->kind = kind;
        return true;
    }
    TR_CATCH { return false; }
}

int PriceInCats(Character* character, const Traits& traits, Faction* playerFaction)
{
    const Prices& prices = GetConfig().prices;

    int level = 1;
    SafeRoughLevel(character, &level);
    if (level < 1)
    {
        level = 1;
    }

    // Two curves added together, because no single one covers the range.
    //
    // The bottom has to stay cheap: helping a new player buy a level 3 bandit
    // for a few hundred cats is most of the point. The top has to reach the
    // tens of thousands, or a legendary fighter is priced like a farmhand.
    // A straight line cannot do both, and a power curve steep enough for the
    // top makes the bottom unaffordable.
    //
    // So: a gentle square term carries the early levels, and a logistic S-curve
    // -- the standard game-design answer for a cost that should accelerate and
    // then level off -- carries the rest. Shifted to pass through zero so the
    // sigmoid does not charge for merely existing.
    //
    // Calibrated against the published Recruit-Prisoners-for-Cats tiers:
    // ~1k at 5 stats, ~7k at 12, ~20k at 25, ~35k at 40, ~55k at 50, ~80k at 75.
    const float lv = (float)level;
    const float shape = prices.priceCurveSteepness;
    const float mid   = prices.priceCurveMidpoint;
    const float sigNow  = 1.0f / (1.0f + expf(-shape * (lv - mid)));
    const float sigZero = 1.0f / (1.0f + expf(shape * mid));

    float cats = ((float)prices.catsPerLevel * 0.1f) * lv * lv
               + (float)prices.catsCeiling * (sigNow - sigZero);
    if (cats < (float)prices.catsMinimum)
    {
        cats = (float)prices.catsMinimum;
    }

    // Talent, only in the presets that price at all. An edge of 1.0 is somebody
    // exactly as good as their level suggests and changes nothing; the multiplier
    // only bites for people who are unusually good at something.
    const Config& cfg = GetConfig();
    if (cfg.preset != Preset_Cheat && cfg.prices.talentWeight > 0.0f)
    {
        Talent talent;
        if (SafeReadTalent(character, level, &talent) && talent.edge > 1.0f)
        {
            // A craftsman is worth more than a swordsman of the same level:
            // Kenshi hands you fighters constantly and engineers almost never.
            float familyWeight = 1.0f;
            if (strcmp(talent.kind, "oficio") == 0)      familyWeight = 1.6f;
            else if (strcmp(talent.kind, "sombra") == 0) familyWeight = 1.3f;

            const float over = (talent.edge - 1.0f) * familyWeight;
            cats *= 1.0f + (over * cfg.prices.talentWeight);
        }
    }

    // A named character is not just a strong one: they are the only one there
    // is. The reference mod puts its uniques at 100-150k against ~80k for the
    // strongest ordinary fighter, and that gap is the whole point of a name.
    bool named = false;
    if (SafeIsUnique(character, &named) && named)
    {
        cats *= GetConfig().prices.uniqueMultiplier;
    }

    cats *= PersonalityMultiplier(traits.personality);
    cats *= FactionMultiplier(character, playerFaction);

    return (int)cats;
}

// Which currency does this archetype deal in? Condition takes precedence over
// archetype: a starving bandit wants food before he wants cats.
// A named character is not one kind of problem. The bandit king and the Holy
// Nation's leader are both unique, and only one of them has a price.
//
// The split is what they believe in. Someone who already sells people, or who
// enjoys the cruelty, understands a transaction perfectly well -- money is a
// language they speak. A zealot does not: offering coin to someone who thinks
// you are unclean is the wrong tool entirely, and the only thing that moves
// them is being kept and broken.
//
// Both are recruitable. Neither is free.
bool UniqueTakesMoney(const Traits& traits)
{
    switch (traits.personality)
    {
    case PT_COLD_CRUEL:
    case PT_TRAITOROUS:
    case PT_MANIACAL:
    case PT_SMART:
        return true;
    default:
        break;
    }

    // Failing personality, the trade they chose says it instead.
    return traits.npcType == OT_BANDIT || traits.npcType == OT_TRADER ||
           traits.npcType == OT_SLAVER;
}

Currency ChooseCurrency(const Traits& traits, const Condition& condition, bool animal)
{
    if (animal)
    {
        return Currency_Care;
    }

    if (traits.slaveState == IS_SLAVE || traits.slaveState == ESCAPING_SLAVE ||
        traits.npcType == OT_SLAVE)
    {
        return Currency_Freedom;
    }

    if (condition.starving || condition.wounded)
    {
        return Currency_Care;
    }

    // Honorable y Warm/Kind nunca aceptan soborno por dinero (Cats)
    if (traits.personality == PT_HONORABLE || traits.personality == PT_WARM_KIND)
    {
        return Currency_Care;
    }

    switch (traits.npcType)
    {
    case OT_BANDIT:
    case OT_TRADER:
    case OT_SLAVER:
    case OT_CIVILIAN:
    case OT_ADVENTURER:
    case OT_DIPLOMAT:
        return Currency_Cats;

    case OT_MILITARY:
    case OT_LAW_ENFORCEMENT:
        return Currency_Submission;

    default:
        return (traits.personality == PT_BRAVE) ? Currency_Submission : Currency_Cats;
    }
}
} // namespace

// Player-facing text is Spanish to match the game it is built against.
// Non-ASCII is written as UTF-8 hex escapes because the v100 toolset reads this
// source as Latin-1, so an accented character typed directly arrives mangled.

const char* CurrencyName(Currency currency)
{
    switch (currency)
    {
    case Currency_None:       return "nada";
    case Currency_Cats:       return "cats";
    case Currency_Care:       return "cuidados";
    case Currency_Freedom:    return "libertad";
    case Currency_Submission: return "sumision";
    case Currency_Refused:    return "rechazo";
    default:                  return "?";
    }
}

// The NPC's own voice. Archetype decides what they care about, personality
// decides how they say it -- so a maniacal bandit and a fearful farmer never
// open the same way, without the mod author writing a branch for each pairing.
std::string SpeakOpeningLine(Character* character, const Verdict& verdict)
{
    // A beak thing does not negotiate in Spanish. It gets a sound rather than a
    // sentence, and blanking these keys in messages.json silences it entirely --
    // which is the other reasonable answer.
    if (verdict.isAnimal)
    {
        if (verdict.viaRescue)
        {
            return Say("animal.rescued", "*gime bajo y no te quita los ojos de encima*",
                       character);
        }
        if (verdict.currency == Currency_Care)
        {
            return Say("animal.hungry", "*olfatea tus manos, buscando comida*", character);
        }
        if (verdict.currency == Currency_Refused)
        {
            return Say("animal.refused", "*gruñe y retrocede*", character);
        }
        return Say("animal.calm", "*te observa, quieto*", character);
    }

    Traits traits;
    if (character == NULL || !SafeReadTraits(character, &traits))
    {
        return "\xc2\xbfY bien? Habla de una vez.";
    }

    if (verdict.currency == Currency_Refused)
    {
        // A refusal that comes from the relationship should sound like the
        // relationship. An ally who thinks you are too green and a zealot who
        // hates your banner are not saying the same thing.
        if (!verdict.band.empty())
        {
            const std::string keyed =
                Say("faction." + verdict.band + ".greeting", "", character);
            if (!keyed.empty())
            {
                return keyed;
            }
        }

        if (verdict.reason.find("harto") != std::string::npos ||
            verdict.reason.find("pesado") != std::string::npos)
        {
            return "\xc2\xa1" "Me est\xc3\xa1" "s hartando! D\xc3\xa9jame en paz.";
        }
        if (verdict.reason.find("d\xc3\xa9" "bil") != std::string::npos ||
            verdict.reason.find("fuerte") != std::string::npos)
        {
            return "\xc2\xbf" "T\xc3\xba pretender darme \xc3\xb3" "rdenes? Eres demasiado d\xc3\xa9" "bil. L\xc3\xa1" "rgate.";
        }
        if (verdict.reason.find("honor") != std::string::npos)
        {
            return "El honor no se compra con gatos. No me rebajar\xc3\xa9 a esto.";
        }
        // A loyal soldier has no idea who the player is; the grudge line was
        // accusing every one of them of something they never did.
        if (verdict.reason.find("lealtad") != std::string::npos ||
            verdict.reason.find("reputaci") != std::string::npos ||
            verdict.reason.find("respeto") != std::string::npos)
        {
            return "\xc2\xbf" "Qui\xc3\xa9n eres t\xc3\xba? \xc2\xbf" "Qu\xc3\xa9 quieres de m\xc3\xad?";
        }
        // The one refusal that really is about something the player did.
        if (verdict.reason.find("mataste") != std::string::npos)
        {
            return "S\xc3\xa9 lo que hiciste. Fuera de mi vista.";
        }
        return "\xc2\xbf" "Qui\xc3\xa9n eres t\xc3\xba? No tengo nada que hablar contigo.";
    }

    if (verdict.viaRescue)
    {
        switch (traits.personality)
        {
        case PT_HONORABLE:
            return "Estaba muerto y no me abandonaste. Eso es una deuda.";
        case PT_COLD_CRUEL:
        case PT_MANIACAL:
            return "Me curaste. Qu\xc3\xa9 raro. Nadie hace eso por aqu\xc3\xad.";
        case PT_FEARFUL:
            return "Cre\xc3\xad que era mi fin. Y te quedaste.";
        default:
            return "Pudiste seguir de largo. No lo hiciste.";
        }
    }

    switch (verdict.currency)
    {
    case Currency_Freedom:
        return "Qu\xc3\xadtame estas cadenas y ser\xc3\xa9 tuyo.";

    case Currency_Care:
        if (traits.personality == PT_HONORABLE || traits.personality == PT_WARM_KIND)
        {
            return "No me interesan tus monedas. Solo valoro a quien ayuda de coraz\xc3\xb3n.";
        }
        if (traits.personality == PT_FEARFUL)
        {
            return "Por favor... dame algo de comer y te seguir\xc3\xa9.";
        }
        return "No he comido nada. Dame comida y hablamos.";

    case Currency_Submission:
        switch (traits.personality)
        {
        case PT_HONORABLE:
            return "Me venciste con honor, pero eso no me hace tuyo. Todav\xc3\xad" "a no.";
        case PT_MANIACAL:
            return "\xc2\xa1""Adelante! Haz lo peor que puedas, solo me da risa.";
        case PT_FEARFUL:
            return "No pienso pelear contigo. Solo... no me dejes aqu\xc3\xad.";
        default:
            return "Me tienes enjaulado. Eso es todo lo que tienes.";
        }

    case Currency_None:
        if (!verdict.willJoin)
        {
            return "";
        }
        return "Estoy listo para unirme a ti.";

    case Currency_Cats:
    default:
        switch (traits.npcType)
        {
        case OT_BANDIT:
            return "Todos tenemos un precio. El m\xc3\xado se paga en cats.";
        case OT_TRADER:
            return "Hablemos de negocios. Yo no trabajo gratis.";
        case OT_SLAVER:
            return "\xc2\xbfMe quieres? C\xc3\xb3mprame, igual que yo te comprar\xc3\xad" "a.";
        default:
            return (traits.personality == PT_FEARFUL)
                       ? "Solo quiero salir de aqu\xc3\xad. \xc2\xbfMe haces que valga la pena?"
                       : "Sigo a quien pague. \xc2\xbf" "Est\xc3\xa1" "s pagando?";
        }
    }
}

// The window's own voice. Short, because the window has one line for it, and
// factual, because the NPC's own words are already on screen in their bubble.
std::string DescribeOffer(Character* character, const Verdict& verdict)
{
    const std::string name = CharacterName(character);

    if (!verdict.willJoin)
    {
        // A refusal only reaches the window when something can be done about
        // it, so the reason is the useful half.
        return name + " no se une todav\xc3\xad" "a: " + verdict.reason + ".";
    }

    switch (verdict.currency)
    {
    case Currency_Cats:
    {
        std::ostringstream text;
        text << name << " se unir\xc3\xa1 a tu grupo por " << verdict.cats << " cats.";
        return text.str();
    }

    case Currency_Freedom:
        return name + " se unir\xc3\xa1 a tu grupo si lo liberas.";

    case Currency_Submission:
        return name + " ya no tiene voluntad de pelear y desea unirse a tu grupo.";

    case Currency_Care:
        return name + " te debe la comida y las curas, y desea unirse a tu grupo.";

    case Currency_None:
    default:
        return verdict.viaRescue
                   ? name + " te debe la vida y desea unirse a tu grupo."
                   : name + " desea unirse a tu grupo.";
    }
}

// Several lines per personality so a prisoner shouting twice does not repeat
// himself word for word. Kept short: this is a bubble the player reads in
// passing, not a speech.
std::string HailLine(Character* character)
{
    Traits traits;
    if (character == NULL || !SafeReadTraits(character, &traits))
    {
        return "\xc2\xa1" "Eh! Hablemos.";
    }

    // Cheap rotation off the pointer, so the same NPC keeps his own flavour of
    // line while different ones in a row do not all say the same thing.
    const unsigned int pick =
        (unsigned int)((reinterpret_cast<size_t>(character) >> 4) % 3);

    switch (traits.personality)
    {
    case PT_FEARFUL:
        if (pick == 0) return "\xc2\xa1" "Espera! Podemos hablar, \xc2\xbfs\xc3\xad?";
        if (pick == 1) return "No me dejes aqu\xc3\xad... escucha un momento.";
        return "Por favor. Solo escucha lo que tengo que decir.";

    case PT_HONORABLE:
        if (pick == 0) return "Est\xc3\xa1 bien. Te escucho.";
        if (pick == 1) return "Has ganado. Habla y te oir\xc3\xa9.";
        return "Ya no tengo nada que probar. Di lo tuyo.";

    case PT_MANIACAL:
        if (pick == 0) return "\xc2\xa1" "Ja! Ven aqu\xc3\xad, quiero hablar contigo.";
        if (pick == 1) return "\xc2\xa1" "Eh, t\xc3\xba! S\xc3\xad, t\xc3\xba. Acerca" "te.";
        return "Esto se puso interesante. Hablemos.";

    case PT_COLD_CRUEL:
        if (pick == 0) return "Tenemos algo que discutir. Ven.";
        if (pick == 1) return "Acerca" "te. No voy a gritarlo.";
        return "Hablemos, antes de que cambie de idea.";

    case PT_BRAVE:
        if (pick == 0) return "\xc2\xa1" "Eh! Ven, quiero hablar.";
        if (pick == 1) return "No me voy a quedar callado. Hablemos.";
        return "Ven aqu\xc3\xad. Tengo algo que proponerte.";

    default:
        if (pick == 0) return "\xc2\xa1" "Eh! Hablemos.";
        if (pick == 1) return "Oye... ven un momento.";
        return "\xc2\xbf" "Tienes un momento? Quiero hablar.";
    }
}

std::string SpeakAcceptLine(const Verdict& verdict)
{
    // Short enough to fit the button. What the player is agreeing to is spelled
    // out in the window's body, so the button only has to say yes.
    return "ACEPTAR";
}

bool IsInPlayerCage(Character* character)
{
    if (character == NULL)
    {
        return false;
    }

    bool inPrison = false;
    if (!SafeInPrison(character, &inPrison) || !inPrison)
    {
        return false;
    }

    Faction* playerFaction = NULL;
    if (!SafePlayerFaction(&playerFaction))
    {
        return false;
    }

    // Held in an actual cage: the cage's owner decides whose prisoner they are.
    Faction* cageFaction = NULL;
    if (SafeCageFaction(character, &cageFaction))
    {
        return cageFaction == playerFaction;
    }

    // Shackled or carried off, with no cage object to ask. The engine still
    // knows who owns them, so fall back to that -- otherwise a prisoner dragged
    // home would be unreachable for having been put down in the wrong spot.
    Character* leader = NULL;
    if (SafePlayerLeader(&leader))
    {
        bool mine = false;
        if (SafeIsMyPrisoner(leader, character, &mine) && mine)
        {
            return true;
        }
    }

    return false;
}

bool IsGenuinelyDying(Character* character)
{
    if (character == NULL)
    {
        return false;
    }

    Condition condition;
    if (!SafeReadCondition(character, &condition))
    {
        return false;
    }

    bool pretending = false;
    SafeIsPretending(character, &pretending);

    return condition.dying && !pretending;
}

void MarkRescuedBy(Character* character, Character* saviour)
{
    if (character == NULL || saviour == NULL)
    {
        return;
    }

    // The engine stores this per-NPC and saves it with the game, so the debt
    // survives even if the player walks away before the NPC wakes up.
    if (SafeRememberTag(character, saviour, LT_MY_LIFESAVER))
    {
        LogInfo("Marked " + CharacterName(character) + " as rescued.");
    }
}

bool RemembersPlayerAsSaviour(Character* character)
{
    if (character == NULL)
    {
        return false;
    }

    Character* leader = NULL;
    if (!SafePlayerLeader(&leader))
    {
        return false;
    }

    bool remembered = false;
    if (!SafeMemoryTag(character, leader, LT_MY_LIFESAVER, &remembered))
    {
        return false;
    }
    return remembered;
}

Verdict Evaluate(Character* character)
{
    Verdict verdict;

    if (character == NULL)
    {
        verdict.reason = "no hay nadie ah\xc3\xad";
        return verdict;
    }

    const Config& config = GetConfig();

    // Cheat preset keeps the original behaviour: no rules at all.
    if (config.preset == Preset_Cheat)
    {
        verdict.willJoin = true;
        verdict.reason = "se une a ti";
        return verdict;
    }

    Faction* playerFaction = NULL;
    Character* leader = NULL;
    SafePlayerFaction(&playerFaction);
    SafePlayerLeader(&leader);

    bool animal = false;
    SafeIsAnimal(character, &animal);
    verdict.isAnimal = animal;
    if (animal && !config.animalsEnabled)
    {
        verdict.reason = "los animales est\xc3\xa1n desactivados";
        return verdict;
    }

    Traits traits;
    if (!SafeReadTraits(character, &traits))
    {
        // Without personality and archetype there is nothing to price against.
        // Refusing outright would break the very NPCs this mod exists to cover,
        // so fall back to the plain recruit instead.
        verdict.willJoin = true;
        verdict.reason = "se une a ti";
        return verdict;
    }

    Condition condition;
    SafeReadCondition(character, &condition);

    // Health only means something while they are conscious: getOverallHealthRating
    // reads 0 for anyone knocked out, so testing it on a KO'd character would
    // call them gravely wounded no matter how intact they are.
    condition.wounded = condition.unconscious
                            ? condition.dying
                            : (condition.health <= config.prices.woundedThreshold);

    // The raw hunger field is NOT a 0..1 fraction -- a healthy NPC reads about
    // 3 -- so the engine's own isReallyHungry decides, and this is only an
    // opt-in extra trigger for players who raise the threshold deliberately.
    if (config.prices.hungerThreshold > 0.0f &&
        condition.hunger >= config.prices.hungerThreshold)
    {
        condition.starving = true;
    }
    
    verdict.wantsFood = condition.starving;

    bool pretending = false;
    SafeIsPretending(character, &pretending);
    if (pretending)
    {
        verdict.reason = "se est\xc3\xa1 haciendo el muerto";
        return verdict;
    }

    bool corpse = false;
    if (SafeIsDead(character, &corpse) && corpse)
    {
        verdict.reason = "est\xc3\xa1 muerto";
        return verdict;
    }

    // Nobody agrees to anything while they are out cold. A deal struck with an
    // unconscious body is not a deal, and it undercuts the rescue route, whose
    // whole point is that the offer arrives when they wake and find out who
    // saved them.
    if (condition.unconscious)
    {
        LogCondition(character, condition, "unconscious");
        if (config.rescueEnabled && RemembersPlayerAsSaviour(character))
        {
            verdict.viaRescue = true;
            verdict.reason = "inconsciente: te lo deber\xc3\xa1 al despertar";
        }
        else if (condition.dying)
        {
            verdict.viaRescue = true;
            verdict.currency = Currency_Care;
            verdict.reason = "se muere y no reacciona: c\xc3\xbaralo y espera";
        }
        else
        {
            verdict.reason = "inconsciente: espera a que despierte";
        }
        return verdict;
    }

    // The one honest no. Unlike faction hatred, this is a reason the player
    // understands and could have avoided.
    if (leader != NULL)
    {
        bool grudge = false;
        if (SafeMemoryTag(character, leader, LT_KILLED_MY_FRIEND, &grudge) && grudge)
        {
            verdict.currency = Currency_Refused;
            verdict.reason = "se niega: mataste a alguien suyo";
            return verdict;
        }
    }

    // Route A -- rescue. Someone you pulled out of the dirt owes you nothing
    // else. Checked first because it outranks every price.
    if (config.rescueEnabled && RemembersPlayerAsSaviour(character))
    {
        verdict.willJoin = true;
        verdict.viaRescue = true;
        verdict.currency = Currency_None;
        verdict.reason = "te debe la vida";
        return verdict;
    }

    // Still dying and not yet saved: the route is open, but the price is care.
    if (config.rescueEnabled && condition.dying)
    {
        verdict.viaRescue = true;
        verdict.currency = Currency_Care;
        verdict.reason = "se est\xc3\xa1 muriendo: c\xc3\xbaralo primero";
        return verdict;
    }

    // Easy mode, the healing half: someone patched all the way back up owes the
    // player whether or not the engine happened to credit the first aid. The
    // applyFirstAid hook only fires while the player's own medic is working, so
    // a prisoner healed by a bed or by time would otherwise slip through.
    if (config.rescueEnabled && config.prices.easyHoursCaged > 0.0f &&
        !condition.wounded && !condition.starving && condition.health >= 1.0f &&
        IsInPlayerCage(character))
    {
        verdict.willJoin = true;
        verdict.viaRescue = true;
        verdict.currency = Currency_Care;
        verdict.reason = "sano gracias a ti, y quiere unirse";
        return verdict;
    }

    // Route C -- surrender. Someone you beat, still on their feet but with the
    // fight gone, can yield rather than be cut down.
    if (config.surrenderEnabled && !animal && IsSurrendered(character))
    {
        verdict.willJoin = true;
        verdict.currency = Currency_Submission;
        verdict.reason = "se rinde antes que morir aqu\xc3\xad";
        return verdict;
    }

    // Nivel relativo del NPC vs l\xc3\xadder del jugador
    int npcLevel = 1;
    SafeRoughLevel(character, &npcLevel);
    int playerLevel = 1;
    if (leader != NULL)
    {
        SafeRoughLevel(leader, &playerLevel);
    }

    bool inAnyCage = false;
    const bool inMyCage = IsInPlayerCage(character);
    if (!inMyCage)
    {
        SafeInPrison(character, &inAnyCage);
        if (inAnyCage)
        {
            verdict.reason = "est\xc3\xa1 en la jaula de otro";
            return verdict;
        }

        // Insistence check: si el jugador ha insistido demasiado recientemente
        double remainingHours = 0.0;
        if (IsIgnoringPlayer(character, &remainingHours))
        {
            verdict.currency = Currency_Refused;
            verdict.reason = "est\xc3\xa1 harto de tu insistencia: d\xc3\xa9jalo en paz un rato";
            return verdict;
        }

        // Faction relation, read once and used for everything below: who will
        // talk, who will take money, and how they open. It is still never a
        // flat gate for people who like you -- only for the ones who do not.
        float relation = 0.0f;
        const bool haveRelation = SafeFactionRelation(character, playerFaction, &relation);
        const std::string band = haveRelation ? RelationBand(relation) : "neutral";
        verdict.band = band;

        const bool goodNatured = (traits.personality == PT_HONORABLE ||
                                  traits.personality == PT_WARM_KIND);

        // Allies. The player already knows they are allies -- the cursor goes
        // yellow -- so refusing outright reads as a bug, not as a rule.
        if (haveRelation && relation >= 25.0f)
        {
            // Cheat and Balanced-with-easy-mode: being on the same side is
            // enough on its own.
            if (config.preset == Preset_Cheat)
            {
                verdict.willJoin = true;
                verdict.currency = Currency_None;
                verdict.reason = Say("faction." + band + ".reason",
                                     "sois aliados: se une a ti", character);
                return verdict;
            }

            const int chance = AllyJoinChance(npcLevel, playerLevel);
            if (StableChance(character, chance))
            {
                verdict.willJoin = true;
                verdict.currency = Currency_None;
                verdict.reason = Say("faction." + band + ".reason",
                                     "os lleva\xc3\xads bien: se une a ti", character);
                return verdict;
            }

            // Not a refusal to be argued with -- a "not you, not yet", which is
            // what a level gap actually means between allies.
            verdict.currency = Currency_Refused;
            verdict.reason = Say("faction." + band + ".reasonMaybe",
                                 "te aprecia, pero te mide: hazte m\xc3\xa1s fuerte",
                                 character);
            return verdict;
        }

        // Below -15 an honourable or kind NPC has nothing to discuss. Their
        // objection is who you are, and no amount of money answers that.
        if (haveRelation && relation < -15.0f && goodNatured)
        {
            verdict.currency = Currency_Refused;
            verdict.reason = Say("faction." + band + ".reasonHonourable",
                                 "su honor no le deja tratar con alguien de tu reputaci\xc3\xb3n",
                                 character);
            return verdict;
        }

        // Below -30 even the greedy stop listening. Money works because the
        // seller expects to live to spend it; past open war that stops holding.
        if (haveRelation && relation < -30.0f)
        {
            verdict.currency = Currency_Refused;
            verdict.reason = Say("faction.enemigo.reasonGreedy",
                                 "ni por dinero: te odia demasiado", character);
            return verdict;
        }

        // NPC libre: Flujo normal de soborno y negociacion
        // 1. Si el nivel del NPC es muy superior (+15 niveles), ni te escucha (desprecio por debilidad)
        if (npcLevel - playerLevel > 15)
        {
            verdict.currency = Currency_Refused;
            verdict.reason = "eres demasiado debil comparado con el: ni se molesta en escucharte";
            return verdict;
        }

        // 2. Personalidades honorables o buenas nunca aceptan Cats
        if (traits.personality == PT_HONORABLE || traits.personality == PT_WARM_KIND)
        {
            if (condition.starving)
            {
                verdict.currency = Currency_Care;
                verdict.reason = "tiene hambre y aceptaria comida";
            }
            else
            {
                verdict.currency = Currency_Refused;
                verdict.reason = "su honor le impide aceptar sobornos: gana su respeto, "
                                 "o enc\xc3\xa1" "rcelalo y som\xc3\xa9telo";
            }
            return verdict;
        }

        // 3. Si no tiene personalidad corruptible/sobornable (ej. militares en servicio libre)
        if (traits.npcType == OT_MILITARY || traits.npcType == OT_LAW_ENFORCEMENT)
        {
            verdict.currency = Currency_Refused;
            // A reason is only worth showing if it names the way out. Two
            // exist here, so both are offered rather than only the one the
            // mod happens to prefer.
            verdict.reason = "sirve con lealtad a su facci\xc3\xb3n: gana reputaci\xc3\xb3n, "
                             "o enc\xc3\xa1" "rcelalo y som\xc3\xa9telo";
            return verdict;
        }

        // 4. Acepta soborno dinámico
        verdict.currency = Currency_Cats;
        verdict.cats = PriceInCats(character, traits, playerFaction);

        int purse = 0;
        const bool havePurse = (leader != NULL) && SafePlayerMoney(leader, &purse);

        std::ostringstream text;
        if (havePurse && purse < verdict.cats)
        {
            text << "pide " << verdict.cats << " cats y tienes " << purse;
            verdict.reason = text.str();
        }
        else
        {
            verdict.willJoin = true;
            text << "se une por " << verdict.cats << " cats";
            verdict.reason = text.str();
        }
        return verdict;
    }

    if (!config.captivityEnabled)
    {
        verdict.reason = "el reclutamiento de prisioneros est\xc3\xa1 desactivado";
        return verdict;
    }

    verdict.viaCaptivity = true;

    bool unique = false;
    SafeIsUnique(character, &unique);

    // Easy mode: time alone breaks anyone. Whoever they are and whatever they
    // would normally charge, long enough in a cage is its own argument -- and
    // the player only has to wait, not work out what this particular NPC wants.
    if (config.prices.easyHoursCaged > 0.0f)
    {
        const float held = HoursHeld(character);
        if (held >= config.prices.easyHoursCaged)
        {
            verdict.willJoin = true;
            verdict.currency = Currency_Submission;
            verdict.reason = "lleva bastante encerrado y se rinde";
            return verdict;
        }
    }

    if (playerLevel - npcLevel >= 40)
    {
        verdict.willJoin = true;
        verdict.currency = Currency_Submission;
        verdict.reason = "intimidado por tu inmenso poder, se somete sin dudarlo";
        return verdict;
    }

    verdict.currency = ChooseCurrency(traits, condition, animal);

    // A named character overrides the ordinary table, because for them the
    // currency IS the story. Someone starving or bleeding still gets care
    // first -- a leader in that state is a person before they are a symbol.
    if (unique && !animal && !condition.starving && !condition.wounded)
    {
        verdict.currency = UniqueTakesMoney(traits) ? Currency_Cats
                                                    : Currency_Submission;
    }

    switch (verdict.currency)
    {
    case Currency_Freedom:
    {
        // Someone you freed from slavers does not need convincing. The cheapest
        // route in the mod, and the most Kenshi.
        verdict.willJoin = true;
        verdict.reason = "seguir\xc3\xa1 a quien lo liber\xc3\xb3";
        break;
    }

    case Currency_Care:
    {
        // An animal that was never hurt or hungry has no reason to bond with
        // you; taming is something you do for a creature in need.
        if (animal && !condition.starving && !condition.wounded &&
            !RemembersPlayerAsSaviour(character))
        {
            verdict.reason = "sano y desconfiado: b\xc3\xbascalo herido o hambriento";
            break;
        }

        const bool cared = !condition.starving && !condition.wounded;

        // Animals earn their own arithmetic. A cub follows the first person who
        // feeds it; a grown one wants to see that you can keep doing it.
        if (animal && condition.starving)
        {
            float maturity = 0.5f;
            SafeAnimalMaturity(character, &maturity);
            const int meals = MealsToTame(npcLevel, playerLevel, maturity);

            verdict.currency = Currency_Care;
            verdict.wantsFood = true;
            verdict.cats = meals;   // meals, not money: the window reads it as the price

            if (meals <= 1)
            {
                verdict.reason = (maturity < 0.25f)
                    ? "cachorro hambriento: una comida y te sigue"
                    : "d\xc3\xa9" "bil y hambriento: una comida basta";
            }
            else
            {
                char buf[96] = { 0 };
                _snprintf_s(buf, sizeof(buf), _TRUNCATE,
                            "hambriento pero cr\xc3\xa9" "dulo: %d comidas y te seguir\xc3\xa1",
                            meals);
                verdict.reason = buf;
            }
            break;
        }

        if (cared)
        {
            verdict.willJoin = true;
            verdict.reason = animal ? "alimentado y curado: conf\xc3\xad" "a en ti"
                                    : "alimentado y curado: se unir\xc3\xa1";
        }
        else if (condition.starving && condition.wounded)
        {
            verdict.reason = "hambriento y herido: dale comida y c\xc3\xbaralo";
        }
        else if (condition.starving)
        {
            verdict.reason = "hambriento: dale de comer";
        }
        else
        {
            verdict.reason = "herido: c\xc3\xbaralo";
        }
        break;
    }

    case Currency_Submission:
    {
        // Being in your cage already proves you beat them -- you cannot cage
        // someone you did not knock down. What the proud still owe is being
        // worn down, and that is read from their condition rather than a clock:
        // hunger and wounds are both things the player can hasten or relieve.
        const bool wornDown = condition.starving || condition.wounded;

        if (wornDown)
        {
            verdict.willJoin = true;
            verdict.reason = "quebrado y sin orgullo: te seguir\xc3\xa1";
        }
        else if (unique)
        {
            // No price is coming for this one. Their objection is what they
            // believe, and belief does not have an exchange rate -- only time
            // in a cage and the body's own arguments.
            verdict.reason = Say("unique.zealot",
                                 "no hay dinero que compre lo que cree: "
                                 "el hambre y las heridas harán el trabajo",
                                 character);
        }
        else
        {
            verdict.reason = "sigue desafiante: d\xc3\xa9jalo con hambre o h\xc3\xa9relo";
        }
        break;
    }

    case Currency_Cats:
    default:
    {
        verdict.cats = PriceInCats(character, traits, playerFaction);

        int purse = 0;
        const bool havePurse = (leader != NULL) && SafePlayerMoney(leader, &purse);

        std::ostringstream text;
        if (havePurse && purse < verdict.cats)
        {
            text << "pide " << verdict.cats << " cats y tienes " << purse;
            verdict.reason = text.str();
        }
        else
        {
            verdict.willJoin = true;
            text << "se une por " << verdict.cats << " cats";
            verdict.reason = text.str();
        }
        break;
    }
    }

    return verdict;
}

bool IsIgnoringPlayer(Character* character, double* outRemainingHours)
{
    if (character == NULL)
    {
        return false;
    }

    double now = 0.0;
    if (!SafeGameHours(&now))
    {
        return false;
    }

    std::map<Character*, TalkState>::iterator it = g_talkStates.find(character);
    if (it == g_talkStates.end())
    {
        return false;
    }

    if (it->second.ignoredUntilHour > now)
    {
        if (outRemainingHours != NULL)
        {
            *outRemainingHours = it->second.ignoredUntilHour - now;
        }
        return true;
    }

    return false;
}

bool RecordInteraction(Character* character, double nowHours)
{
    if (character == NULL)
    {
        return false;
    }

    TalkState& state = g_talkStates[character];
    if (state.ignoredUntilHour > nowHours)
    {
        return true;
    }

    // Si pasaron mas de 2 horas de juego desde el ultimo intento, resetea el contador
    if (nowHours - state.lastHour > 2.0)
    {
        state.attempts = 0;
    }

    state.lastHour = nowHours;
    state.attempts++;

    // 3 intentos seguidos sin exito provocan que ignore al jugador por 4 horas
    if (state.attempts >= 3)
    {
        state.ignoredUntilHour = nowHours + 4.0;
        return true;
    }

    return false;
}

void SafePacify(Character* character, Character* playerLeader)
{
    if (character == NULL)
    {
        return;
    }

    TR_TRY
    {
        character->endCombatMode();
        if (playerLeader != NULL)
        {
            character->clearTempEnemyStatus(playerLeader);
        }
        character->reThinkCurrentAIAction();
    }
    TR_CATCH { }
}
} // namespace totalrecruit
