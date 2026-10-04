#include "leave_homing_rules.h"

#include <cstdio>

namespace totalrecruit
{
namespace
{
// One optionally signed integer in [*at, end), spaces around it skipped. Advances *at.
bool ReadInt(const std::string& text, std::string::size_type* at, std::string::size_type end, int* out)
{
    std::string::size_type i = *at;
    while (i < end && text[i] == ' ')
        ++i;
    bool negative = false;
    if (i < end && (text[i] == '-' || text[i] == '+'))
    {
        negative = text[i] == '-';
        ++i;
    }
    const std::string::size_type digitsStart = i;
    long value = 0;
    while (i < end && text[i] >= '0' && text[i] <= '9')
    {
        value = value * 10 + (text[i] - '0');
        if (value > 2000000000L)
            return false;
        ++i;
    }
    if (i == digitsStart)
        return false;
    while (i < end && text[i] == ' ')
        ++i;
    if (i != end)
        return false;
    *out = (int)(negative ? -value : value);
    *at = i;
    return true;
}
} // namespace

HomingAction StepHoming(float sinceStartSeconds, float sinceOrderSeconds, float distanceMeters)
{
    if (distanceMeters >= 0.0f && distanceMeters <= kHomingArriveMeters)
        return Homing_Arrived;
    if (sinceStartSeconds >= kHomingTimeoutSeconds)
        return Homing_GaveUp;
    if (sinceOrderSeconds >= kHomingReissueSeconds)
        return Homing_Reissue;
    return Homing_Wait;
}

const char* HomingActionText(HomingAction action)
{
    switch (action)
    {
    case Homing_Wait:    return "on the way";
    case Homing_Reissue: return "move order re-issued";
    case Homing_Arrived: return "arrived";
    case Homing_GaveUp:  return "gave up";
    }
    return "unknown";
}

std::string FormatJoinSpot(int x, int z)
{
    char buffer[32] = { 0 };
    _snprintf_s(buffer, sizeof(buffer), _TRUNCATE, "%d,%d", x, z);
    return std::string(buffer);
}

bool ParseJoinSpot(const std::string& text, int* x, int* z)
{
    const std::string::size_type comma = text.find(',');
    if (comma == std::string::npos || text.find(',', comma + 1) != std::string::npos)
        return false;
    std::string::size_type at = 0;
    int px = 0, pz = 0;
    if (!ReadInt(text, &at, comma, &px))
        return false;
    at = comma + 1;
    if (!ReadInt(text, &at, text.size(), &pz))
        return false;
    *x = px;
    *z = pz;
    return true;
}
} // namespace totalrecruit
