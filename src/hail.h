#pragma once

class Character;

namespace totalrecruit
{
// Prisoners call out to the player when they are ready to talk.
//
// A recruitable captive is otherwise indistinguishable from scenery: the player
// has to right-click on spec to discover anything changed. Kenshi's own answer
// to this is people shouting at you as you walk past, so a captive who has come
// round to the idea says so out loud, in their own voice, once.
//
// This replaces the cursor hint, which needed the game to route its world
// cursor through a hook it does not actually use.
bool InstallHailing();
void ShutdownHailing();

// Drops every hail clock and restarts the sweep timer. The load boundary calls
// it: the clocks belong to prisoners of the world that was just abandoned.
void ForgetAllHails();

// Called from the frame tick. Cheap on most frames: it only does real work a
// few times a minute, and only for captives held by the player.
void HailTick();
} // namespace totalrecruit
