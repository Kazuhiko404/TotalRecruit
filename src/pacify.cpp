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

namespace
{
// One member at a time, so a member freed mid-loop costs only that member.
void CalmTowardOneGuarded(Character* npc, Character* member)
{
    npc->clearTempEnemyStatus(member);
    npc->rememberCharacter(member, ST_NONE);
    member->clearTempEnemyStatus(npc);
}

void SafeCalmTowardOne(Character* npc, Character* member)
{
    TR_TRY { CalmTowardOneGuarded(npc, member); }
    TR_CATCH { }
}

bool SafeTempEnemyOf(Character* npc, Character* member, bool* out)
{
    TR_TRY
    {
        *out = npc->getCharacterMemoryTag(member, ST_TEMPORARY_ENEMY);
        return true;
    }
    TR_CATCH { return false; }
}
} // namespace

void SafeCalmToward(Character* npc, Character* const* squad, int count)
{
    if (npc == NULL)
    {
        return;
    }

    TR_TRY
    {
        npc->endCombatMode();
        npc->attackTarget(0);
    }
    TR_CATCH { }

    for (int i = 0; i < count; ++i)
    {
        if (squad[i] != NULL && squad[i] != npc)
        {
            SafeCalmTowardOne(npc, squad[i]);
        }
    }
}

namespace
{
void ClearTempEnemyPairGuarded(Character* npc, Character* member)
{
    npc->clearTempEnemyStatus(member);
    member->clearTempEnemyStatus(npc);
}
} // namespace

void SafeClearTempEnemies(Character* npc, Character* const* squad, int count)
{
    if (npc == NULL)
    {
        return;
    }
    for (int i = 0; i < count; ++i)
    {
        if (squad[i] != NULL && squad[i] != npc)
        {
            TR_TRY { ClearTempEnemyPairGuarded(npc, squad[i]); }
            TR_CATCH { }
        }
    }
}

bool SafeHostileToward(Character* npc, Character* const* squad, int count)
{
    if (npc == NULL)
    {
        return false;
    }
    if (SafeInCombat(npc))
    {
        return true;
    }
    for (int i = 0; i < count; ++i)
    {
        bool marked = false;
        if (squad[i] != NULL && SafeTempEnemyOf(npc, squad[i], &marked) && marked)
        {
            return true;
        }
    }
    return false;
}

void SafeMarkHostile(Character* npc, Character* playerLeader)
{
    if (npc == NULL || playerLeader == NULL)
    {
        return;
    }
    TR_TRY { npc->rememberCharacter(playerLeader, ST_TEMPORARY_ENEMY); }
    TR_CATCH { }
}

bool SafeInCombat(Character* character)
{
    if (character == NULL)
    {
        return false;
    }
    TR_TRY { return character->isInCombatMode(true, true); }
    TR_CATCH { return false; }
}
} // namespace totalrecruit
