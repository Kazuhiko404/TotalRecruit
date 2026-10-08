#include "care_streak_rules.h"

namespace totalrecruit
{
const int kCareStreakPerDayMin = 0;
const int kCareStreakPerDayMax = 10;
const int kCareStreakMaxMin = 0;
const int kCareStreakMaxMax = 40;

namespace
{
// Far beyond any cap: keeps a prisoner held for years from overflowing the count.
const int kStreakCeiling = 100000;

int ClampInt(int value, int low, int high)
{
    return value < low ? low : (value > high ? high : value);
}
} // namespace

int CareStreakDays(const CareStreak& stored, int today)
{
    if (stored.dayIndex < 0 || !stored.dayKept)
    {
        return 0;
    }
    if (today == stored.dayIndex)
    {
        return stored.streak;
    }
    if (today == stored.dayIndex + 1)
    {
        // The stored day is over and was kept: it joins the chain.
        return stored.streak < kStreakCeiling ? stored.streak + 1 : kStreakCeiling;
    }
    // A day with no sighting, or a clock that went back.
    return 0;
}

CareStreak ObserveCare(const CareStreak& stored, int today, bool caredFor)
{
    CareStreak next;
    next.dayIndex = today;
    if (stored.dayIndex >= 0 && today == stored.dayIndex)
    {
        next.streak = stored.streak;
        next.dayKept = stored.dayKept;
    }
    else if (stored.dayIndex >= 0 && today > stored.dayIndex)
    {
        next.streak = CareStreakDays(stored, today);
        next.dayKept = true;
    }
    else
    {
        // Never seen, or a day before the record: start afresh.
        next.streak = 0;
        next.dayKept = true;
    }

    if (!caredFor)
    {
        // One sighting that was not cared for loses the day and the chain.
        next.streak = 0;
        next.dayKept = false;
    }
    return next;
}

int CareStreakBonus(int streakDays, const Submission& sub)
{
    const int perDay = ClampInt(sub.caredForStreakPerDay, kCareStreakPerDayMin, kCareStreakPerDayMax);
    const int cap = ClampInt(sub.caredForStreakMax, kCareStreakMaxMin, kCareStreakMaxMax);
    if (streakDays <= 0 || perDay <= 0)
    {
        return 0;
    }
    // Past `cap` days even +1 a day has reached the cap: no product to overflow.
    if (streakDays >= cap)
    {
        return cap;
    }
    const int bonus = streakDays * perDay;
    return bonus > cap ? cap : bonus;
}

void CareStreakLedger::Observe(const CharKey& key, int today, bool caredFor)
{
    Entries::iterator found = entries_.find(key);
    const CareStreak stored = found != entries_.end() ? found->second : CareStreak();
    entries_[key] = ObserveCare(stored, today, caredFor);
}

int CareStreakLedger::StreakDays(const CharKey& key, int today) const
{
    Entries::const_iterator found = entries_.find(key);
    return found != entries_.end() ? CareStreakDays(found->second, today) : 0;
}

int CareStreakLedger::SavedDay(const CharKey& key) const
{
    Entries::const_iterator found = entries_.find(key);
    return found != entries_.end() && found->second.dayIndex >= 0 ? found->second.dayIndex + 1 : 0;
}

int CareStreakLedger::SavedStreak(const CharKey& key) const
{
    Entries::const_iterator found = entries_.find(key);
    return found != entries_.end() ? found->second.streak : 0;
}

int CareStreakLedger::SavedKept(const CharKey& key) const
{
    Entries::const_iterator found = entries_.find(key);
    return found != entries_.end() && found->second.dayKept ? 1 : 0;
}

void CareStreakLedger::RestoreDay(const CharKey& key, int saved)
{
    if (saved > 0)
    {
        entries_[key].dayIndex = saved - 1;
    }
}

void CareStreakLedger::RestoreStreak(const CharKey& key, int saved)
{
    if (saved > 0)
    {
        entries_[key].streak = saved < kStreakCeiling ? saved : kStreakCeiling;
    }
}

void CareStreakLedger::RestoreKept(const CharKey& key, int saved)
{
    if (saved > 0)
    {
        entries_[key].dayKept = true;
    }
}

void CareStreakLedger::Clear()
{
    entries_.clear();
}

int CareStreakLedger::Size() const
{
    return (int)entries_.size();
}
} // namespace totalrecruit
