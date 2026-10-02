#include "hold_clock.h"

namespace totalrecruit
{
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
