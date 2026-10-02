#include "common.h"
#include "rescue_memory.h"
#include "engine_reads.h"
#include "cats_purse.h"
#include "money_rules.h"
#include "log.h"

#include <sstream>

namespace totalrecruit
{
namespace
{
// Whether this NPC has already handed over their parting gift. Stored in the
// engine's own per-character memory, so it survives a reload: without it the player
// could reopen the window and be paid again by the same rescued NPC every time.
//
// LT_I_SCREWED_THIS_GUY is the closest thing the engine offers to "we have already
// settled up". Nothing else in the mod reads it.
bool AlreadyPaidTheirDebt(Character* character)
{
    Character* leader = NULL;
    if (character == NULL || !SafePlayerLeader(&leader))
    {
        return false;
    }
    bool paid = false;
    return SafeMemoryTag(character, leader, LT_I_SCREWED_THIS_GUY, &paid) && paid;
}
} // namespace

void MarkRescuedBy(Character* character, Character* saviour)
{
    if (character == NULL || saviour == NULL)
    {
        return;
    }

    // The engine stores this per-NPC and saves it with the game, so the debt
    // survives even if the player walks away before the NPC wakes up.
    if (SafeRememberTag(character, saviour, LT_MY_LIFESAVER))
    {
        LogInfo("Marked " + CharacterName(character) + " as rescued.");
    }
}

int PayPartingGift(Character* character)
{
    Character* leader = NULL;
    if (character == NULL || !SafePlayerLeader(&leader))
    {
        return 0;
    }

    if (AlreadyPaidTheirDebt(character))
    {
        return 0;
    }

    // They give what they are carrying, never a figure the mod invents. An NPC with
    // empty pockets owes the same debt and simply cannot pay it -- which is the
    // honest outcome, not a bug. The move itself is money_rules.cpp: checked debit,
    // then credit, and the debit is put back if the credit fails.
    CharacterPurse theirs(character);
    CharacterPurse mine(leader);
    const int gift = TransferAllCats(theirs, mine);
    if (gift <= 0)
    {
        return 0;
    }

    SafeRememberTag(character, leader, LT_I_SCREWED_THIS_GUY);
    std::ostringstream note;
    note << CharacterName(character) << " paid the player " << gift
         << " cats for the rescue.";
    LogInfo(note.str());
    return gift;
}

bool RemembersPlayerAsSaviour(Character* character)
{
    if (character == NULL)
    {
        return false;
    }

    Character* leader = NULL;
    if (!SafePlayerLeader(&leader))
    {
        return false;
    }

    bool remembered = false;
    if (!SafeMemoryTag(character, leader, LT_MY_LIFESAVER, &remembered))
    {
        return false;
    }
    return remembered;
}
} // namespace totalrecruit
