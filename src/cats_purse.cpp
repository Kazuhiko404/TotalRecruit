#include "common.h"
#include "cats_purse.h"

namespace totalrecruit
{
namespace
{
// Every engine touch is guarded and keeps a POD-only frame (C2712).
bool SafeReadBalance(Character* owner, int* out)
{
    TR_TRY
    {
        if (owner == NULL)
        {
            return false;
        }
        *out = owner->getMoney();
        return true;
    }
    TR_CATCH { return false; }
}

bool SafeDebit(Character* owner, int amount)
{
    TR_TRY
    {
        return owner != NULL && owner->takeMoney(amount);
    }
    TR_CATCH { return false; }
}

bool SafeCredit(Character* owner, int amount)
{
    TR_TRY
    {
        if (owner == NULL)
        {
            return false;
        }
        Ownerships* wallet = owner->getOwnerships();
        if (wallet == NULL)
        {
            return false;
        }
        wallet->addMoney(amount);
        return true;
    }
    TR_CATCH { return false; }
}

bool SafePlayerSquadLeader(Character** out)
{
    TR_TRY
    {
        if (ou == NULL || ou->player == NULL)
        {
            return false;
        }
        Platoon* platoon = ou->player->currentPlatoon;
        if (platoon == NULL)
        {
            return false;
        }
        *out = platoon->getSquadLeader();
        return (*out != NULL);
    }
    TR_CATCH { return false; }
}
} // namespace

bool CharacterPurse::Balance(int* out) const
{
    return SafeReadBalance(owner_, out);
}

bool CharacterPurse::Debit(int amount)
{
    return amount > 0 && SafeDebit(owner_, amount);
}

bool CharacterPurse::Credit(int amount)
{
    return amount > 0 && SafeCredit(owner_, amount);
}

Character* PlayerPurseOwner()
{
    Character* leader = NULL;
    return SafePlayerSquadLeader(&leader) ? leader : NULL;
}
} // namespace totalrecruit
