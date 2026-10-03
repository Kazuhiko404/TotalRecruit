#pragma once

// Taking a character out of the player's squad, the native way.
//
// Kenshi has no "leave the squad" call of its own. KenshiExtensionPlugin's dismiss
// (KenshiExtensionPlugin/src/CharacterExtension.cpp:72-108, proven in game) does it in
// five steps, ported here:
//
//   1. the faction to go to: the one named by `homeFactionId` (a faction string id,
//      engine_reads.h FactionStringId, read before they were recruited); else KEP's
//      default "1233-gamedata.base"; else FactionManager::getEmptyFaction. Never a
//      player faction.
//   2. if the player has them selected, PlayerInterface::unselectPlayerCharacter.
//   3. Character::setFaction(faction, NULL): out of the player's faction.
//   4. Faction::createNewEmptyActivePlatoon("5006-squad.mod" -- the vanilla
//      "Wandering Squad" template --, permanent, where they stand) and
//      ActivePlatoon::addActiveObject: a squad of their own, whose AI walks them away.
//   5. their task system cleared (AITaskSytem::clearPermajobs, actions.clearAndDelete,
//      clearSlaveJobs) so no player order survives, then reThinkCurrentAIAction.
//
// Skipped from KEP: clearing the blackboard jobs and Blackboard::addPackages(template).
// AI/Blackboard.h is not in this KenshiLib, so the new platoon's own packages and the
// re-think are trusted to give them their routine (QA.md checks they walk away).
// KEP's SquadManagementScreen::refreshSquads belongs to its screen; the portrait bar
// is refreshed instead (GameWorld::addPortraitUpdate).

#include <string>

class Character;

namespace totalrecruit
{
// Takes `member` out of the player's squad back to `homeFactionId` (see above). True
// when they are no longer the player's. Every engine call is guarded; a failure
// part-way is logged with the step it stopped at.
bool LeaveSquad(Character* member, const std::string& homeFactionId);
} // namespace totalrecruit
