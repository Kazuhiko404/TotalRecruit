// Tests for src/money_rules.cpp: charging the player and moving cats between
// characters, against fake purses. The engine glue (cats_purse.cpp) is covered
// by QA.md; what is pinned here is the arithmetic and the failure handling that
// used to be repeated at every call site.

#include "test_support.h"
#include "money_rules.h"

#include <climits>

using namespace totalrecruit;

namespace
{
// A purse with the same contract as the engine's: a debit that cannot be covered
// takes nothing and says so.
class FakePurse : public CatsPurse
{
public:
    explicit FakePurse(int cats)
        : cats_(cats), readable_(true), failCredit_(false), failDebit_(false)
    {
    }

    virtual bool Balance(int* out) const
    {
        if (!readable_) return false;
        *out = cats_;
        return true;
    }
    virtual bool Debit(int amount)
    {
        if (failDebit_ || amount <= 0 || amount > cats_) return false;
        cats_ -= amount;
        return true;
    }
    virtual bool Credit(int amount)
    {
        if (failCredit_ || amount <= 0) return false;
        cats_ += amount;
        return true;
    }

    int cats_;
    bool readable_;
    bool failCredit_;
    bool failDebit_;
};

void AffordabilityRules()
{
    const char* name = "money: can the purse cover the price";
    FakePurse purse(1000);
    CHECK(CanAfford(purse, 1000));
    CHECK(CanAfford(purse, 999));
    CHECK(!CanAfford(purse, 1001));
    CHECK(CanAfford(purse, 0));        // nothing due
    CHECK(CanAfford(purse, -5));
    FakePurse broke(0);
    CHECK(!CanAfford(broke, 1));
    FakePurse blind(10);
    blind.readable_ = false;
    CHECK(CanAfford(blind, 500));      // unknown balance is not a refusal; the debit decides
}

void ChargingIsAllOrNothing()
{
    const char* name = "money: a charge takes the whole price or nothing";
    FakePurse purse(1000);
    CHECK(ChargePrice(purse, 400) == Charge_Paid);
    CHECK(purse.cats_ == 600);

    CHECK(ChargePrice(purse, 601) == Charge_Short);
    CHECK(purse.cats_ == 600);         // nothing taken on a short purse

    CHECK(ChargePrice(purse, 0) == Charge_Free);
    CHECK(ChargePrice(purse, -10) == Charge_Free);
    CHECK(purse.cats_ == 600);         // a negative price never pays the player

    FakePurse refusing(1000);
    refusing.failDebit_ = true;
    CHECK(ChargePrice(refusing, 100) == Charge_Failed);
    CHECK(refusing.cats_ == 1000);

    FakePurse blind(1000);
    blind.readable_ = false;
    CHECK(ChargePrice(blind, 100) == Charge_Paid);      // the debit's own answer decides
    FakePurse blindShort(50);
    blindShort.readable_ = false;
    CHECK(ChargePrice(blindShort, 100) == Charge_Failed);
    CHECK(blindShort.cats_ == 50);
}

void RefundRestoresTheCharge()
{
    const char* name = "money: a refund gives back exactly the charge";
    FakePurse purse(1000);
    CHECK(ChargePrice(purse, 400) == Charge_Paid);
    CHECK(RefundCharge(purse, 400));
    CHECK(purse.cats_ == 1000);
    CHECK(!RefundCharge(purse, 0));
    CHECK(!RefundCharge(purse, -1));
    CHECK(purse.cats_ == 1000);
}

void TransferMovesExactlyTheAmount()
{
    const char* name = "money: a transfer debits one side and credits the other by the same amount";
    FakePurse npc(700);
    FakePurse player(50);
    CHECK(TransferCats(npc, player, 300) == 300);
    CHECK(npc.cats_ == 400);
    CHECK(player.cats_ == 350);
    CHECK(npc.cats_ + player.cats_ == 750);        // nothing minted, nothing lost
}

void TransferRefusesBadAmounts()
{
    const char* name = "money: a transfer never runs backwards or past the funds";
    FakePurse npc(100);
    FakePurse player(100);
    CHECK(TransferCats(npc, player, 0) == 0);
    CHECK(TransferCats(npc, player, -50) == 0);    // a negative amount must not pull cats from the player
    CHECK(TransferCats(npc, player, 101) == 0);    // more than they carry
    CHECK(npc.cats_ == 100);
    CHECK(player.cats_ == 100);
}

void FailedCreditPutsTheDebitBack()
{
    const char* name = "money: if the credit fails the cats go back to the giver";
    FakePurse npc(500);
    FakePurse player(0);
    player.failCredit_ = true;
    CHECK(TransferCats(npc, player, 500) == 0);
    CHECK(npc.cats_ == 500);
    CHECK(player.cats_ == 0);
}

void FailedDebitCreditsNothing()
{
    const char* name = "money: if the debit fails nothing is credited";
    FakePurse npc(500);
    npc.failDebit_ = true;
    FakePurse player(0);
    CHECK(TransferCats(npc, player, 200) == 0);
    CHECK(player.cats_ == 0);
}

void TransferDoesNotOverflowTheReceiver()
{
    const char* name = "money: a transfer cannot wrap the receiver's balance";
    FakePurse npc(1000);
    FakePurse rich(INT_MAX - 10);
    CHECK(TransferCats(npc, rich, 500) == 0);
    CHECK(npc.cats_ == 1000);
    CHECK(rich.cats_ == INT_MAX - 10);
    CHECK(TransferCats(npc, rich, 10) == 10);      // exactly fits
    CHECK(rich.cats_ == INT_MAX);
}

void TransferAllTakesTheWholePurse()
{
    const char* name = "money: handing over everything they carry";
    FakePurse npc(1234);
    FakePurse player(1);
    CHECK(TransferAllCats(npc, player) == 1234);
    CHECK(npc.cats_ == 0);
    CHECK(player.cats_ == 1235);

    FakePurse empty(0);
    CHECK(TransferAllCats(empty, player) == 0);
    CHECK(player.cats_ == 1235);

    FakePurse blind(900);
    blind.readable_ = false;
    CHECK(TransferAllCats(blind, player) == 0);    // cannot know how much to move
    CHECK(blind.cats_ == 900);
}
} // namespace

void RunMoneyRulesTests()
{
    AffordabilityRules();
    ChargingIsAllOrNothing();
    RefundRestoresTheCharge();
    TransferMovesExactlyTheAmount();
    TransferRefusesBadAmounts();
    FailedCreditPutsTheDebitBack();
    FailedDebitCreditsNothing();
    TransferDoesNotOverflowTheReceiver();
    TransferAllTakesTheWholePurse();
}
