#include "money_rules.h"

#include <climits>

namespace totalrecruit
{
bool CanAfford(const CatsPurse& purse, int price)
{
    if (price <= 0)
    {
        return true;
    }

    int balance = 0;
    if (!purse.Balance(&balance))
    {
        return true;    // unknown, not "no": the debit itself will say
    }
    return balance >= price;
}

ChargeResult ChargePrice(CatsPurse& payer, int price)
{
    if (price <= 0)
    {
        return Charge_Free;
    }

    int balance = 0;
    if (payer.Balance(&balance) && balance < price)
    {
        return Charge_Short;
    }

    return payer.Debit(price) ? Charge_Paid : Charge_Failed;
}

bool RefundCharge(CatsPurse& payer, int amount)
{
    return amount > 0 && payer.Credit(amount);
}

int TransferCats(CatsPurse& from, CatsPurse& to, int amount)
{
    if (amount <= 0)
    {
        return 0;
    }

    // The receiver must be able to hold it before anything is taken.
    int receiving = 0;
    if (to.Balance(&receiving) && receiving > INT_MAX - amount)
    {
        return 0;
    }

    if (!from.Debit(amount))
    {
        return 0;
    }

    if (!to.Credit(amount))
    {
        from.Credit(amount);    // put it back: a failed transfer destroys nothing
        return 0;
    }
    return amount;
}

int TransferAllCats(CatsPurse& from, CatsPurse& to)
{
    int held = 0;
    if (!from.Balance(&held) || held <= 0)
    {
        return 0;
    }
    return TransferCats(from, to, held);
}
} // namespace totalrecruit
