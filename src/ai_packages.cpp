#include "common.h"
#include "ai_packages.h"

// AI/Blackboard.h brings AI/AI.h and AI/AITaskSystem.h with it; Tasker.h is not
// included (it redefines taskPriority, see npc_motion.cpp). AI/AIPackage.h cannot be
// included next to it either (both define enum BlackboardSignalFunctions in 0.5.0),
// so the two AIPackage members read here go through their documented offsets.
#include <kenshi/AI/Blackboard.h>
#include <kenshi/CharMovement.h>

#include <cstring>

// See ai_packages.h. SEH frames hold PODs only (C2712): every std::string and
// Ogre::Vector3 lives in a plain helper, names come back through char buffers, and
// the Outcome is filled outside the guards.

namespace totalrecruit
{
namespace AiPackages
{
const char* const kBodyguardPackageId = "5090-gamedata.base";

namespace
{
// The vanilla "Wandering Squad": the template of a new platoon when the old one had none.
const char* const kWanderingSquadId = "5006-squad.mod";
const size_t kNameBytes = 128;
// AI/AIPackage.h: "hand missionEmployer; // 0x48", "GameData* packageData; // 0x68".
const size_t kMissionEmployerOffset = 0x48;
const size_t kPackageDataOffset = 0x68;

GameData* PackageData(AIPackage* package)
{
    return *reinterpret_cast<GameData**>(reinterpret_cast<char*>(package) + kPackageDataOffset);
}

const hand& MissionEmployer(AIPackage* package)
{
    return *reinterpret_cast<const hand*>(reinterpret_cast<char*>(package) + kMissionEmployerOffset);
}

// ------------------------------------------------------------- Plain helpers
GameData* GameDataUnguarded(const char* sid)
{
    return ou->gamedata.getData(std::string(sid));
}

GameData* SafeGameData(const char* sid)
{
    TR_TRY
    {
        if (ou == NULL)
            return NULL;
        return GameDataUnguarded(sid);
    }
    TR_CATCH { return NULL; }
}

void CopyName(const std::string& from, char* out)
{
    strncpy_s(out, kNameBytes, from.c_str(), _TRUNCATE);
}

// ------------------------------------------------------------- Reads
Platoon* SafePlatoonOf(Character* npc)
{
    if (npc == NULL)
        return NULL;
    TR_TRY
    {
        ActivePlatoon* active = npc->getPlatoon();
        return active != NULL ? active->me : NULL;
    }
    TR_CATCH { return NULL; }
}

Blackboard* SafeBlackboardOf(Platoon* platoon)
{
    if (platoon == NULL)
        return NULL;
    TR_TRY { return platoon->blackboard; }
    TR_CATCH { return NULL; }
}

GameData* SafeSquadTemplateOf(Platoon* platoon)
{
    if (platoon == NULL)
        return NULL;
    TR_TRY { return platoon->squadTemplate; }
    TR_CATCH { return NULL; }
}

bool SafeCurrentPackageName(Blackboard* bb, char* out)
{
    out[0] = 0;
    if (bb == NULL)
        return false;
    TR_TRY
    {
        AIPackage* current = bb->currentPackage;
        if (current == NULL || PackageData(current) == NULL)
            return false;
        CopyName(PackageData(current)->name, out);
        return true;
    }
    TR_CATCH { return false; }
}

// 1 = a contract, 0 = none, -1 = unread. `employerIs` NULL skips that check; with
// one given, a contract for someone else reads as 0.
int SafeContractState(Blackboard* bb, Character* employerIs)
{
    if (bb == NULL)
        return -1;
    TR_TRY
    {
        AIPackage* contract = bb->contractJob;
        if (contract == NULL)
            return 0;
        if (employerIs == NULL)
            return 1;
        return MissionEmployer(contract).getCharacter() == employerIs ? 1 : 0;
    }
    TR_CATCH { return -1; }
}

// ------------------------------------------------------------- Actions
// KEP's prep before a package change: stop moving and drop the orders.
bool SafePrepare(Character* npc)
{
    TR_TRY
    {
        CharMovement* movement = npc->getMovement();
        if (movement != NULL)
            movement->halt();
        OrdersReceiver* orders = npc->getOrdersReciever();
        if (orders != NULL)
            orders->clearOrders();
        return true;
    }
    TR_CATCH { return false; }
}

// hand has no destructor (TotalSparring fight_starter.cpp), so it may live here.
bool SafeSetContract(Blackboard* bb, GameData* package, int hours, Character* employer)
{
    TR_TRY
    {
        const hand giver(employer);
        bb->_setContractJob(package, hours, giver);
        return bb->contractJob != NULL;
    }
    TR_CATCH { return false; }
}

// 1 = the contract's package is the active one, 0 = not, -1 = unread.
int SafeContractIsActive(Blackboard* bb, GameData* package)
{
    TR_TRY
    {
        AIPackage* current = bb->getCurrentPackage();
        return (current != NULL && PackageData(current) == package) ? 1 : 0;
    }
    TR_CATCH { return -1; }
}

bool SafeReplacePackage(Blackboard* bb, GameData* package)
{
    TR_TRY { bb->replaceAIPackage(package); return true; }
    TR_CATCH { return false; }
}

bool SafeEndContract(Blackboard* bb)
{
    TR_TRY { bb->endContractJob(); return true; }
    TR_CATCH { return false; }
}

// The string member is assigned outside the guard (an operator= call it is, but it
// keeps the guarded frame free of anything std::string).
void ClearReplacementUnguarded(Blackboard* bb)
{
    bb->replacementAIPackageSID = "";
}

bool SafeResetPackages(Blackboard* bb, GameData* squadTemplate)
{
    TR_TRY
    {
        ClearReplacementUnguarded(bb);
        bb->clearAllPackages();
        bb->addFallbackPackages(squadTemplate);
        bb->addPackages(squadTemplate);
        return true;
    }
    TR_CATCH { return false; }
}

// KEP's dismiss reset. 1 = done, 0 = no template, -1 = faulted.
int SafeDismissReset(Blackboard* bb, GameData* squadTemplate)
{
    if (squadTemplate == NULL)
        return 0;
    TR_TRY
    {
        bb->_flagCleared = true;
        bb->leaderJobs.clear();
        bb->squadJobs.clear();
        bb->squadJobs2.clear();
        bb->slaveJobs.clear();
        bb->addPackages(squadTemplate);
        return 1;
    }
    TR_CATCH { return -1; }
}

// How many package lists the blackboard holds; -1 unread.
int SafePackageListCount(Blackboard* bb)
{
    TR_TRY { return (int)bb->packagesMain.size(); }
    TR_CATCH { return -1; }
}

bool SafeClearPermajobs(Character* npc)
{
    TR_TRY
    {
        if (npc->ai == NULL || npc->ai->taskSystemAI == NULL)
            return false;
        npc->ai->taskSystemAI->clearPermajobs();
        return true;
    }
    TR_CATCH { return false; }
}

Platoon* NewPlatoonUnguarded(Character* npc, Faction* faction, GameData* squadTemplate)
{
    const Ogre::Vector3 position = npc->getPosition();
    return faction->createNewEmptyActivePlatoon(squadTemplate, true, position);
}

bool SafeJoinNewPlatoon(Character* npc, GameData* squadTemplate)
{
    TR_TRY
    {
        Faction* faction = npc->getFaction();
        if (faction == NULL)
            return false;
        Platoon* platoon = NewPlatoonUnguarded(npc, faction, squadTemplate);
        if (platoon == NULL || platoon->activePlatoon == NULL)
            return false;
        return platoon->activePlatoon->_NV_addActiveObject(npc);
    }
    TR_CATCH { return false; }
}

int SafeMemberCount(Character* npc)
{
    TR_TRY
    {
        ActivePlatoon* active = npc->getPlatoon();
        return active != NULL ? (int)active->things.size() : -1;
    }
    TR_CATCH { return -1; }
}

std::string NameOf(Blackboard* bb)
{
    char name[kNameBytes] = { 0 };
    SafeCurrentPackageName(bb, name);
    return std::string(name);
}
} // namespace

bool PlatoonMembers(Character* npc, int* members)
{
    if (npc == NULL || members == NULL)
        return false;
    const int count = SafeMemberCount(npc);
    if (count < 0)
        return false;
    *members = count;
    return true;
}

Outcome DetachToOwnPlatoon(Character* npc)
{
    Outcome out;
    if (npc == NULL)
    {
        out.why = "no character";
        return out;
    }
    GameData* squadTemplate = SafeSquadTemplateOf(SafePlatoonOf(npc));
    if (squadTemplate == NULL)
        squadTemplate = SafeGameData(kWanderingSquadId);
    if (squadTemplate == NULL)
    {
        out.why = "no squad template (theirs nor 5006-squad.mod)";
        return out;
    }
    out.ok = SafeJoinNewPlatoon(npc, squadTemplate);
    out.why = out.ok ? "own platoon" : "createNewEmptyActivePlatoon / addActiveObject failed";
    out.package = NameOf(SafeBlackboardOf(SafePlatoonOf(npc)));
    return out;
}

Outcome AssignBodyguard(Character* npc, Character* employer, int hours)
{
    Outcome out;
    if (npc == NULL || employer == NULL)
    {
        out.why = npc == NULL ? "no character" : "no employer (squad leader)";
        return out;
    }
    Blackboard* bb = SafeBlackboardOf(SafePlatoonOf(npc));
    if (bb == NULL)
    {
        out.why = "no platoon blackboard";
        return out;
    }
    GameData* package = SafeGameData(kBodyguardPackageId);
    if (package == NULL)
    {
        out.why = std::string("package ") + kBodyguardPackageId + " not in the game data";
        return out;
    }
    if (!SafePrepare(npc))
        out.why = "prep (halt / clearOrders) faulted; ";

    if (!SafeSetContract(bb, package, hours < 1 ? 1 : hours, employer))
    {
        out.why += "_setContractJob left no contractJob";
        out.package = NameOf(bb);
        return out;
    }
    const int active = SafeContractIsActive(bb, package);
    if (active == 0)
    {
        const bool replaced = SafeReplacePackage(bb, package);
        out.why += replaced ? "replaceAIPackage needed; " : "replaceAIPackage faulted; ";
    }
    const int contract = SafeContractState(bb, employer);
    out.ok = contract == 1;
    if (contract == 0)
        out.why += "contractJob missing or employer is not the leader after assignment";
    else if (contract < 0)
        out.why += "contractJob unreadable after assignment";
    else
        out.why += active == 1 ? "contract active" : "contract set";
    out.package = NameOf(bb);
    return out;
}

bool HasContract(Character* npc, Character* employerIs)
{
    return SafeContractState(SafeBlackboardOf(SafePlatoonOf(npc)), employerIs) == 1;
}

Outcome EndContract(Character* npc)
{
    Outcome out;
    Platoon* platoon = SafePlatoonOf(npc);
    Blackboard* bb = SafeBlackboardOf(platoon);
    if (bb == NULL)
    {
        out.why = "no platoon blackboard";
        return out;
    }
    const bool ended = SafeEndContract(bb);
    GameData* squadTemplate = SafeSquadTemplateOf(platoon);
    if (squadTemplate == NULL)
    {
        out.ok = false;
        out.why = std::string(ended ? "contract ended" : "endContractJob faulted") +
                  "; no squad template to reset the packages from";
        out.package = NameOf(bb);
        return out;
    }
    const bool reset = SafeResetPackages(bb, squadTemplate);
    const bool permajobs = SafeClearPermajobs(npc);
    out.ok = ended && reset;
    out.why = std::string(ended ? "contract ended" : "endContractJob faulted") +
              (reset ? ", packages reset to the squad template" : ", package reset faulted") +
              (permajobs ? "" : ", clearPermajobs failed");
    out.package = NameOf(bb);
    return out;
}

Outcome ResetToTemplate(Character* npc)
{
    Outcome out;
    Platoon* platoon = SafePlatoonOf(npc);
    Blackboard* bb = SafeBlackboardOf(platoon);
    if (bb == NULL)
    {
        out.why = "no platoon blackboard";
        return out;
    }
    const int reset = SafeDismissReset(bb, SafeSquadTemplateOf(platoon));
    if (reset <= 0)
    {
        out.why = reset == 0 ? "the platoon has no squad template" : "the package reset faulted";
        return out;
    }
    out.package = NameOf(bb);
    const int lists = SafePackageListCount(bb);
    out.ok = !out.package.empty() || lists > 0;
    out.why = out.ok ? (out.package.empty() ? "package lists filled, none active yet" : "package active")
                     : "no package after addPackages";
    return out;
}

std::string CurrentPackageName(Character* npc)
{
    return NameOf(SafeBlackboardOf(SafePlatoonOf(npc)));
}
} // namespace AiPackages
} // namespace totalrecruit
