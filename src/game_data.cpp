#include "common.h"
#include "game_data.h"
#include "log.h"

#include <algorithm>

#include <kenshi/GameDataManager.h>

namespace totalrecruit
{
namespace
{
// Squad templates reference their members through these lists, matching how
// KEP's SpawnTool reads them (SpawnTool.cpp:1158-1160).
const char* const kMemberLists[] = { "leader", "squad", "squad2" };
const int kMemberListCount = 3;

std::vector<DataEntry> g_squads;
std::vector<DataEntry> g_characters;
bool g_squadsLoaded = false;
bool g_charactersLoaded = false;

std::string ToLower(const std::string& text)
{
    std::string out(text);
    std::transform(out.begin(), out.end(), out.begin(), ::tolower);
    return out;
}

bool ByName(const DataEntry& a, const DataEntry& b)
{
    return ToLower(a.name) < ToLower(b.name);
}

// Each guarded helper touches only PODs, because MSVC rejects __try in a
// function that also unwinds C++ objects.
bool SafeGetDataOfType(itemType type, lektor<GameData*>* list)
{
    TR_TRY
    {
        ou->gamedata.getDataOfType(*list, type);
        return true;
    }
    TR_CATCH { return false; }
}

// The std::string for the list name is built by the caller; constructing it
// inside __try would put an object needing unwinding in the guarded frame.
bool SafeGetListAsDatas(GameData* squad, const std::string& listName,
                        lektor<GameData*>* list)
{
    TR_TRY
    {
        squad->getAllFromListAsDatas(listName, *list, &ou->gamedata, CHARACTER);
        return true;
    }
    TR_CATCH { return false; }
}

// name and stringID are std::string members, so only their c_str pointers
// cross the guard; no temporary is constructed inside it.
bool SafeReadEntry(GameData* data, const char** name, const char** stringID)
{
    TR_TRY
    {
        *name = data->name.c_str();
        *stringID = data->stringID.c_str();
        return true;
    }
    TR_CATCH { return false; }
}

void SafeFreeLektor(lektor<GameData*>* list)
{
    TR_TRY
    {
        if (list->stuff != NULL)
        {
            free(list->stuff);
            list->stuff = NULL;
        }
    }
    TR_CATCH { }
}

// getDataOfType fills a lektor the caller owns; the buffer is released here.
void CollectType(itemType type, std::vector<DataEntry>* out)
{
    lektor<GameData*> list;
    if (!SafeGetDataOfType(type, &list))
    {
        LogError("getDataOfType failed.");
        return;
    }

    for (unsigned int i = 0; i < list.count; ++i)
    {
        GameData* data = list.stuff[i];
        if (data == NULL)
        {
            continue;
        }

        const char* name = NULL;
        const char* stringID = NULL;
        if (!SafeReadEntry(data, &name, &stringID) || name == NULL)
        {
            continue;
        }

        DataEntry entry;
        entry.name = name;
        entry.stringID = (stringID != NULL) ? stringID : "";
        entry.data = data;
        out->push_back(entry);
    }

    SafeFreeLektor(&list);
}
} // namespace

const std::vector<DataEntry>& SquadTemplates(bool reload)
{
    if (reload)
    {
        g_squadsLoaded = false;
    }
    if (g_squadsLoaded)
    {
        return g_squads;
    }

    g_squads.clear();
    CollectType(SQUAD_TEMPLATE, &g_squads);
    std::sort(g_squads.begin(), g_squads.end(), ByName);
    g_squadsLoaded = true;

    char message[128];
    sprintf_s(message, sizeof(message), "Indexed %u squad templates.",
              (unsigned int)g_squads.size());
    LogInfo(message);
    return g_squads;
}

const std::vector<DataEntry>& Characters(bool reload)
{
    if (reload)
    {
        g_charactersLoaded = false;
    }
    if (g_charactersLoaded)
    {
        return g_characters;
    }

    g_characters.clear();
    // Characters and animals are separate item types, as in KEP's SpawnTool.
    CollectType(CHARACTER, &g_characters);
    CollectType(ANIMAL_CHARACTER, &g_characters);
    std::sort(g_characters.begin(), g_characters.end(), ByName);
    g_charactersLoaded = true;

    char message[128];
    sprintf_s(message, sizeof(message), "Indexed %u characters.",
              (unsigned int)g_characters.size());
    LogInfo(message);
    return g_characters;
}

std::vector<DataEntry> SquadMembers(const DataEntry& squad)
{
    std::vector<DataEntry> members;
    if (squad.data == NULL)
    {
        return members;
    }

    for (int listIndex = 0; listIndex < kMemberListCount; ++listIndex)
    {
        const std::string listName(kMemberLists[listIndex]);

        lektor<GameData*> list;
        if (!SafeGetListAsDatas(squad.data, listName, &list))
        {
            continue;
        }

        for (unsigned int i = 0; i < list.count; ++i)
        {
            GameData* data = list.stuff[i];
            if (data == NULL)
            {
                continue;
            }

            bool duplicate = false;
            for (size_t seen = 0; seen < members.size(); ++seen)
            {
                if (members[seen].data == data)
                {
                    duplicate = true;
                    break;
                }
            }
            if (duplicate)
            {
                continue;
            }

            const char* name = NULL;
            const char* stringID = NULL;
            if (!SafeReadEntry(data, &name, &stringID) || name == NULL)
            {
                continue;
            }

            DataEntry entry;
            entry.name = name;
            entry.stringID = (stringID != NULL) ? stringID : "";
            entry.role = kMemberLists[listIndex];
            entry.data = data;
            members.push_back(entry);
        }

        SafeFreeLektor(&list);
    }

    std::sort(members.begin(), members.end(), ByName);
    return members;
}

std::vector<DataEntry> Filter(const std::vector<DataEntry>& entries,
                              const std::string& keyword)
{
    if (keyword.empty())
    {
        return entries;
    }

    const std::string needle = ToLower(keyword);
    std::vector<DataEntry> out;
    for (size_t i = 0; i < entries.size(); ++i)
    {
        if (ToLower(entries[i].name).find(needle) != std::string::npos)
        {
            out.push_back(entries[i]);
        }
    }
    return out;
}

void ClearDataCache()
{
    g_squads.clear();
    g_characters.clear();
    g_squadsLoaded = false;
    g_charactersLoaded = false;
}
} // namespace totalrecruit
