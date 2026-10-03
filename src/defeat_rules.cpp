#include "defeat_rules.h"

namespace totalrecruit
{
DefeatCredit DecideDefeatCredit(bool victimIsPlayers, bool winnerReadable,
                                bool winnerIsPlayer, bool creditedToPlayer)
{
    if (victimIsPlayers)
    {
        return DefeatCredit_SkipPlayersOwn;
    }
    if (creditedToPlayer)
    {
        return DefeatCredit_Counts;
    }
    if (!winnerReadable)
    {
        return DefeatCredit_SkipWinnerUnreadable;
    }
    return winnerIsPlayer ? DefeatCredit_Counts : DefeatCredit_SkipNotThePlayer;
}
} // namespace totalrecruit
