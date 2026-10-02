#include "common.h"
#include "pacify.h"

namespace totalrecruit
{
void SafePacify(Character* character, Character* playerLeader)
{
    if (character == NULL)
    {
        return;
    }

    TR_TRY
    {
        character->endCombatMode();
        if (playerLeader != NULL)
        {
            character->clearTempEnemyStatus(playerLeader);
        }
        character->reThinkCurrentAIAction();
    }
    TR_CATCH { }
}
} // namespace totalrecruit
