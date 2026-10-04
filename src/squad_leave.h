#pragma once

// Taking a character out of the player's squad, the native way.
//
// Kenshi has no "leave the squad" call of its own. KenshiExtensionPlugin's dismiss
// (KenshiExtensionPlugin/src/CharacterExtension.cpp:72-108, proven in game) does it in
// six steps, ported here:
//
//   1. the faction to go to: the one named by `homeFactionId` (a faction string id,
//      engine_reads.h FactionStringId, read before they were recruited); else KEP's
//      default "1233-gamedata.base"; else FactionManager::getEmptyFaction. Never a
//      player faction.
//   2. if the player has them selected, PlayerInterface::unselectPlayerCharacter.
//   3. Character::setFaction(faction, NULL): out of the player's faction.
//   4. Faction::createNewEmptyActivePlatoon("5006-squad.mod" -- the vanilla
//      "Wandering Squad" template --, permanent, where they stand) and
//      ActivePlatoon::addActiveObject: a squad of their own.
//   5. the new platoon's blackboard reset to its template (ai_packages.h
//      ResetToTemplate: _flagCleared, the job lists cleared, addPackages): the
//      Wandering Squad's package, 2675-gamedata.base "Wandering from town to town",
//      walks them away. Logged "[temp] <name> native wander package <name> assigned".
//   6. their task system cleared (AITaskSytem::clearPermajobs, actions.clearAndDelete,
//      clearSlaveJobs) so no player order survives, then reThinkCurrentAIAction.
//
// KEP's SquadManagementScreen::refreshSquads belongs to its screen; the portrait bar
// is refreshed instead (GameWorld::addPortraitUpdate). Without the wander package
// (step 4 or 5 failed) the caller walks them back to where they joined
// (leave_homing.h).

#include <string>

class Character;

namespace totalrecruit
{
// Takes `member` out of the player's squad back to `homeFactionId` (see above). True
// when they are no longer the player's. Every engine call is guarded; a failure
// part-way is logged with the step it stopped at. `wanderAssigned` (may be NULL) says
// whether the new platoon got its wander package.
bool LeaveSquad(Character* member, const std::string& homeFactionId, bool* wanderAssigned);
} // namespace totalrecruit
