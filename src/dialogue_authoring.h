#pragma once

#include <string>

class Character;
class DialogLineData;

namespace totalrecruit
{
// Writes dialogue text at runtime.
//
// The FCS side of this mod would otherwise mean hand-authoring a branch for
// every archetype, personality and refusal reason -- dozens of lines, all of
// them things the rule engine already knows. Instead the mod author creates a
// small placeholder tree in FCS (a root and a few replies, text left blank),
// and the plugin fills in the words the moment before the window opens.
//
// This works because DialogLineData exposes its text as a plain writable
// `std::string* texts` array with a `lineCount` beside it, and those objects
// live in DialogDataManager::allDatas -- a runtime map rebuilt from GameData on
// every load. So the edits are in-memory only: they never touch the save, never
// touch the .mod, and are gone at shutdown.
//
// What the author still owns in FCS: how many replies there are, who speaks,
// and what each reply does. What the plugin owns: the words.

// Replaces every text line on this record with `text`. Returns false when the
// record has no text slots, which is the placeholder being malformed rather
// than an error worth stopping for.
bool SetLineText(DialogLineData* line, const std::string& text);

// Fills in a placeholder conversation for one NPC: the root line becomes what
// the NPC says about their situation, and each reply is labelled with what it
// will cost. Called just before the window opens.
//
// Silently does nothing when no placeholder is configured, so the mod keeps
// working as a code-only install.
void AuthorConversation(Character* npc, const std::string& dialogueStringId);

// Makes the NPC say one line out loud, in a speech bubble over their head. No
// dialogue tree, no window, no pause -- Character::sayALine puts the words on
// screen directly.
//
// This is what lets the moment breathe: the NPC speaks where they stand, and
// the window only appears for the decision itself.
void SpeakAloud(Character* npc, const std::string& text);

// The NPC reacts out loud to being clicked, saying what they want. Used when
// no dialogue placeholder is configured, so a code-only install still feels
// like a conversation rather than a menu.
void SpeakVerdict(Character* npc);
} // namespace totalrecruit
