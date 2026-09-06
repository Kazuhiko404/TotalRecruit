#pragma once

class Character;

namespace totalrecruit
{
// Losing a limb is the moment Kenshi already treats as a life changing: the
// character stops fighting, runs, and screams. The engine announces it with
// EV_LOST_ARM and EV_LOST_LEG, so the mod does not have to guess at the state.
//
// At that moment personality stops deciding. A maniac with one leg begs the
// same as a coward, because the argument was never about temperament -- it was
// about being able to keep fighting, and they no longer can.
//
// Three answers, because two would be a lie: joining and killing are not the
// only things a player can do to someone bleeding in the dirt.
//   Aceptar  -- they join
//   Perdonar -- they lose all aggro and leave; nobody gains anything
//   Rechazar -- nothing changes, and the fight goes on
void OfferMercy(Character* npc);

// True while this NPC's plea is the one on screen, so the offer window knows
// its third button means "spare them" rather than "feed them".
bool IsMercyPending(Character* npc);

// Clears the plea without acting on it (the player closed the window).
void ForgetMercy(Character* npc);

// Perdonar: drop aggro and let them go.
void GrantMercy(Character* npc);

void ShutdownMercy();
} // namespace totalrecruit
