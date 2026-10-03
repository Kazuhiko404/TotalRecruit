#include "common.h"
#include "squad_leave.h"
#include "engine_reads.h"
#include "log.h"

// AI/AITaskSystem.h (the task system) and not Tasker.h: the two define taskPriority,
// so one translation unit takes only one of them (npc_motion.cpp does the same). AI/AI.h
// cannot be included either (it redefines CharacterMessage against Character.h, notes
// S11), so the AI's task system is read at its documented offset.
#include <kenshi/AI/AITaskSystem.h>

namespace totalrecruit
{
namespace
{
// KEP's fallback when the home faction cannot be had (CharacterExtension.cpp:72).
const char* const kFallbackFactionId = "1233-gamedata.base";
// The vanilla "Wandering Squad" template KEP builds the new platoon from.
const char* const kSquadTemplateId = "5006-squad.mod";
// AI::taskSystemAI (AI/AI.h:115, "AITaskSytem* taskSystemAI; // 0x20").
const size_t kTaskSystemOffset = 0x20;

// -------------------------------------------------- Lookups (C2712: the strings
// are built outside the guards, which only take the const char* through).
Faction* FactionByIdUnguarded(const char* sid)
{
    return ou->factionMgr->getFactionByStringID(std::string(sid));
}

// A faction the character may go to: real, and not the player's.
Faction* SafeUsableFaction(const char* sid)
{
    if (sid == NULL || sid[0] == 0)
    {
        return NULL;
    }
    TR_TRY
    {
        if (ou == NULL || ou->factionMgr == NULL)
        {
            return NULL;
        }
        Faction* faction = FactionByIdUnguarded(sid);
        return (faction != NULL && faction->isPlayer == NULL) ? faction : NULL;
    }
    TR_CATCH { return NULL; }
}

Faction* SafeEmptyFaction()
{
    TR_TRY
    {
        if (ou == NULL || ou->factionMgr == NULL)
        {
            return NULL;
        }
        return ou->factionMgr->getEmptyFaction();
    }
    TR_CATCH { return NULL; }
}

GameData* TemplateUnguarded(const char* sid)
{
    return ou->gamedata.getData(std::string(sid));
}

GameData* SafeSquadTemplate()
{
    TR_TRY
    {
        if (ou == NULL)
        {
            return NULL;
        }
        return TemplateUnguarded(kSquadTemplateId);
    }
    TR_CATCH { return NULL; }
}

Faction* ResolveFaction(const std::string& homeFactionId, std::string* used)
{
    Faction* faction = SafeUsableFaction(homeFactionId.c_str());
    if (faction != NULL)
    {
        *used = homeFactionId;
        return faction;
    }
    faction = SafeUsableFaction(kFallbackFactionId);
    if (faction != NULL)
    {
        *used = kFallbackFactionId;
        return faction;
    }
    *used = "(empty faction)";
    return SafeEmptyFaction();
}

// ------------------------------------------------------------------ The steps
bool SafeUnselect(Character* member)
{
    TR_TRY
    {
        if (ou == NULL || ou->player == NULL)
        {
            return false;
        }
        if (ou->player->isObjectSelected(member))
        {
            ou->player->unselectPlayerCharacter(member);
        }
        return true;
    }
    TR_CATCH { return false; }
}

bool SafeSetFaction(Character* member, Faction* faction)
{
    TR_TRY
    {
        // Character's own setFaction (CharacterHuman / CharacterAnimal do not
        // override it), so the non-virtual thunk is the exact function.
        member->_NV_setFaction(faction, NULL);
        return true;
    }
    TR_CATCH { return false; }
}

// SEH frames hold PODs only (C2712): the position lives in this plain helper.
Platoon* NewPlatoonUnguarded(Character* member, Faction* faction, GameData* squadTemplate)
{
    const Ogre::Vector3 position = member->getPosition();
    return faction->createNewEmptyActivePlatoon(squadTemplate, true, position);
}

bool SafeJoinNewPlatoon(Character* member, Faction* faction, GameData* squadTemplate)
{
    TR_TRY
    {
        Platoon* platoon = NewPlatoonUnguarded(member, faction, squadTemplate);
        if (platoon == NULL || platoon->activePlatoon == NULL)
        {
            return false;
        }
        // An ActivePlatoon's own addActiveObject: the thunk is the exact function.
        return platoon->activePlatoon->_NV_addActiveObject(member);
    }
    TR_CATCH { return false; }
}

AITaskSytem* SafeTaskSystem(Character* member)
{
    TR_TRY
    {
        if (member->ai == NULL)
        {
            return NULL;
        }
        return *reinterpret_cast<AITaskSytem**>(reinterpret_cast<char*>(member->ai) +
                                                kTaskSystemOffset);
    }
    TR_CATCH { return NULL; }
}

bool SafeClearTasks(Character* member)
{
    AITaskSytem* tasks = SafeTaskSystem(member);
    if (tasks == NULL)
    {
        return false;
    }
    TR_TRY
    {
        tasks->clearPermajobs();
        tasks->actions.clearAndDelete();
        tasks->clearSlaveJobs();
        return true;
    }
    TR_CATCH { return false; }
}

void SafeRethink(Character* member)
{
    TR_TRY { member->reThinkCurrentAIAction(); }
    TR_CATCH { }
}

// getHandle returns a reference (C2712): its own frame.
void PortraitUpdateUnguarded(Character* member)
{
    ou->addPortraitUpdate(member->getHandle());
}

void SafeRefreshPortraits(Character* member)
{
    TR_TRY
    {
        if (ou != NULL)
        {
            PortraitUpdateUnguarded(member);
        }
    }
    TR_CATCH { }
}

void LogStep(const std::string& name, const char* what)
{
    LogError("[temp] " + name + ": leaving the squad stopped at " + what + ".");
}
} // namespace

bool LeaveSquad(Character* member, const std::string& homeFactionId)
{
    if (member == NULL)
    {
        return false;
    }
    const std::string name = CharacterName(member);

    std::string used;
    Faction* faction = ResolveFaction(homeFactionId, &used);
    if (faction == NULL)
    {
        LogStep(name, "finding a faction to go back to");
        return false;
    }

    if (!SafeUnselect(member))
    {
        LogStep(name, "unselecting them");
    }
    if (!SafeSetFaction(member, faction))
    {
        LogStep(name, "setFaction");
        return false;
    }

    GameData* squadTemplate = SafeSquadTemplate();
    if (squadTemplate == NULL || !SafeJoinNewPlatoon(member, faction, squadTemplate))
    {
        // Out of the player's faction already; without a platoon of their own they
        // stand where they are until their faction's AI picks them up.
        LogStep(name, squadTemplate == NULL ? "finding the squad template 5006-squad.mod"
                                            : "creating their own platoon");
    }

    if (!SafeClearTasks(member))
    {
        LogStep(name, "clearing their task system");
    }
    SafeRethink(member);
    SafeRefreshPortraits(member);

    bool stillMine = false;
    const bool read = SafeIsPlayerCharacter(member, &stillMine);
    if (read && stillMine)
    {
        LogStep(name, "the check: still one of the player's");
        return false;
    }
    LogInfo("[temp] " + name + " left the squad for faction " + used + ".");
    return true;
}
} // namespace totalrecruit
