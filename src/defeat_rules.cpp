#include "defeat_rules.h"

namespace totalrecruit
{
int PersistedDefeatFloor(bool markedInSave, int configuredFloor, int defeatsToBreak)
{
    if (!markedInSave)
    {
        return 0;
    }
    return configuredFloor > defeatsToBreak ? configuredFloor : defeatsToBreak;
}

int EffectiveDefeatCount(int sessionCount, int persistedFloor)
{
    return sessionCount > persistedFloor ? sessionCount : persistedFloor;
}

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

bool IsEchoOfLastDefeat(double nowHours, double lastDefeatHour, double debounceHours)
{
    return nowHours - lastDefeatHour < debounceHours;
}
} // namespace totalrecruit
