#pragma once

class Character;

namespace totalrecruit
{
// Stops hostility, clears temp enemies, and resets AI action so recruited or rescued
// NPCs do not immediately turn and attack the player.
void SafePacify(Character* character, Character* playerLeader = 0);
} // namespace totalrecruit
