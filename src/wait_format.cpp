#include "wait_format.h"

#include <cmath>
#include <cstdio>

namespace totalrecruit
{
namespace
{
std::string Number(int value)
{
    char buffer[16] = { 0 };
    _snprintf_s(buffer, sizeof(buffer), _TRUNCATE, "%d", value);
    return buffer;
}

void ReplaceAll(std::string* text, const std::string& token, const std::string& value)
{
    std::string::size_type at = text->find(token);
    while (at != std::string::npos)
    {
        text->replace(at, token.size(), value);
        at = text->find(token, at + value.size());
    }
}

int HoursRoundedUp(float hoursLeft)
{
    const int whole = (int)hoursLeft;
    return ((float)whole < hoursLeft) ? whole + 1 : whole;
}
} // namespace

WaitShown ShowWait(float hoursLeft)
{
    WaitShown shown;
    if (hoursLeft <= 0.0f)
    {
        return shown;
    }
    if (hoursLeft < 1.0f)
    {
        // A float wait (0.4 h, 1/60 h) times 60 lands a hair over the whole
        // minute it means; the slack keeps it there.
        int minutes = (int)std::ceil((double)hoursLeft * 60.0 - 1e-3);
        if (minutes < 1)
        {
            minutes = 1;
        }
        if (minutes < 60)
        {
            shown.unit = WaitUnit_Minutes;
            shown.value = minutes;
            return shown;
        }
    }
    shown.unit = WaitUnit_Hours;
    shown.value = HoursRoundedUp(hoursLeft);
    if (shown.value < 1)
    {
        shown.value = 1;
    }
    return shown;
}

int WaitHoursValue(const WaitShown& shown)
{
    return shown.unit == WaitUnit_Hours ? shown.value : 0;
}

std::string FillWait(const WaitTexts& texts, const WaitShown& shown)
{
    if (shown.unit == WaitUnit_Minutes)
    {
        return ReplaceMinutes(texts.minutes, shown.value);
    }
    if (shown.unit == WaitUnit_Hours)
    {
        std::string out = texts.hours;
        ReplaceAll(&out, "{hours}", Number(shown.value));
        return out;
    }
    return std::string();
}

std::string WaitMinutesKey(const std::string& hoursKey)
{
    return hoursKey + "Minutes";
}

std::string ReplaceMinutes(const std::string& text, int minutes)
{
    std::string out = text;
    ReplaceAll(&out, "{minutes}", Number(minutes));
    return out;
}
} // namespace totalrecruit
