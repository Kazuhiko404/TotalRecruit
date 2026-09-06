#pragma once

#include <string>
#include <vector>

class GameData;

namespace totalrecruit
{
struct DataEntry
{
    std::string name;
    std::string stringID;
    std::string role; // Which member list a squad entry came from.
    GameData* data;

    DataEntry() : data(NULL) {}
};

// Every plain squad template, sorted by name. Unique/story squads are excluded
// because they expect a specific town and world state.
const std::vector<DataEntry>& SquadTemplates(bool reload);

// Every character and animal definition, sorted by name.
const std::vector<DataEntry>& Characters(bool reload);

// The characters a squad template is built from, read from its leader/squad/
// squad2 lists.
std::vector<DataEntry> SquadMembers(const DataEntry& squad);

// Case-insensitive substring filter.
std::vector<DataEntry> Filter(const std::vector<DataEntry>& entries,
                              const std::string& keyword);

void ClearDataCache();
} // namespace totalrecruit
