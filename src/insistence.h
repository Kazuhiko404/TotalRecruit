#pragma once

class Character;

namespace totalrecruit
{
// Insistence: what happens when the player keeps asking (R2.4). Pestering should
// cost something, and the cost should grow -- a flat penalty makes the fourth ask
// free once the third has already landed. The numbers are config.insistence.
//
// It lives only in memory (a reload forgets it), keyed by CharKey.

// Checks if the NPC is currently ignoring the player due to excessive insistence.
bool IsIgnoringPlayer(Character* character, double* outRemainingHours = 0);

// Records an interaction attempt for insistence tracking. Returns true if ignored.
// Called only for a talk that counts (CountsAsInsistence, insistence_rules.h).
bool RecordInteraction(Character* character, double nowHours);

// Forgets every insistence record (who has been pestered, who is ignoring the
// player). Called when a save loads.
void ResetTalkStates();
} // namespace totalrecruit
