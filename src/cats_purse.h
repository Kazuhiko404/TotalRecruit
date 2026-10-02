#pragma once

// The engine side of money_rules.h: a purse backed by a character.
//
// One place knows the engine's money API and its sign convention, so the price
// path, the rescue gift, the mercy tribute and a refund all move cats the same
// way. The roles are the ones that have been confirmed in play:
//   Balance  Character::getMoney()               (what the offer window compares)
//   Debit    Character::takeMoney(+n)            (true when the funds were there)
//   Credit   Ownerships::addMoney(+n)            (the only call that adds; there is no
//                                                 Character::addMoney)
// Nothing here ever passes a negative amount to the engine: crediting through
// takeMoney(-n) and debiting through addMoney(-n) both used to exist, and neither
// checked that the money was really there.

#include "money_rules.h"

class Character;

namespace totalrecruit
{
class CharacterPurse : public CatsPurse
{
public:
    explicit CharacterPurse(Character* owner) : owner_(owner) {}

    virtual bool Balance(int* out) const;
    virtual bool Debit(int amount);
    virtual bool Credit(int amount);

private:
    Character* owner_;
};

// The squad leader of the player's current squad: the purse the offer window
// reads and the price is taken from. NULL when the game has no player yet.
Character* PlayerPurseOwner();
} // namespace totalrecruit
