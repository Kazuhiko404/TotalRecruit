#include "resignation_rules.h"
#include "chance_rules.h"

namespace totalrecruit
{
const int kResignationPerDayMin = 0;
const int kResignationPerDayMax = 5;
const int kResignationMaxMin = 0;
const int kResignationMaxMax = 30;

namespace
{
const float kHoursPerDay = 24.0f;
// Far beyond any cap: keeps a years-long hold from overflowing the product.
const int kDaysCeiling = 100000;

int ClampInt(int value, int low, int high)
{
    return value < low ? low : (value > high ? high : value);
}
} // namespace

int FullDaysHeld(float hoursHeld)
{
    if (!(hoursHeld > 0.0f))
    {
        return 0;
    }
    const float days = hoursHeld / kHoursPerDay;
    return days >= (float)kDaysCeiling ? kDaysCeiling : (int)days;
}

int ResignationChance(float hoursHeld, Temperament personality, const Submission& sub)
{
    const int perDay = ClampInt(sub.resignationPerDay, kResignationPerDayMin, kResignationPerDayMax);
    const int cap = ClampInt(sub.resignationMax, kResignationMaxMin, kResignationMaxMax);
    const int days = FullDaysHeld(hoursHeld);
    if (days <= 0 || perDay <= 0 || cap <= 0)
    {
        return 0;
    }
    // Past `cap` days even +1 a day has reached the cap: no product to overflow.
    const int raw = days >= cap ? cap : (days * perDay > cap ? cap : days * perDay);
    const float resolve = ResolveMultiplier(personality, sub);
    const int value = (int)((float)raw / (resolve > 0.01f ? resolve : 1.0f));
    return value > cap ? cap : value;
}
} // namespace totalrecruit
