#pragma once

// How many defeats a character counts as having suffered, without the engine.
//
// The count is made of two readings that cover each other's blind spot: the
// precise session counter (lost on reload) and the floor the save keeps in the
// engine's per-character memory (survives, but can only say "at least once").
// A reload costs precision, never progress -- which only holds if the floor is
// large enough to keep every route the count unlocked.

namespace totalrecruit
{
// What a reload leaves behind for a character carrying the persistent
// "the player beat my squad once" memory tag, and 0 for one without it.
//
// The configured floor is never allowed below defeatsToBreak: the memory tag is
// written at the first knockout, so a floor of 1 under a threshold of 2 turned
// every reload into a step back from "ready to yield" to "barely touched"
// (REQUISITOS R3.3: the NPC never goes back to intact once defeated).
int PersistedDefeatFloor(bool markedInSave, int configuredFloor, int defeatsToBreak);

// The count the rules read: the precise session count, but never below the floor.
int EffectiveDefeatCount(int sessionCount, int persistedFloor);

// Whether a knockout counts towards yielding to the player, and why not.
enum DefeatCredit
{
    // The player put them down: it counts.
    DefeatCredit_Counts,
    // One of the player's own people going down is nobody's defeat to inflict.
    DefeatCredit_SkipPlayersOwn,
    // The engine's own record of who beat them is empty or unreadable.
    DefeatCredit_SkipWinnerUnreadable,
    // Somebody else beat them (or they fell on their own).
    DefeatCredit_SkipNotThePlayer
};

// The engine raises a knockout for everyone it puts down, deaths in the field the
// player never saw included, and lastGuyWhoDefeatedMe is its answer to who did
// it. That answer is only worth asking when the plugin did not see the act
// itself: when the player's own action caused the knockout (the beating, the one
// deliberate way to put a prisoner out) the credit is explicit, because asking
// the engine to confirm what the plugin just did made the beating count only when
// the engine happened to agree.
DefeatCredit DecideDefeatCredit(bool victimIsPlayers, bool winnerReadable,
                                bool winnerIsPlayer, bool creditedToPlayer);

// One beating raises several engine calls as the body settles, and a character
// left down keeps registering while they lie there. A second knockout inside the
// debounce window is an echo of the first, not a second defeat.
bool IsEchoOfLastDefeat(double nowHours, double lastDefeatHour, double debounceHours);
} // namespace totalrecruit
