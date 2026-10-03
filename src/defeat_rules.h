#pragma once

// Who gets credit for a knockout, without the engine.
//
// Whether a credited knockout is a NEW defeat or the same one seen again is
// defeat_episode_rules.h; how many the character has suffered, and how that
// survives a reload, is defeat_ledger.h.

namespace totalrecruit
{
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
} // namespace totalrecruit
