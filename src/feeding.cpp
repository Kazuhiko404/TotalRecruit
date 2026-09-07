#include "common.h"
#include "feeding.h"
#include "config.h"
#include "recruit_core.h"
#include "log.h"

#include <kenshi/Inventory.h>
#include <kenshi/Item.h>

namespace totalrecruit
{
namespace
{
// Engine pointers can already be freed, so every dereference is guarded and
// each guard keeps a POD-only frame -- C2712 counts every C++ object in the
// whole function, not just those inside the __try block.

// Walks the squad's inventories for something edible. Both food types count:
// ITEM_FOOD_RESTRICTED is what most rations are, and excluding it would leave
// the player holding food the mod refuses to see.
bool FindFoodIn(Character* holder, Item** outItem, Inventory** outInventory)
{
    Inventory* inventory = holder->inventory;
    if (inventory == NULL)
    {
        return false;
    }

    lektor<InventorySection*>& sections = inventory->getAllSections();
    for (unsigned int s = 0; s < sections.size(); ++s)
    {
        InventorySection* section = sections[s];
        if (section == NULL)
        {
            continue;
        }

        const Ogre::vector<InventorySection::SectionItem>::type& items = section->getItems();
        for (unsigned int i = 0; i < items.size(); ++i)
        {
            Item* item = items[i].item;
            if (item == NULL)
            {
                continue;
            }

            const ItemFunction func = item->itemFunction;
            if (func == ITEM_FOOD || func == ITEM_FOOD_RESTRICTED)
            {
                *outItem = item;
                *outInventory = inventory;
                return true;
            }
        }
    }
    return false;
}

bool SafeFindFoodIn(Character* holder, Item** outItem, Inventory** outInventory)
{
    TR_TRY { return FindFoodIn(holder, outItem, outInventory); }
    TR_CATCH { return false; }
}

// Searches the player's squad in order, so the first character carrying food is
// the one who gives it up.
bool FindFoodInSquad(Character** outHolder, Item** outItem, Inventory** outInventory)
{
    if (ou == NULL || ou->player == NULL)
    {
        return false;
    }

    lektor<Character*>& squad = ou->player->playerCharacters;
    for (unsigned int i = 0; i < squad.size(); ++i)
    {
        Character* member = squad[i];
        if (member == NULL)
        {
            continue;
        }

        if (SafeFindFoodIn(member, outItem, outInventory))
        {
            *outHolder = member;
            return true;
        }
    }
    return false;
}

bool SafeFindFoodInSquad(Character** outHolder, Item** outItem, Inventory** outInventory)
{
    TR_TRY { return FindFoodInSquad(outHolder, outItem, outInventory); }
    TR_CATCH { return false; }
}

// eatItem consumes the item and reports success, but it does NOT move the
// hunger value for an animal: ten pieces of meat left a bog dog at exactly the
// fullness it started with. CharacterAnimal overrides foodUpdate and has its
// own eatItemInMouth, so beasts are fed through a path this call never reaches.
//
// So the nourishment is applied here. CorpseFeeder, which feeds animals for a
// living, does the same thing -- its config sets nutrition per body and a feed
// rate outright rather than trusting the engine to credit it.
bool SafeEat(Character* captive, Item* food, Inventory* from, bool* outAte)
{
    TR_TRY
    {
        *outAte = captive->eatItem(food, from);
        return true;
    }
    TR_CATCH { return false; }
}

// Read separately before and after the meal so the credit can be skipped when
// the engine did move the value on its own -- which it does for people.
bool SafeReadHunger(Character* character, float* out)
{
    TR_TRY
    {
        MedicalSystem* medical = character->getMedical();
        if (medical == NULL)
        {
            return false;
        }
        *out = medical->hunger;
        return true;
    }
    TR_CATCH { return false; }
}

// Never past a full stomach: 3.0 is the ceiling, confirmed by KEP writing that
// value to repair a broken medical state and by CorpseFeeder documenting the
// same scale (3.00 ~= 300 UI hunger).
bool SafeAddHunger(Character* character, float amount)
{
    TR_TRY
    {
        MedicalSystem* medical = character->getMedical();
        if (medical == NULL)
        {
            return false;
        }
        float value = medical->hunger + amount;
        if (value > 3.0f) value = 3.0f;
        if (value < 0.0f) value = 0.0f;
        medical->hunger = value;
        return true;
    }
    TR_CATCH { return false; }
}

// The name is copied out rather than referenced, because getName returns by
// value and the temporary dies with the call.
void CopyItemName(Item* item, char* buffer, size_t size)
{
    strncpy_s(buffer, size, item->getName().c_str(), _TRUNCATE);
}

bool SafeItemName(Item* item, char* buffer, size_t size)
{
    TR_TRY
    {
        CopyItemName(item, buffer, size);
        return true;
    }
    TR_CATCH { return false; }
}
} // namespace

bool PlayerHasFood()
{
    Character* holder = NULL;
    Item* food = NULL;
    Inventory* inventory = NULL;
    return SafeFindFoodInSquad(&holder, &food, &inventory);
}

bool FeedCaptive(Character* captive, std::string* outMessage)
{
    if (captive == NULL)
    {
        if (outMessage) *outMessage = "no one to feed";
        return false;
    }

    Character* holder = NULL;
    Item* food = NULL;
    Inventory* inventory = NULL;
    if (!SafeFindFoodInSquad(&holder, &food, &inventory))
    {
        if (outMessage) *outMessage = "you have no food to give";
        return false;
    }

    char itemName[128] = { 0 };
    SafeItemName(food, itemName, sizeof(itemName));

    float before = 0.0f;
    const bool haveBefore = SafeReadHunger(captive, &before);

    bool ate = false;
    if (!SafeEat(captive, food, inventory, &ate) || !ate)
    {
        if (outMessage) *outMessage = "they would not take it";
        return false;
    }

    // Credit the meal only if the engine did not. A person eating through
    // eatItem is fed by the engine already; doubling it would empty the
    // player's pack for half the meals.
    float after = before;
    const bool haveAfter = SafeReadHunger(captive, &after);
    const bool engineFed = haveBefore && haveAfter && (after > before + 0.001f);

    if (haveBefore && haveAfter && !engineFed)
    {
        SafeAddHunger(captive, GetConfig().animals.nutritionPerMeal);
        SafeReadHunger(captive, &after);
    }

    const std::string name = CharacterName(captive);
    {
        char buf[160] = { 0 };
        _snprintf_s(buf, sizeof(buf), _TRUNCATE,
                    " with %s. hunger %.2f -> %.2f (%s)",
                    itemName, before, after,
                    engineFed ? "engine" : "credited by plugin");
        LogInfo("Fed " + name + buf);
    }
    if (outMessage)
    {
        *outMessage = "fed them " + std::string(itemName);
    }
    return true;
}
} // namespace totalrecruit
