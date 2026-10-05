#include "hold_duration_rules.h"

#include <cmath>
#include <cstdio>

namespace totalrecruit
{
namespace
{
// A hair under a whole minute after float arithmetic (2.2499999 h) is that minute.
const double kMinuteSlack = 0.001;
const int kMinutesPerHour = 60;
const int kMinutesPerDay = 24 * 60;
} // namespace

std::string FormatHoldDuration(float hours)
{
    int minutes = 0;
    if (hours > 0.0f)
    {
        // Far beyond any save's age; keeps the minute count inside an int.
        if (hours > 1000000.0f) hours = 1000000.0f;
        minutes = (int)std::floor((double)hours * kMinutesPerHour + kMinuteSlack);
    }

    char buf[32] = { 0 };
    if (minutes < kMinutesPerHour)
    {
        _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%d min", minutes);
    }
    else if (minutes < kMinutesPerDay)
    {
        const int h = minutes / kMinutesPerHour;
        const int m = minutes % kMinutesPerHour;
        if (m == 0)
            _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%d h", h);
        else
            _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%d h %d min", h, m);
    }
    else
    {
        const int d = minutes / kMinutesPerDay;
        const int h = (minutes % kMinutesPerDay) / kMinutesPerHour;
        if (h == 0)
            _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%d d", d);
        else
            _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%d d %d h", d, h);
    }
    return buf;
}
} // namespace totalrecruit
