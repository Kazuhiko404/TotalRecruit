#pragma once

#include <string>
#include <vector>

#include "game_data.h"

namespace totalrecruit
{
// Creates `quantity` characters from a template near the player's squad.
// Each unit is placed on an expanding ring and settled onto walkable ground.
bool SpawnTemplate(const DataEntry& entry, int quantity, std::string* outMessage);

// Creates one character per member of a squad roster.
bool SpawnMembers(const std::vector<DataEntry>& members, std::string* outMessage);

// Spawns a whole squad template through the engine, so the roster, leader and
// unit counts come out exactly as the template defines them.
enum SquadSpawnMode
{
    SquadSpawn_Player,   // joins the player's faction
    SquadSpawn_Roaming,  // spawns in its own faction, roaming
};

bool SpawnSquad(const DataEntry& squad, SquadSpawnMode mode, std::string* outMessage);

// How many characters this session has spawned.
int SpawnedCount();
} // namespace totalrecruit
