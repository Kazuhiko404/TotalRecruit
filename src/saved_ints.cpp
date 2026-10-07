#include "saved_ints.h"
#include "defeats.h"
#include "feeding_ledger.h"
#include "healing_ledger.h"
#include "submission_tracker.h"
#include "temp_member.h"
#include "helper.h"
#include "duel_challenge_rules.h"
#include "beating_stress.h"
#include "hold_tracking.h"

namespace totalrecruit
{
namespace
{
const SavedIntField kFields[] = {
    { "total_recruit.player_defeats", &SavedDefeatCount, &RestoreSavedDefeatCount },
    { "total_recruit.fed_by_player",  &SavedPlayerMeals, &RestoreSavedPlayerMeals },
    { "total_recruit.healed_by_player", &SavedPlayerHealing, &RestoreSavedPlayerHealing },
    // Legacy whole-hour row first, then the exact minute (cooldown_minutes.h): the
    // minute row wins when a save has both.
    { "total_recruit.reject_cooldown_until", &SavedRejectCooldownLegacyHour, &RestoreSavedRejectCooldownLegacyHour },
    { "total_recruit.reject_cooldown_until_min", &SavedRejectCooldown, &RestoreSavedRejectCooldown },
    { "total_recruit.temp_expiry_hour", &SavedTempExpiryHour, &RestoreSavedTempExpiryHour },
    { "total_recruit.temp_strikes",     &SavedTempStrikes,    &RestoreSavedTempStrikes },
    { "total_recruit.helper_expiry_hour", &SavedHelperExpiryHour, &RestoreSavedHelperExpiryHour },
    { "total_recruit.helper_reject_until", &SavedHelperRejectUntil, &RestoreSavedHelperRejectUntil },
    { "total_recruit.duel_cooldown_until", &SavedDuelCooldownLegacyHour, &RestoreSavedDuelCooldownLegacyHour },
    { "total_recruit.duel_cooldown_until_min", &SavedDuelCooldownUntil, &RestoreSavedDuelCooldownUntil },
    { "total_recruit.beating_stress", &SavedBeatingStress, &RestoreSavedBeatingStress },
    { "total_recruit.beating_stress_hour", &SavedBeatingStressHour, &RestoreSavedBeatingStressHour },
    { "total_recruit.hold_start_hour", &SavedHoldStart, &RestoreSavedHoldStart },
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
