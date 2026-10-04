#pragma once

// The engine's own AI packages on a character's platoon (Platoon::blackboard,
// AI/Blackboard.h, KenshiLib 0.5.0): the one place this mod touches a blackboard.
//
// Every sequence is KenshiExtensionPlugin's, proven in game:
//   AssignBodyguard   kep-mod-tool/PlatoonTool.cpp:145-155 (halt + clearOrders, then
//                     the package) with Blackboard::_setContractJob(package, hours,
//                     hand(employer)) -- the call a hired mercenary's dialogue makes --
//                     and, only when the contract did not become the active package,
//                     Blackboard::replaceAIPackage(package). The contract is read back:
//                     contractJob set, its missionEmployer, the current package.
//                     CharBody::_endAction from KEP's prep is protected in this
//                     KenshiLib and is left out.
//   EndContract       Blackboard::endContractJob, then KEP's "reset AI"
//                     (PlatoonTool.cpp:170-180): replacementAIPackageSID = "",
//                     clearAllPackages, addFallbackPackages + addPackages(the platoon's
//                     squadTemplate), clearPermajobs.
//   ResetToTemplate   KEP's dismiss (CharacterExtension.cpp:91-105) after a new
//                     platoon: _flagCleared = true, leader/squad/squad2/slave jobs
//                     cleared, addPackages(the platoon's squad template). Read back:
//                     a current package, or at least the package lists filled.
//   DetachToOwnPlatoon  Faction::createNewEmptyActivePlatoon(the platoon's own squad
//                     template, permanent, where it stands) + addActiveObject: a
//                     contract is the whole platoon's, so a helper in a squad of others
//                     is moved out first.
//
// Every engine touch is SEH-guarded and NULL-checked (platoon, blackboard, game data,
// squad template, employer); a failure comes back as false with a short English
// reason for the caller's log. Nothing is thrown, nothing is logged here.

#include <string>

class Character;

namespace totalrecruit
{
namespace AiPackages
{
// The vanilla Bodyguard package (SIG_BODYGUARD; val0 = hours).
extern const char* const kBodyguardPackageId;

// What a call did, for the caller's log.
struct Outcome
{
    bool ok;
    // Why it failed, or a note on how it went ("replaceAIPackage needed", ...).
    std::string why;
    // The platoon's current package afterwards, "" when unread.
    std::string package;

    Outcome() : ok(false) {}
};

// How many objects `npc`'s active platoon holds; false when unread.
bool PlatoonMembers(Character* npc, int* members);

// Moves `npc` out of its squad into a platoon of its own (same faction, the old
// platoon's squad template, else "5006-squad.mod").
Outcome DetachToOwnPlatoon(Character* npc);

// The platoon's Bodyguard contract toward `employer` for `hours`, read back.
Outcome AssignBodyguard(Character* npc, Character* employer, int hours);

// Whether `npc`'s platoon carries a contract now; `employerIs` (may be NULL) is
// compared with the contract's missionEmployer when given. False when unread.
bool HasContract(Character* npc, Character* employerIs);

// Ends the platoon's contract and gives it back the packages of its own template.
Outcome EndContract(Character* npc);

// The packages of the platoon's own squad template, after a dismiss (see above).
Outcome ResetToTemplate(Character* npc);

// The platoon's current package name, "" when there is none or it cannot be read.
std::string CurrentPackageName(Character* npc);
} // namespace AiPackages
} // namespace totalrecruit
