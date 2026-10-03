#pragma once

// The talk hint's memory: "is this NPC worth talking to?", remembered per NPC for a
// short while.
//
// The engine asks Dialogue::hasDialogue_Fast constantly, once per candidate NPC in
// view, and answering means a rules evaluation (cursor_hint.cpp). A one-entry cache
// thrashed as soon as two NPCs were on screen, so every frame paid a full evaluation
// per NPC. This keeps up to `capacity` answers, each trusted for `ttlMs`, and evicts
// the oldest when full. The TTL lets a verdict follow the world (an NPC fed, beaten
// or caged changes their answer within a couple of seconds) without a per-frame cost.
//
// Engine-free and clock-free: the key is an opaque pointer and the time is passed
// in (GetTickCount milliseconds, wraparound safe), so it is unit tested in
// tests/rules_tests. The key is never dereferenced. It is a raw Character*, so the
// owner clears the cache at a load (LoadBoundary), when every character is freed; a
// pointer reused inside one session can at worst show a stale hint for one TTL.

#include <cstddef>

namespace totalrecruit
{
class TalkHintCache
{
public:
    static const unsigned int kDefaultTtlMs = 2000;
    static const size_t kMaxCapacity = 64;

    // capacity is clamped to 1..kMaxCapacity.
    explicit TalkHintCache(unsigned int ttlMs = kDefaultTtlMs, size_t capacity = kMaxCapacity);

    // True, with the remembered answer, when `who` was stored less than ttlMs ago.
    bool Lookup(const void* who, unsigned int nowMs, bool* worthTalking) const;

    // Remembers (or refreshes) the answer for `who`. When full, the entry stored
    // longest ago is replaced.
    void Store(const void* who, unsigned int nowMs, bool worthTalking);

    void Clear();
    size_t Size() const;

private:
    struct Entry
    {
        const void* who;
        unsigned int storedAt;
        bool worthTalking;
    };

    unsigned int m_ttlMs;
    size_t m_capacity;
    size_t m_size;
    Entry m_entries[kMaxCapacity];
};
} // namespace totalrecruit
