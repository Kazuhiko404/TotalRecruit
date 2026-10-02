#include "plea_tracker.h"

namespace totalrecruit
{
bool PleaTracker::Begin(const CharKey& who)
{
    if (active_)
    {
        return false;
    }
    active_ = true;
    who_ = who;
    return true;
}

bool PleaTracker::IsPending(const CharKey& who) const
{
    return active_ && who_ == who;
}

bool PleaTracker::HasPending() const
{
    return active_;
}

bool PleaTracker::ClearIf(const CharKey& who)
{
    if (!IsPending(who))
    {
        return false;
    }
    active_ = false;
    return true;
}

void PleaTracker::Clear()
{
    active_ = false;
}
} // namespace totalrecruit
