#include "care_join_rules.h"

namespace totalrecruit
{
const int kCareJoinStreakDaysMin = 0;
const int kCareJoinStreakDaysMax = 30;

int CareJoinStreakDaysNeeded(const Submission& sub)
{
    const int days = sub.careJoinStreakDays;
    if (days < kCareJoinStreakDaysMin)
    {
        return kCareJoinStreakDaysMin;
    }
    return days > kCareJoinStreakDaysMax ? kCareJoinStreakDaysMax : days;
}

int CareJoinDaysMissing(int streakDays, const Submission& sub)
{
    const int kept = streakDays > 0 ? streakDays : 0;
    const int needed = CareJoinStreakDaysNeeded(sub);
    return kept >= needed ? 0 : needed - kept;
}
} // namespace totalrecruit
