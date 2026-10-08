#include "beating_daily_rules.h"

namespace totalrecruit
{
const float kBeatingDiminishFactorMin = 0.3f;
const float kBeatingDiminishFactorMax = 1.0f;
const float kBeatingDailyStressCapMin = 0.1f;
const float kBeatingDailyStressCapMax = 1.0f;

namespace
{
const float kSavedScale = 1000.0f;   // the gained bar is saved in thousandths

float Clamp(float value, float low, float high)
{
    return value < low ? low : (value > high ? high : value);
}
} // namespace

BeatingDay BeatingDayFor(const BeatingDay& stored, int today)
{
    if (stored.dayIndex == today)
    {
        return stored;
    }
    BeatingDay fresh;
    fresh.dayIndex = today;
    return fresh;
}

float BeatingDiminishMultiplier(int blowsSoFar, float factor)
{
    const float f = Clamp(factor, kBeatingDiminishFactorMin, kBeatingDiminishFactorMax);
    float multiplier = 1.0f;
    for (int i = 0; i < blowsSoFar; ++i)
    {
        multiplier *= f;
    }
    return multiplier;
}

float CapBeatingGain(float gain, float gainedToday, float dailyCap)
{
    if (gain <= 0.0f)
    {
        return 0.0f;
    }
    const float cap = Clamp(dailyCap, kBeatingDailyStressCapMin, kBeatingDailyStressCapMax);
    const float left = cap - (gainedToday > 0.0f ? gainedToday : 0.0f);
    if (left <= 0.0f)
    {
        return 0.0f;
    }
    return gain < left ? gain : left;
}

float DailyBeatingGain(BeatingBlow blow, int playerLevel, int npcLevel,
                       Temperament personality, const BeatingDay& today, const Submission& sub)
{
    // 1. diminish the raw gain, 2. the per-blow clamp (inside the scaled gain),
    // 3. the daily cap (see the header).
    const float scale = BeatingDiminishMultiplier(today.blows, sub.beatingDiminishFactor);
    const float perBlow = BeatingStressGainScaled(blow, playerLevel, npcLevel, personality, scale, sub);
    return CapBeatingGain(perBlow, today.gained, sub.beatingDailyStressCap);
}

BeatingDay RecordBeating(const BeatingDay& today, float added)
{
    BeatingDay next = today;
    next.blows = today.blows + 1;
    next.gained = Clamp(today.gained + (added > 0.0f ? added : 0.0f), 0.0f, 1.0f);
    return next;
}

BeatingDay BeatingDayLedger::Today(const CharKey& key, int today) const
{
    Entries::const_iterator it = entries_.find(key);
    return BeatingDayFor(it == entries_.end() ? BeatingDay() : it->second, today);
}

void BeatingDayLedger::Record(const CharKey& key, const BeatingDay& day)
{
    entries_[key] = day;
}

int BeatingDayLedger::SavedDay(const CharKey& key) const
{
    Entries::const_iterator it = entries_.find(key);
    return (it == entries_.end() || it->second.dayIndex < 0) ? 0 : it->second.dayIndex + 1;
}

int BeatingDayLedger::SavedBlows(const CharKey& key) const
{
    Entries::const_iterator it = entries_.find(key);
    return it == entries_.end() ? 0 : it->second.blows;
}

int BeatingDayLedger::SavedGained(const CharKey& key) const
{
    Entries::const_iterator it = entries_.find(key);
    return it == entries_.end() ? 0
                                : (int)(Clamp(it->second.gained, 0.0f, 1.0f) * kSavedScale + 0.5f);
}

void BeatingDayLedger::RestoreDay(const CharKey& key, int saved)
{
    if (saved > 0)
    {
        entries_[key].dayIndex = saved - 1;
    }
}

void BeatingDayLedger::RestoreBlows(const CharKey& key, int saved)
{
    if (saved > 0)
    {
        entries_[key].blows = saved;
    }
}

void BeatingDayLedger::RestoreGained(const CharKey& key, int saved)
{
    if (saved > 0)
    {
        entries_[key].gained = Clamp((float)saved / kSavedScale, 0.0f, 1.0f);
    }
}

void BeatingDayLedger::Clear()
{
    entries_.clear();
}

int BeatingDayLedger::Size() const
{
    return (int)entries_.size();
}
} // namespace totalrecruit
