#include "hold_clock.h"

#include <climits>

namespace totalrecruit
{
int HoldStartToSavedTenths(double hours)
{
    const double tenths = hours * 10.0 + 0.5;
    if (!(tenths >= 1.0))
    {
        return 1;
    }
    if (tenths >= (double)INT_MAX)
    {
        return INT_MAX;
    }
    return (int)tenths;
}

double HoldStartFromSavedTenths(int tenths)
{
    return tenths <= 0 ? 0.0 : (double)tenths / 10.0;
}

void HoldClock::Observe(const CharKey& who, bool caged, double nowHours)
{
    if (!caged)
    {
        since_.erase(who);
        return;
    }

    std::map<CharKey, double>::iterator found = since_.find(who);
    if (found == since_.end() || found->second > nowHours)
    {
        // First sighting in the cage -- or the game clock went backwards (an
        // older save was loaded), in which case the recorded start lies in the
        // future and would freeze the hold at zero until time caught up.
        since_[who] = nowHours;
    }
}

float HoldClock::HoursHeld(const CharKey& who, double nowHours) const
{
    std::map<CharKey, double>::const_iterator found = since_.find(who);
    if (found == since_.end())
    {
        return 0.0f;
    }
    const double held = nowHours - found->second;
    return held > 0.0 ? (float)held : 0.0f;
}

int HoldClock::SavedStartTenths(const CharKey& who) const
{
    std::map<CharKey, double>::const_iterator found = since_.find(who);
    return found == since_.end() ? 0 : HoldStartToSavedTenths(found->second);
}

void HoldClock::RestoreStartTenths(const CharKey& who, int tenths)
{
    if (tenths <= 0)
    {
        return;
    }
    since_[who] = HoldStartFromSavedTenths(tenths);
}

bool HoldClock::IsTracked(const CharKey& who) const
{
    return since_.find(who) != since_.end();
}

int HoldClock::TrackedCount() const
{
    return (int)since_.size();
}

void HoldClock::Clear()
{
    since_.clear();
}
} // namespace totalrecruit
