#include "beating_stress_rules.h"
#include "chance_rules.h"

namespace totalrecruit
{
const int kBeatingStressSavedScale = 1000;

namespace
{
float Clamp(float value, float low, float high)
{
    if (high < low)
    {
        high = low;
    }
    return value < low ? low : (value > high ? high : value);
}

float Clamp01(float value)
{
    return Clamp(value, 0.0f, 1.0f);
}
} // namespace

float BeatingLevelFactor(int playerLevel, int npcLevel, const Submission& sub)
{
    const float player = (float)(playerLevel < 1 ? 1 : playerLevel);
    const float npc = (float)(npcLevel < 1 ? 1 : npcLevel);
    return Clamp(player / npc, sub.beatingStressLevelMin, sub.beatingStressLevelMax);
}

float BeatingStressGain(BeatingBlow blow, int playerLevel, int npcLevel,
                        Temperament personality, const Submission& sub)
{
    const float base = (blow == BeatingBlow_Knockout) ? sub.knockoutStressBase
                                                      : sub.beatStressBase;
    if (base <= 0.0f)
    {
        return 0.0f;
    }
    const float resolve = ResolveMultiplier(personality, sub);
    const float raw = base * BeatingLevelFactor(playerLevel, npcLevel, sub) /
                      (resolve > 0.01f ? resolve : 1.0f);
    return Clamp01(Clamp(raw, sub.beatingStressMin, sub.beatingStressMax));
}

float DecayedBeatingStress(float value, double lastHours, double nowHours, float fadeHours)
{
    const float start = Clamp01(value);
    const double since = nowHours - lastHours;
    if (fadeHours <= 0.0f || since <= 0.0)
    {
        return start;
    }
    const double left = (double)start - since / (double)fadeHours;
    return left > 0.0 ? (float)left : 0.0f;
}

int BeatingStressToSaved(float value)
{
    return (int)(Clamp01(value) * (float)kBeatingStressSavedScale + 0.5f);
}

float BeatingStressFromSaved(int saved)
{
    return Clamp01((float)saved / (float)kBeatingStressSavedScale);
}

int GameHourToSavedTenths(double hours)
{
    return hours <= 0.0 ? 0 : (int)(hours * 10.0 + 0.5);
}

double GameHourFromSavedTenths(int tenths)
{
    return tenths <= 0 ? 0.0 : (double)tenths / 10.0;
}

float BeatingStressLedger::Get(const CharKey& key, double nowHours, float fadeHours) const
{
    Entries::const_iterator it = entries_.find(key);
    if (it == entries_.end())
    {
        return 0.0f;
    }
    const Entry& entry = it->second;
    return entry.haveHour ? DecayedBeatingStress(entry.value, entry.lastHours, nowHours, fadeHours)
                          : Clamp01(entry.value);
}

float BeatingStressLedger::Add(const CharKey& key, float gain, double nowHours, float fadeHours)
{
    const float left = Get(key, nowHours, fadeHours);
    Entry entry;
    entry.value = Clamp01(left + (gain > 0.0f ? gain : 0.0f));
    entry.lastHours = nowHours;
    entry.haveHour = true;
    entries_[key] = entry;
    return entry.value;
}

int BeatingStressLedger::SavedValue(const CharKey& key) const
{
    Entries::const_iterator it = entries_.find(key);
    return it == entries_.end() ? 0 : BeatingStressToSaved(it->second.value);
}

int BeatingStressLedger::SavedHourTenths(const CharKey& key) const
{
    Entries::const_iterator it = entries_.find(key);
    return (it == entries_.end() || !it->second.haveHour)
               ? 0 : GameHourToSavedTenths(it->second.lastHours);
}

void BeatingStressLedger::RestoreValue(const CharKey& key, int saved)
{
    if (saved <= 0)
    {
        return;
    }
    entries_[key].value = BeatingStressFromSaved(saved);
}

void BeatingStressLedger::RestoreHourTenths(const CharKey& key, int tenths)
{
    if (tenths <= 0)
    {
        return;
    }
    Entry& entry = entries_[key];
    entry.lastHours = GameHourFromSavedTenths(tenths);
    entry.haveHour = true;
}

void BeatingStressLedger::Clear()
{
    entries_.clear();
}

int BeatingStressLedger::Size() const
{
    return (int)entries_.size();
}
} // namespace totalrecruit
