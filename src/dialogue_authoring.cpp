#include "common.h"
#include "dialogue_authoring.h"
#include "rules.h"
#include "config.h"
#include "recruit_core.h"
#include "engine_reads.h"
#include "log.h"

namespace totalrecruit
{
namespace
{
// Writing into `texts` is safe because DialogLineData objects live in
// DialogDataManager::allDatas, a runtime map rebuilt from GameData on every
// load. Nothing here reaches the save file or the .mod on disk.
void WriteText(DialogLineData* line, const char* text)
{
    for (int i = 0; i < line->lineCount; ++i)
    {
        line->texts[i] = text;
    }
}

bool SafeWriteText(DialogLineData* line, const char* text, int* outSlots)
{
    TR_TRY
    {
        if (line->texts == NULL || line->lineCount <= 0)
        {
            *outSlots = 0;
            return true;
        }
        *outSlots = line->lineCount;
        WriteText(line, text);
        return true;
    }
    TR_CATCH { return false; }
}

GameData* LookupDialogue(const char* stringId)
{
    return ou->gamedata.getData(std::string(stringId), DIALOGUE);
}

bool SafeLookupDialogue(const char* stringId, GameData** out)
{
    if (ou == NULL)
    {
        return false;
    }
    TR_TRY
    {
        *out = LookupDialogue(stringId);
        return (*out != NULL);
    }
    TR_CATCH { return false; }
}

bool SafeGetLine(GameData* data, DialogLineData** out)
{
    TR_TRY
    {
        *out = DialogDataManager::getData(data);
        return (*out != NULL);
    }
    TR_CATCH { return false; }
}

// Replies are the root's children. The engine builds that list lazily, so
// setupChildren is called first in case the placeholder has not been opened yet.
bool SafeGetChildren(DialogLineData* root, DialogChoiceList** out)
{
    TR_TRY
    {
        root->setupChildren();
        *out = root->children;
        return (*out != NULL);
    }
    TR_CATCH { return false; }
}

bool SafeChildCount(DialogChoiceList* children, int* out)
{
    TR_TRY
    {
        *out = (int)children->conversationChoices.size();
        return true;
    }
    TR_CATCH { return false; }
}

bool SafeChildAt(DialogChoiceList* children, int index, DialogLineData** out)
{
    TR_TRY
    {
        *out = children->conversationChoices[index];
        return (*out != NULL);
    }
    TR_CATCH { return false; }
}
} // namespace

namespace
{
// sayALine takes a std::string, so the temporary is built in a plain function
// and only the call crosses the guard.
void SpeakNow(Character* npc, const char* text)
{
    npc->sayALine(std::string(text), true);
}

bool SafeSpeak(Character* npc, const char* text)
{
    TR_TRY
    {
        SpeakNow(npc, text);
        return true;
    }
    TR_CATCH { return false; }
}

bool SafeCanSpeak(Character* npc, bool* out)
{
    TR_TRY
    {
        *out = npc->canSpeakNormally();
        return true;
    }
    TR_CATCH { return false; }
}
} // namespace

void SpeakAloud(Character* npc, const std::string& text)
{
    if (npc == NULL || text.empty())
    {
        return;
    }

    // canSpeakNormally already accounts for being knocked out, gagged or
    // otherwise unable to talk. Without it an unconscious body sprouts a speech
    // bubble, which reads as a bug even when the words are right.
    bool canSpeak = false;
    if (!SafeCanSpeak(npc, &canSpeak) || !canSpeak)
    {
        return;
    }

    SafeSpeak(npc, text.c_str());
}

void SpeakVerdict(Character* npc)
{
    if (npc == NULL)
    {
        return;
    }

    const Verdict verdict = Evaluate(npc);
    SpeakAloud(npc, SpeakOpeningLine(npc, verdict));
}

bool SetLineText(DialogLineData* line, const std::string& text)
{
    if (line == NULL)
    {
        return false;
    }

    int slots = 0;
    if (!SafeWriteText(line, text.c_str(), &slots))
    {
        return false;
    }
    return slots > 0;
}

void AuthorConversation(Character* npc, const std::string& dialogueStringId)
{
    if (npc == NULL || dialogueStringId.empty())
    {
        return;
    }

    GameData* data = NULL;
    if (!SafeLookupDialogue(dialogueStringId.c_str(), &data))
    {
        return;
    }

    DialogLineData* root = NULL;
    if (!SafeGetLine(data, &root))
    {
        return;
    }

    const Verdict verdict = Evaluate(npc);

    // The root is what the NPC says about their own situation.
    SetLineText(root, SpeakOpeningLine(npc, verdict));

    // The replies are the player's options. The author decides how many there
    // are and what each one does; the plugin only supplies the words, so a
    // placeholder with two replies works and one with five works too.
    DialogChoiceList* children = NULL;
    if (!SafeGetChildren(root, &children))
    {
        return;
    }

    int count = 0;
    if (!SafeChildCount(children, &count) || count <= 0)
    {
        return;
    }

    // First reply accepts, and names the price so the player sees what they are
    // agreeing to. Any further replies decline, and the last one carries the
    // reason -- which is the thing no dialogue-data mod can say.
    for (int i = 0; i < count; ++i)
    {
        DialogLineData* reply = NULL;
        if (!SafeChildAt(children, i, &reply))
        {
            continue;
        }

        if (i == 0)
        {
            SetLineText(reply, SpeakAcceptLine(verdict));
        }
        else if (i == count - 1)
        {
            SetLineText(reply, "Leave them. (" + verdict.reason + ")");
        }
        else
        {
            SetLineText(reply, "Say nothing.");
        }
    }

    LogInfo("Authored '" + dialogueStringId + "' for " + CharacterName(npc) +
            " (" + verdict.reason + ").");
}
} // namespace totalrecruit
