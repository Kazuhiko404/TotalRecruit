#include "common.h"
#include "probe.h"
#include "recruit_core.h"
#include "engine_reads.h"
#include "log.h"

#include <sstream>

namespace totalrecruit
{
namespace
{
// Every accessor below reads through a pointer the engine may already have
// freed, so each one is guarded. The guards live in their own POD-only frames
// because C2712 counts every C++ object in the whole function, not just the
// ones inside the __try block.

bool SafeStateBroadcast(Character* character, StateBroadcastData** out)
{
    TR_TRY
    {
        *out = character->getStateBroadcast();
        return (*out != NULL);
    }
    TR_CATCH { return false; }
}

// What the AI currently intends, and the long-term marks it remembers about the
// player. Read together because the open question (I4) is whether writing one
// of those marks changes any of these -- the engine reads the same memory the
// mod would be writing to, and nothing in the headers says what it does with it.
//
// Logged on every probe so a before-and-after comparison needs no special mode:
// click an NPC, apply the mark, click again, diff the two lines.
bool SafeReadAggression(StateBroadcastData* broadcast, float* outIntended,
                        float* outTowardsTarget, float* outDistress,
                        int* outIntendsToAttack, int* outTempNeutral)
{
    TR_TRY
    {
        *outIntended         = broadcast->intendedAggression;
        *outTowardsTarget    = broadcast->aggressionTowardsTarget;
        *outDistress         = broadcast->distress;
        *outIntendsToAttack  = (broadcast->intendsToAttack.getCharacter() != NULL) ? 1 : 0;
        *outTempNeutral      = broadcast->isTemporaryNeutral ? 1 : 0;
        return true;
    }
    TR_CATCH { return false; }
}

// The engine's own per-character memory about the player. These persist in the
// save, and the AI consults them: LT_MY_CAPTOR and LT_KILLED_MY_FRIEND plainly
// mean something to it, which is exactly why writing LT_DEFEATED_MY_SQUAD_ONCE
// for the mod's own bookkeeping needs proving before it is trusted.
bool SafeReadMemoryTags(Character* character, Character* player, int* out, int count)
{
    TR_TRY
    {
        static const CharacterPerceptionTags_LongTerm kTags[] = {
            LT_MY_LIFESAVER, LT_MY_CAPTOR, LT_FREED_ME,
            LT_DEFEATED_MY_SQUAD_ONCE, LT_SQUAD_LOST_TO_ME_ONCE,
            LT_KILLED_MY_FRIEND, LT_I_SCREWED_THIS_GUY, LT_STOLE_FROM_ME
        };
        for (int i = 0; i < count; ++i)
        {
            out[i] = character->getCharacterMemoryTag(player, kTags[i]) ? 1 : 0;
        }
        return true;
    }
    TR_CATCH { return false; }
}

bool SafeReadBroadcastFields(StateBroadcastData* broadcast, int* outNpcType,
                             int* outPersonality, int* outLooksLike,
                             int* outSlaveState, int* outEscaped, int* outKidnapped)
{
    TR_TRY
    {
        *outNpcType     = (int)broadcast->NPCType;
        *outPersonality = (int)broadcast->myPersonality;
        *outLooksLike   = (int)broadcast->looksLike_clothingArchetype;
        *outSlaveState  = (int)broadcast->_slaveState;
        *outEscaped     = broadcast->isEscapedPrisoner ? 1 : 0;
        *outKidnapped   = broadcast->isKidnapped ? 1 : 0;
        return true;
    }
    TR_CATCH { return false; }
}

bool SafeMedical(Character* character, MedicalSystem** out)
{
    TR_TRY
    {
        *out = character->getMedical();
        return (*out != NULL);
    }
    TR_CATCH { return false; }
}

// isProbablyDying is the engine's own dying-vs-KO predicate; the rescue route
// depends on it existing and being callable from a plugin.
bool SafeReadMedicalFields(MedicalSystem* medical, int* outDying, int* outUnconscious,
                           int* outBloodloss, int* outHungerKO, int* outReallyHungry,
                           float* outBlood, float* outHunger, float* outHealth)
{
    TR_TRY
    {
        *outDying        = medical->isProbablyDying() ? 1 : 0;
        *outUnconscious  = medical->isUnconcious() ? 1 : 0;
        *outBloodloss    = medical->isInBloodlossTrauma() ? 1 : 0;
        *outHungerKO     = medical->isHungerKO() ? 1 : 0;
        *outReallyHungry = medical->isReallyHungry() ? 1 : 0;
        *outBlood        = medical->blood;
        *outHunger       = medical->hunger;
        *outHealth       = medical->getOverallHealthRating();
        return true;
    }
    TR_CATCH { return false; }
}

// PS_PLAYING_DEAD exists, so "unconscious" alone would let the player farm
// recruits off NPCs that are only pretending.
bool SafeReadCharacterFlags(Character* character, int* outDown, int* outReallyUnconscious,
                            int* outUnique, int* outAnimal, int* outRoughLevel,
                            int* outProneState, int* outInSomething,
                            int* outChained, int* outKidnapped)
{
    TR_TRY
    {
        *outDown               = character->isDown() ? 1 : 0;
        *outReallyUnconscious  = character->isLiterallyUnconciousNotPretending() ? 1 : 0;
        *outUnique             = character->isUnique() ? 1 : 0;
        *outAnimal             = (character->isAnimal() != NULL) ? 1 : 0;
        *outRoughLevel         = character->getRoughLevel();
        *outProneState         = (int)character->getProneState();
        *outInSomething        = (int)character->inSomething;
        *outChained            = character->isChainedMode() ? 1 : 0;
        *outKidnapped          = character->isKidnapped() ? 1 : 0;
        return true;
    }
    TR_CATCH { return false; }
}

// "Is this prisoner in MY cage" has no direct accessor; it is derived by
// resolving the cage handle and comparing its owning faction to the player's.
// getName returns std::string by value, so the call and the copy out of it both
// live in a helper the guard only jumps through -- a c_str() from the temporary
// would dangle the moment it returned.
void CopyCageName(RootObjectBase* cage, char* buffer, size_t size)
{
    strncpy_s(buffer, size, cage->getName().c_str(), _TRUNCATE);
}

// Resolved as RootObjectBase rather than Building: getFaction and getName both
// live on the base, and including Building.h would collide with Platoon.h over
// BuildingDesignation, which each defines identically and neither guards.
bool SafeReadCage(Character* character, Faction** outCageFaction,
                  char* nameBuffer, size_t nameSize)
{
    TR_TRY
    {
        RootObjectBase* cage = character->inWhat.getRootObjectBase();
        if (cage == NULL)
        {
            return false;
        }
        *outCageFaction = cage->getFaction();
        CopyCageName(cage, nameBuffer, nameSize);
        return true;
    }
    TR_CATCH { return false; }
}

bool SafeHasDialogue(Character* character, int* outHasDialogue, int* outTalkToMe,
                     int* outGetUpPeace, int* outHealedFinished)
{
    TR_TRY
    {
        if (character->dialogue == NULL)
        {
            return false;
        }
        *outHasDialogue     = character->hasDialogue() ? 1 : 0;
        *outTalkToMe        = character->dialogue->hasDialogueEvent(EV_PLAYER_TALK_TO_ME) ? 1 : 0;
        *outGetUpPeace      = character->dialogue->hasDialogueEvent(EV_GET_UP_PEACE) ? 1 : 0;
        *outHealedFinished  = character->dialogue->hasDialogueEvent(EV_BEING_HEALED_FINISHED) ? 1 : 0;
        return true;
    }
    TR_CATCH { return false; }
}

bool SafeFactionName(Faction* faction, const char** out)
{
    TR_TRY
    {
        *out = faction->getName().c_str();
        return true;
    }
    TR_CATCH { return false; }
}

// Which faction the character themselves belongs to -- distinct from the cage's
// owner, which is all this probe used to report. Wild fauna and someone's herd
// animal are both animals; the difference has to live somewhere, and faction is
// where Kenshi models belonging.
bool SafeOwnFaction(Character* character, Faction** out)
{
    TR_TRY
    {
        *out = character->getFaction();
        return (*out != NULL);
    }
    TR_CATCH { return false; }
}

// isNotARealFaction is the engine's own marker for the bookkeeping factions it
// uses for things that are not organisations -- which is where loose wildlife
// is expected to sit. Expected, not proven: this logs it so a click on a wild
// beast and a click on a tame one can settle it with evidence.
bool SafeFactionFlags(Faction* faction, int* outNotReal, int* outIsPlayer,
                      int* outFundamentalType, int* outSize)
{
    TR_TRY
    {
        *outNotReal          = faction->isNotARealFaction() ? 1 : 0;
        *outIsPlayer         = faction->isThePlayer() ? 1 : 0;
        *outFundamentalType  = (int)faction->getFundamentalNPCType();
        *outSize             = faction->getFactionSize();
        return true;
    }
    TR_CATCH { return false; }
}

const char* NpcTypeName(int value)
{
    switch (value)
    {
    case OT_NONE:            return "OT_NONE";
    case OT_LAW_ENFORCEMENT: return "OT_LAW_ENFORCEMENT";
    case OT_MILITARY:        return "OT_MILITARY";
    case OT_TRADER:          return "OT_TRADER";
    case OT_CIVILIAN:        return "OT_CIVILIAN";
    case OT_DIPLOMAT:        return "OT_DIPLOMAT";
    case OT_SLAVE:           return "OT_SLAVE";
    case OT_SLAVER:          return "OT_SLAVER";
    case OT_BANDIT:          return "OT_BANDIT";
    case OT_ADVENTURER:      return "OT_ADVENTURER";
    default:                 return "OT_<unknown>";
    }
}

// Four ordinals are unnamed in the reversed enum, so they are reported by
// number rather than guessed at.
const char* PersonalityName(int value)
{
    switch (value)
    {
    case PT_NONE:       return "PT_NONE";
    case PT_HONORABLE:  return "PT_HONORABLE";
    case PT_TRAITOROUS: return "PT_TRAITOROUS";
    case PT_SMART:      return "PT_SMART";
    case PT_DUMB:       return "PT_DUMB";
    case PT_BRAVE:      return "PT_BRAVE";
    case PT_FEARFUL:    return "PT_FEARFUL";
    case PT_WARM_KIND:  return "PT_WARM_KIND";
    case PT_COLD_CRUEL: return "PT_COLD_CRUEL";
    case PT_NORMAL:     return "PT_NORMAL";
    case PT_MANIACAL:   return "PT_MANIACAL";
    default:            return "PT_<unnamed ordinal>";
    }
}

const char* SlaveStateName(int value)
{
    switch (value)
    {
    case NOT_SLAVE:      return "NOT_SLAVE";
    case IS_SLAVE:       return "IS_SLAVE";
    case ESCAPING_SLAVE: return "ESCAPING_SLAVE";
    case EX_SLAVE:       return "EX_SLAVE";
    default:             return "<unknown>";
    }
}

const char* ProneStateName(int value)
{
    switch (value)
    {
    case PS_NORMAL:       return "PS_NORMAL";
    case PS_STAYING_LOW:  return "PS_STAYING_LOW";
    case PS_CRIPPLED:     return "PS_CRIPPLED";
    case PS_PLAYING_DEAD: return "PS_PLAYING_DEAD";
    case PS_KO:           return "PS_KO";
    default:              return "<unknown>";
    }
}

const char* UseStuffStateName(int value)
{
    switch (value)
    {
    case IN_NOTHING: return "IN_NOTHING";
    case IN_BED:     return "IN_BED";
    case IN_PRISON:  return "IN_PRISON";
    default:         return "<unknown>";
    }
}

const char* YesNo(int value)
{
    return value ? "yes" : "no";
}
} // namespace

std::string ProbeCharacter(Character* character)
{
    if (character == NULL)
    {
        return "probe: no character";
    }

    const std::string name = CharacterName(character);
    LogInfo("--- probe: " + name + " ---");

    std::ostringstream summary;
    summary << name;

    // 1. Personality and archetype. These are the fields that make the rule
    //    engine work on NPCs from mods that do not exist yet, so they are the
    //    single most important thing to confirm is populated.
    StateBroadcastData* broadcast = NULL;
    if (SafeStateBroadcast(character, &broadcast))
    {
        int npcType = 0, personality = 0, looksLike = 0, slaveState = 0, escaped = 0, kidnapped = 0;
        if (SafeReadBroadcastFields(broadcast, &npcType, &personality, &looksLike,
                                    &slaveState, &escaped, &kidnapped))
        {
            std::ostringstream line;
            line << "  NPCType=" << NpcTypeName(npcType) << " (" << npcType << ")"
                 << "  personality=" << PersonalityName(personality) << " (" << personality << ")"
                 << "  looksLike=" << NpcTypeName(looksLike)
                 << "  slaveState=" << SlaveStateName(slaveState)
                 << "  escapedPrisoner=" << YesNo(escaped)
                 << "  kidnapped=" << YesNo(kidnapped);
            LogInfo(line.str());

            summary << " | " << NpcTypeName(npcType) << " / " << PersonalityName(personality);
        }
        else
        {
            LogError("  stateBroadcast fields unreadable");
            summary << " | broadcast unreadable";
        }
    }
    else
    {
        LogError("  getStateBroadcast() returned null or faulted");
        summary << " | no stateBroadcast";
    }

    // 2. Dying vs merely knocked out. isProbablyDying is the rescue route's
    //    trigger; the pretending check keeps PS_PLAYING_DEAD NPCs out of it.
    MedicalSystem* medical = NULL;
    if (SafeMedical(character, &medical))
    {
        int dying = 0, unconscious = 0, bloodloss = 0, hungerKO = 0, reallyHungry = 0;
        float blood = 0.0f, hunger = 0.0f, health = 0.0f;
        if (SafeReadMedicalFields(medical, &dying, &unconscious, &bloodloss, &hungerKO,
                                  &reallyHungry, &blood, &hunger, &health))
        {
            std::ostringstream line;
            line << "  dying=" << YesNo(dying)
                 << "  unconscious=" << YesNo(unconscious)
                 << "  bloodlossTrauma=" << YesNo(bloodloss)
                 << "  hungerKO=" << YesNo(hungerKO)
                 << "  reallyHungry=" << YesNo(reallyHungry)
                 << "  blood=" << blood
                 << "  hunger=" << hunger
                 << "  health=" << health;
            LogInfo(line.str());

            if (dying)
            {
                summary << " | DYING";
            }
        }
        else
        {
            LogError("  medical fields unreadable");
        }
    }
    else
    {
        LogError("  getMedical() returned null or faulted");
    }

    // 3. Character-level state, including the pretending check.
    int down = 0, reallyUnconscious = 0, unique = 0, animal = 0;
    int roughLevel = 0, proneState = 0, inSomething = 0, chained = 0, kidnapped = 0;
    if (SafeReadCharacterFlags(character, &down, &reallyUnconscious, &unique, &animal,
                               &roughLevel, &proneState, &inSomething, &chained, &kidnapped))
    {
        std::ostringstream line;
        line << "  isDown=" << YesNo(down)
             << "  reallyUnconscious=" << YesNo(reallyUnconscious)
             << "  proneState=" << ProneStateName(proneState)
             << "  isUnique=" << YesNo(unique)
             << "  isAnimal=" << YesNo(animal)
             << "  roughLevel=" << roughLevel
             << "  inSomething=" << UseStuffStateName(inSomething)
             << "  chained=" << YesNo(chained)
             << "  kidnapped=" << YesNo(kidnapped);
        LogInfo(line.str());

        // Own faction, logged so wild-versus-owned can be settled by clicking
        // one of each rather than by assuming how Kenshi models wildlife.
        Faction* ownFaction = NULL;
        if (SafeOwnFaction(character, &ownFaction))
        {
            const char* ownName = NULL;
            int notReal = 0, isPlayerFaction = 0, fundamental = 0, factionSize = 0;
            const bool haveName  = SafeFactionName(ownFaction, &ownName) && ownName != NULL;
            const bool haveFlags = SafeFactionFlags(ownFaction, &notReal, &isPlayerFaction,
                                                    &fundamental, &factionSize);
            std::ostringstream fline;
            fline << "  faction='" << (haveName ? ownName : "?") << "'";
            if (haveFlags)
            {
                fline << "  notARealFaction=" << YesNo(notReal)
                      << "  isPlayerFaction=" << YesNo(isPlayerFaction)
                      << "  fundamentalType=" << NpcTypeName(fundamental)
                      << "  factionSize=" << factionSize;
            }
            else
            {
                fline << "  (faction flags unreadable)";
            }
            LogInfo(fline.str());
        }
        else
        {
            LogInfo("  no faction");
        }

        // I4 -- the open investigation. These two lines are what a
        // before-and-after comparison needs: click an NPC, write the mark,
        // click again, and see whether the AI's intent moved.
        if (broadcast != NULL)
        {
            float intended = 0.0f, towards = 0.0f, distress = 0.0f;
            int intendsAttack = 0, tempNeutral = 0;
            if (SafeReadAggression(broadcast, &intended, &towards, &distress,
                                   &intendsAttack, &tempNeutral))
            {
                std::ostringstream aline;
                aline << "  intendedAggression=" << intended
                      << "  aggroTowardsTarget=" << towards
                      << "  distress=" << distress
                      << "  intendsToAttack=" << YesNo(intendsAttack)
                      << "  temporaryNeutral=" << YesNo(tempNeutral);
                LogInfo(aline.str());
            }
        }

        Character* playerLeader = NULL;
        if (SafePlayerLeader(&playerLeader) && playerLeader != NULL)
        {
            static const char* kTagNames[] = {
                "LIFESAVER", "MY_CAPTOR", "FREED_ME",
                "DEFEATED_MY_SQUAD", "SQUAD_LOST_TO_ME",
                "KILLED_MY_FRIEND", "I_SCREWED_HIM", "STOLE_FROM_ME"
            };
            const int tagCount = (int)(sizeof(kTagNames) / sizeof(kTagNames[0]));
            int tags[8] = { 0 };
            if (SafeReadMemoryTags(character, playerLeader, tags, tagCount))
            {
                std::ostringstream tline;
                tline << "  memoryTags:";
                bool any = false;
                for (int i = 0; i < tagCount; ++i)
                {
                    if (tags[i])
                    {
                        tline << " " << kTagNames[i];
                        any = true;
                    }
                }
                if (!any)
                {
                    tline << " (none)";
                }
                LogInfo(tline.str());
            }
        }

        if (unique)
        {
            summary << " | UNIQUE";
        }
        if (animal)
        {
            summary << " | animal";
        }
    }
    else
    {
        LogError("  character flags unreadable");
    }

    // 4. Cage ownership, derived rather than read: there is no getCaptor().
    if (inSomething == IN_PRISON || chained || kidnapped)
    {
        Faction* cageFaction = NULL;
        char cageName[128] = { 0 };
        Faction* playerFaction = NULL;
        const bool haveCage = SafeReadCage(character, &cageFaction, cageName, sizeof(cageName));
        const bool havePlayer = SafePlayerFaction(&playerFaction);

        if (haveCage)
        {
            const char* cageFactionName = NULL;
            if (cageFaction != NULL && SafeFactionName(cageFaction, &cageFactionName) &&
                cageFactionName != NULL)
            {
                const bool mine = (havePlayer && cageFaction == playerFaction);
                std::ostringstream line;
                line << "  cage='" << cageName << "'"
                     << "  cageFaction='" << cageFactionName << "'"
                     << "  isMyCage=" << YesNo(mine ? 1 : 0);
                LogInfo(line.str());

                summary << (mine ? " | IN MY CAGE" : " | in another cage");
            }
            else
            {
                LogInfo("  cage found but its faction is unreadable");
            }
        }
        else
        {
            LogInfo("  marked IN_PRISON but the cage handle did not resolve");
        }
    }

    // 5. Dialogue reachability. Whether these events are already present tells
    //    us if a runtime addDialoguePackage is needed before firing one.
    int hasDialogue = 0, talkToMe = 0, getUpPeace = 0, healedFinished = 0;
    if (SafeHasDialogue(character, &hasDialogue, &talkToMe, &getUpPeace, &healedFinished))
    {
        std::ostringstream line;
        line << "  hasDialogue=" << YesNo(hasDialogue)
             << "  EV_PLAYER_TALK_TO_ME=" << YesNo(talkToMe)
             << "  EV_GET_UP_PEACE=" << YesNo(getUpPeace)
             << "  EV_BEING_HEALED_FINISHED=" << YesNo(healedFinished);
        LogInfo(line.str());
    }
    else
    {
        LogInfo("  no dialogue component");
    }

    LogInfo("--- end probe ---");
    return summary.str();
}
} // namespace totalrecruit
