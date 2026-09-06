#pragma once

#include <string>

class Character;

namespace totalrecruit
{
// Bridges the plugin's rule engine to FCS-authored dialogue.
//
// The split: FCS owns the words, the branches, the player's choices and the
// engine actions. The plugin owns when to fire, which dialogue, and for whom.
//
// Coverage stays universal because the package is attached at runtime rather
// than relying on it having been installed on the NPC -- which is exactly the
// limitation that forces every other recruitment mod into a patch treadmill.
bool InstallDialogueBridge();
void ShutdownDialogueBridge();

// Attaches the configured DIALOGUE_PACKAGE to this character if it is missing,
// then opens the player-choice window on the given DIALOGUE record.
// Returns false when no package is configured or the record does not resolve,
// in which case the caller should fall back to acting directly.
bool OpenConversation(Character* npc, const std::string& dialogueStringId);

// True when a dialogue window is currently open for a character this plugin
// started. Used to keep the overlay out of the way.
bool IsConversationActive();

// Returns the character the player accepted in dialogue, once, then forgets it.
// Recruiting happens here rather than inside the dialogue hook because the
// engine keeps both parties state-locked until the conversation closes.
// outCats receives the price agreed when the conversation opened.
Character* ConsumePendingRecruit(int* outCats);

// Records what this conversation will cost if the player accepts. The money is
// only taken on acceptance -- declining in dialogue must cost nothing.
void RememberPendingPrice(Character* npc, int cats);
} // namespace totalrecruit
