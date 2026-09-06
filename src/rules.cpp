#include "common.h"
#include "rules.h"
#include "config.h"
#include "recruit_core.h"
#include "surrender.h"
#include "log.h"

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
        *out = character->isDown() && !character->isLiterallyUnconciousNotPretending();
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

int PriceInCats(Character* character, const Traits& traits, Faction* playerFaction)
{
    const Prices& prices = GetConfig().prices;

    int level = 1;
    SafeRoughLevel(character, &level);
    if (level < 1)
    {
        level = 1;
    }

    float cats = (float)(level * prices.catsPerLevel);
    if (cats < (float)prices.catsMinimum)
    {
        cats = (float)prices.catsMinimum;
    }

    cats *= PersonalityMultiplier(traits.personality);
    cats *= FactionMultiplier(character, playerFaction);

    return (int)cats;
}

// Which currency does this archetype deal in? Condition takes precedence over
// archetype: a starving bandit wants food before he wants cats.
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
    Traits traits;
    if (character == NULL || !SafeReadTraits(character, &traits))
    {
        return "\xc2\xbfY bien? Habla de una vez.";
    }

    if (verdict.currency == Currency_Refused)
    {
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
        return "S\xc3\xa9 lo que hiciste. Fuera de mi vista.";
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

    // Past saving, judged by blood rather than by health: getOverallHealthRating
    // reads 0 for anyone merely knocked out, so testing health here declared
    // every KO'd character beyond saving and shut the whole mod down. Blood
    // going negative only happens to a body that has actually bled out.
    if (condition.blood < 0.0f)
    {
        LogCondition(character, condition, "blood<0");
        verdict.reason = "se desangr\xc3\xb3, ya no hay nada que hacer";
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
                verdict.reason = "su honor le impide aceptar sobornos de desconocidos";
            }
            return verdict;
        }

        // 3. Si no tiene personalidad corruptible/sobornable (ej. militares en servicio libre)
        if (traits.npcType == OT_MILITARY || traits.npcType == OT_LAW_ENFORCEMENT)
        {
            verdict.currency = Currency_Refused;
            verdict.reason = "sirve con lealtad a su faccion: capturalo primero";
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
            verdict.reason = "demasiado orgulloso: deja que el hambre y las heridas hagan lo suyo";
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
