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
} // namespace totalrecruit
