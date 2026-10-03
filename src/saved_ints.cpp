#include "saved_ints.h"
#include "defeats.h"
#include "feeding_ledger.h"
#include "submission_tracker.h"
#include "temp_member.h"

namespace totalrecruit
{
namespace
{
const SavedIntField kFields[] = {
    { "total_recruit.player_defeats", &SavedDefeatCount, &RestoreSavedDefeatCount },
    { "total_recruit.fed_by_player",  &SavedPlayerMeals, &RestoreSavedPlayerMeals },
    { "total_recruit.reject_cooldown_until", &SavedRejectCooldown, &RestoreSavedRejectCooldown },
    { "total_recruit.temp_expiry_hour", &SavedTempExpiryHour, &RestoreSavedTempExpiryHour },
    { "total_recruit.temp_strikes",     &SavedTempStrikes,    &RestoreSavedTempStrikes },
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
