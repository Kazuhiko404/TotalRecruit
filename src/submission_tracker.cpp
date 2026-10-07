#include "submission_tracker.h"
#include "submission_rules.h"
#include "cooldown_minutes.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

namespace totalrecruit
{
// ------------------------------------------------------------ The table
void SubmissionMemory::MarkAsked(const CharKey& key, int defeats)
{
    asked_[key] = defeats;
}

bool SubmissionMemory::MayAsk(const CharKey& key, int defeatsNow) const
{
    std::map<CharKey, int>::const_iterator it = asked_.find(key);
    if (it == asked_.end())
        return MayAskAgain(false, 0, defeatsNow);
    return MayAskAgain(true, it->second, defeatsNow);
}

void SubmissionMemory::NoteRollFailed(const CharKey& key, int day)
{
    rollFailedDay_[key] = day;
}

bool SubmissionMemory::RollFailedOn(const CharKey& key, int day) const
{
    std::map<CharKey, int>::const_iterator it = rollFailedDay_.find(key);
    return it != rollFailedDay_.end() && it->second == day;
}

void SubmissionMemory::SetCooldownUntil(const CharKey& key, int untilHour)
{
    if (untilHour <= 0)
    {
        cooldownUntil_.erase(key);
        return;
    }
    cooldownUntil_[key] = untilHour;
}

int SubmissionMemory::CooldownUntil(const CharKey& key) const
{
    std::map<CharKey, int>::const_iterator it = cooldownUntil_.find(key);
    return it == cooldownUntil_.end() ? 0 : it->second;
}

void SubmissionMemory::Clear()
{
    asked_.clear();
    rollFailedDay_.clear();
    cooldownUntil_.clear();
}

// ------------------------------------------- The process-wide instance
namespace
{
SubmissionMemory g_memory;

struct MemoryMutex
{
    CRITICAL_SECTION value;
    MemoryMutex() { InitializeCriticalSection(&value); }
    ~MemoryMutex() { DeleteCriticalSection(&value); }
} g_mutex;

struct Guard
{
    Guard() { EnterCriticalSection(&g_mutex.value); }
    ~Guard() { LeaveCriticalSection(&g_mutex.value); }
private:
    Guard(const Guard&);
    Guard& operator=(const Guard&);
};
} // namespace

void MarkSubmissionAsked(const CharKey& key, int defeats)
{
    Guard lock;
    g_memory.MarkAsked(key, defeats);
}

bool MaySubmissionAsk(const CharKey& key, int defeatsNow)
{
    Guard lock;
    return g_memory.MayAsk(key, defeatsNow);
}

void NoteSubmissionRollFailed(const CharKey& key, int day)
{
    Guard lock;
    g_memory.NoteRollFailed(key, day);
}

bool SubmissionRollFailedOn(const CharKey& key, int day)
{
    Guard lock;
    return g_memory.RollFailedOn(key, day);
}

void SetRejectCooldownUntil(const CharKey& key, int untilMinute)
{
    Guard lock;
    g_memory.SetCooldownUntil(key, untilMinute);
}

int RejectCooldownUntil(const CharKey& key)
{
    Guard lock;
    return g_memory.CooldownUntil(key);
}

int SavedRejectCooldown(const std::string& persistedId)
{
    if (persistedId.empty())
        return 0;
    return RejectCooldownUntil(CharKey::FromId(persistedId));
}

void RestoreSavedRejectCooldown(const std::string& persistedId, int untilMinute)
{
    if (persistedId.empty())
        return;
    SetRejectCooldownUntil(CharKey::FromId(persistedId), SanitiseSavedCooldown(untilMinute));
}

int SavedRejectCooldownLegacyHour(const std::string& persistedId)
{
    return SavedHourFromUntilMinute(SavedRejectCooldown(persistedId));
}

void RestoreSavedRejectCooldownLegacyHour(const std::string& persistedId, int untilHour)
{
    RestoreSavedRejectCooldown(persistedId, UntilMinuteFromSavedHour(SanitiseSavedCooldown(untilHour)));
}

void ForgetAllSubmissionMemory()
{
    Guard lock;
    g_memory.Clear();
}
} // namespace totalrecruit
