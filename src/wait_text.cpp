#include "wait_text.h"
#include "messages.h"

namespace totalrecruit
{
std::string SayWait(const std::string& hoursKey, const std::string& hoursFallback,
                    const std::string& minutesFallback, Character* who, float hoursLeft,
                    int* hoursOut)
{
    const WaitShown shown = ShowWait(hoursLeft);
    if (hoursOut != NULL)
    {
        *hoursOut = WaitHoursValue(shown);
    }
    if (shown.unit == WaitUnit_Minutes)
    {
        return ReplaceMinutes(Say(WaitMinutesKey(hoursKey), minutesFallback, who), shown.value);
    }
    return Say(hoursKey, hoursFallback, who);
}

WaitTexts RestWaitTexts()
{
    const WaitTexts defaults;
    WaitTexts texts;
    texts.hours = Say("cage.button.restHours", defaults.hours, NULL);
    texts.minutes = Say("cage.button.restMinutes", defaults.minutes, NULL);
    return texts;
}
} // namespace totalrecruit
