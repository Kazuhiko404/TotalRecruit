#include "saved_ints.h"
#include "defeats.h"
#include "feeding_ledger.h"

namespace totalrecruit
{
namespace
{
const SavedIntField kFields[] = {
    { "total_recruit.player_defeats", &SavedDefeatCount, &RestoreSavedDefeatCount },
    { "total_recruit.fed_by_player",  &SavedPlayerMeals, &RestoreSavedPlayerMeals },
};
} // namespace

int SavedIntFieldCount()
{
    return (int)(sizeof(kFields) / sizeof(kFields[0]));
}

const SavedIntField& SavedIntFieldAt(int index)
{
    return kFields[index];
}
} // namespace totalrecruit
