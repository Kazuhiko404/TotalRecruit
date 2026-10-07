#include "cooldown_minutes.h"

#include <climits>
#include <cmath>
#include <cstddef>

namespace totalrecruit
{
namespace
{
const double kMinutesPerHour = 60.0;
// A float wait (1/60 h) times 60 lands a hair over a whole minute; this keeps it
// on that minute.
const double kMinuteSlack = 1e-4;
const double kHourSlack = 1e-6;

int ClampedInt(double value)
{
    if (value >= (double)INT_MAX)
    {
        return INT_MAX;
    }
    return value < 1.0 ? 1 : (int)value;
}
} // namespace

int CooldownUntilMinute(double nowHours, float cooldownHours)
{
    if (cooldownHours <= 0.0f)
    {
        return 0;
    }
    const double end = nowHours + (double)cooldownHours;
    if (cooldownHours >= 1.0f)
    {
        // The whole hour it always ended on (submission_rules RejectCooldownUntilHour).
        return ClampedInt(std::ceil(end - kHourSlack) * kMinutesPerHour);
    }
    return ClampedInt(std::ceil(end * kMinutesPerHour - kMinuteSlack));
}

bool CooldownActiveAt(int untilMinute, double nowHours, double* hoursLeft)
{
    if (hoursLeft != NULL)
    {
        *hoursLeft = 0.0;
    }
    if (untilMinute <= 0)
    {
        return false;
    }
    const double left = (double)untilMinute / kMinutesPerHour - nowHours;
    if (left <= 0.0)
    {
        return false;
    }
    if (hoursLeft != NULL)
    {
        *hoursLeft = left;
    }
    return true;
}

int UntilMinuteFromSavedHour(int untilHour)
{
    if (untilHour <= 0)
    {
        return 0;
    }
    return ClampedInt((double)untilHour * kMinutesPerHour);
}

int SavedHourFromUntilMinute(int untilMinute)
{
    if (untilMinute <= 0)
    {
        return 0;
    }
    return untilMinute / 60 + (untilMinute % 60 != 0 ? 1 : 0);
}
} // namespace totalrecruit
