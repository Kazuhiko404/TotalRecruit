#include "talk_hint_cache.h"

namespace totalrecruit
{
namespace
{
size_t ClampCapacity(size_t capacity)
{
    if (capacity < 1)
    {
        return 1;
    }
    return capacity > TalkHintCache::kMaxCapacity ? TalkHintCache::kMaxCapacity : capacity;
}
} // namespace

TalkHintCache::TalkHintCache(unsigned int ttlMs, size_t capacity)
    : m_ttlMs(ttlMs), m_capacity(ClampCapacity(capacity)), m_size(0)
{
}

bool TalkHintCache::Lookup(const void* who, unsigned int nowMs, bool* worthTalking) const
{
    for (size_t i = 0; i < m_size; ++i)
    {
        if (m_entries[i].who != who)
        {
            continue;
        }
        // Unsigned subtraction: the age is right across the GetTickCount wrap.
        if (nowMs - m_entries[i].storedAt >= m_ttlMs)
        {
            return false;
        }
        *worthTalking = m_entries[i].worthTalking;
        return true;
    }
    return false;
}

void TalkHintCache::Store(const void* who, unsigned int nowMs, bool worthTalking)
{
    size_t slot = m_size;
    for (size_t i = 0; i < m_size; ++i)
    {
        if (m_entries[i].who == who)
        {
            slot = i;
            break;
        }
    }

    if (slot == m_size)
    {
        if (m_size < m_capacity)
        {
            ++m_size;
        }
        else
        {
            // Full: the entry stored longest ago (the largest age) makes room.
            slot = 0;
            for (size_t i = 1; i < m_size; ++i)
            {
                if (nowMs - m_entries[i].storedAt > nowMs - m_entries[slot].storedAt)
                {
                    slot = i;
                }
            }
        }
    }

    m_entries[slot].who = who;
    m_entries[slot].storedAt = nowMs;
    m_entries[slot].worthTalking = worthTalking;
}

void TalkHintCache::Clear()
{
    m_size = 0;
}

size_t TalkHintCache::Size() const
{
    return m_size;
}
} // namespace totalrecruit
