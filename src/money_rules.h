#pragma once

// Moving cats, without the engine. A purse is anything that can report a balance
// and be debited or credited; the plugin supplies engine-backed ones
// (cats_purse.h) and the tests supply fakes. The rules here are the part that
// went wrong when every call site did its own arithmetic: who pays whom, whether
// the money was there, and what happens to the cats when half a transfer fails.

namespace totalrecruit
{
class CatsPurse
{
public:
    virtual ~CatsPurse() {}

    // False when the balance cannot be read.
    virtual bool Balance(int* out) const = 0;
    // Takes `amount` (> 0). False, and nothing taken, when the funds are not there.
    virtual bool Debit(int amount) = 0;
    // Adds `amount` (> 0). False when it could not be added.
    virtual bool Credit(int amount) = 0;
};

// True when `purse` holds at least `price`. A price of 0 or less is always
// affordable. An unreadable balance is not a "no": the debit itself decides.
bool CanAfford(const CatsPurse& purse, int price);

enum ChargeResult
{
    Charge_Free,      // nothing was due
    Charge_Paid,      // debited in full
    Charge_Short,     // the purse is readable and holds less than the price
    Charge_Failed     // the debit was refused
};

// Takes `price` from the player's purse, all or nothing.
ChargeResult ChargePrice(CatsPurse& payer, int price);

// Hands back a charge whose purchase did not happen.
bool RefundCharge(CatsPurse& payer, int amount);

// Moves exactly `amount` from one purse to another and returns what moved: 0 when
// it did not (a non-positive amount, short funds, a balance that would overflow, a
// refused step). Never mints and never destroys: if the credit fails after the
// debit, the debit is put back.
int TransferCats(CatsPurse& from, CatsPurse& to, int amount);

// Moves everything `from` holds. Returns what moved (0 if it held nothing).
int TransferAllCats(CatsPurse& from, CatsPurse& to);
} // namespace totalrecruit
