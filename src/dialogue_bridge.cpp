#include "common.h"
#include "dialogue_bridge.h"
#include "rules.h"
#include "config.h"
#include "recruit_core.h"
#include "log.h"
#include "dialogue_authoring.h"

namespace totalrecruit
{
namespace
{
// The character this plugin most recently opened a conversation with, and the
// route it was opened for. Only ever dereferenced behind TR_TRY.
Character* g_conversationNpc = NULL;
bool g_conversationActive = false;

// Set when the player picks a line carrying our accept key, and consumed on the
// next frame. Recruiting from inside _doActions would mutate a character the
// engine still has locked in the conversation.
Character* g_pendingRecruit = NULL;
int g_pendingCats = 0;

// The price quoted when the conversation opened, held against the character it
// was quoted for so a stale figure cannot be charged for someone else.
Character* g_quotedFor = NULL;
int g_quotedCats = 0;

void (*g_doActions_orig)(Dialogue*, DialogLineData*) = NULL;

// getData takes a std::string, so the temporary is built in a plain function
// and only the call is guarded; C2712 counts every object in the whole frame.
GameData* LookupData(const char* stringId, itemType type)
{
    return ou->gamedata.getData(std::string(stringId), type);
}

bool SafeGetData(const char* stringId, itemType type, GameData** out)
{
    if (ou == NULL)
    {
        return false;
    }

    TR_TRY
    {
        *out = LookupData(stringId, type);
        return (*out != NULL);
    }
    TR_CATCH { return false; }
}

bool SafeHasPackage(Character* npc, GameData* package, bool* out)
{
    TR_TRY
    {
        if (npc->dialogue == NULL)
        {
            return false;
        }
        *out = (npc->dialogue->pacakgesIHave.find(package) !=
                npc->dialogue->pacakgesIHave.end());
        return true;
    }
    TR_CATCH { return false; }
}

// Attaching at runtime is what keeps coverage universal: the NPC does not need
// to have shipped with our package, so characters from mods that never heard of
// this plugin are reachable too.
bool SafeAddPackage(Character* npc, GameData* package)
{
    TR_TRY
    {
        if (npc->dialogue == NULL)
        {
            return false;
        }
        npc->dialogue->addDialoguePackage(package);
        return true;
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
        if (platoon == NULL)
        {
            return false;
        }
        *out = platoon->getSquadLeader();
        return (*out != NULL);
    }
    TR_CATCH { return false; }
}

// DialogChoiceList is a stack local here, matching KEP's PlatoonTool: build a
// one-entry list, let the engine score it, then hand the chosen line over.
bool StartConversation(Character* npc, Character* player, GameData* dialogue)
{
    DialogChoiceList list;
    list.add(dialogue, NULL);

    DialogLineData* line = npc->dialogue->_chooseDialog(&list, player, false);
    if (line == NULL)
    {
        return false;
    }
    return npc->dialogue->startPlayerConversation(player, line);
}

bool SafeStartConversation(Character* npc, Character* player, GameData* dialogue, bool* out)
{
    TR_TRY
    {
        if (npc->dialogue == NULL)
        {
            return false;
        }
        *out = StartConversation(npc, player, dialogue);
        return true;
    }
    TR_CATCH { return false; }
}

// Reads the FCS reference list the mod author tags an accept line with. Keeping
// the lookup in its own plain function leaves the guarded frame POD-only.
bool LineCarriesKey(DialogLineData* line, const char* key)
{
    GameData* data = line->getGameData();
    if (data == NULL)
    {
        return false;
    }
    return data->objectReferences.find(key) != data->objectReferences.end();
}

bool SafeLineCarriesKey(DialogLineData* line, const char* key, bool* out)
{
    TR_TRY
    {
        *out = LineCarriesKey(line, key);
        return true;
    }
    TR_CATCH { return false; }
}

bool SafeConversationParties(Dialogue* dialogue, Character** outNpc, Character** outTarget)
{
    TR_TRY
    {
        *outNpc = dialogue->me;
        *outTarget = dialogue->getConversationTarget().getCharacter();
        return (*outNpc != NULL);
    }
    TR_CATCH { return false; }
}

bool SafeEndDialogue(Dialogue* dialogue)
{
    TR_TRY
    {
        dialogue->endDialogue(true);
        return true;
    }
    TR_CATCH { return false; }
}

// Fires for every line the engine executes, including the one the player
// clicked. There is no callback or listener API -- this hook is the only way to
// learn what was chosen.
void DoActions_hook(Dialogue* thisptr, DialogLineData* dialogLine)
{
    if (thisptr != NULL && dialogLine != NULL)
    {
        const std::string& key = GetConfig().dialogue.acceptKey;
        bool accepted = false;
        if (!key.empty() && SafeLineCarriesKey(dialogLine, key.c_str(), &accepted) && accepted)
        {
            Character* npc = NULL;
            Character* target = NULL;
            if (SafeConversationParties(thisptr, &npc, &target))
            {
                // Characters are state-locked for the duration of a
                // conversation, so the world change is deferred: close the
                // window here, recruit on the next frame.
                g_pendingRecruit = npc;
                g_pendingCats = (npc == g_quotedFor) ? g_quotedCats : 0;
                SafeEndDialogue(thisptr);
                LogInfo("Dialogue accept from " + CharacterName(npc) + "; deferring recruit.");
            }
        }
    }

    if (g_doActions_orig != NULL)
    {
        g_doActions_orig(thisptr, dialogLine);
    }
}
} // namespace

bool InstallDialogueBridge()
{
    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&Dialogue::_doActions),
            DoActions_hook, &g_doActions_orig))
    {
        LogError("Could not hook Dialogue::_doActions; dialogue replies will not be read.");
        return false;
    }

    LogInfo("Dialogue bridge installed.");
    return true;
}

void ShutdownDialogueBridge()
{
    g_conversationNpc = NULL;
    g_conversationActive = false;
    g_pendingRecruit = NULL;
}

bool IsConversationActive()
{
    return g_conversationActive;
}

bool OpenConversation(Character* npc, const std::string& dialogueStringId)
{
    if (npc == NULL || dialogueStringId.empty())
    {
        return false;
    }

    GameData* dialogue = NULL;
    if (!SafeGetData(dialogueStringId.c_str(), DIALOGUE, &dialogue))
    {
        LogInfo("Dialogue '" + dialogueStringId + "' not found; acting directly instead.");
        return false;
    }

    // Attach the package first, so lines gated on it can match.
    const std::string& packageId = GetConfig().dialogue.package;
    if (!packageId.empty())
    {
        GameData* package = NULL;
        if (SafeGetData(packageId.c_str(), DIALOGUE_PACKAGE, &package))
        {
            bool alreadyHas = false;
            if (SafeHasPackage(npc, package, &alreadyHas) && !alreadyHas)
            {
                SafeAddPackage(npc, package);
            }
        }
    }

    Character* player = NULL;
    if (!SafePlayerLeader(&player))
    {
        return false;
    }

    // Fill the placeholder with words that fit this NPC, immediately before the
    // window reads them. The author supplies the shape of the conversation in
    // FCS; the plugin supplies what is actually said.
    AuthorConversation(npc, dialogueStringId);

    bool started = false;
    if (!SafeStartConversation(npc, player, dialogue, &started) || !started)
    {
        LogInfo("Could not start conversation with " + CharacterName(npc) + ".");
        return false;
    }

    g_conversationNpc = npc;
    g_conversationActive = true;
    return true;
}

void RememberPendingPrice(Character* npc, int cats)
{
    g_quotedFor = npc;
    g_quotedCats = cats;
}

Character* ConsumePendingRecruit(int* outCats)
{
    Character* pending = g_pendingRecruit;
    if (outCats != NULL)
    {
        *outCats = g_pendingCats;
    }

    g_pendingRecruit = NULL;
    g_pendingCats = 0;

    if (pending != NULL)
    {
        g_conversationActive = false;
        g_quotedFor = NULL;
        g_quotedCats = 0;
    }
    return pending;
}
} // namespace totalrecruit
