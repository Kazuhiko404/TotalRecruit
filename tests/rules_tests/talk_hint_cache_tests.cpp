// Tests for src/talk_hint_cache.cpp: the per-NPC memory behind the talk hint.

#include "test_support.h"
#include "talk_hint_cache.h"

using namespace totalrecruit;

namespace
{
// Opaque keys: the cache never dereferences them.
int g_alice = 0;
int g_bob = 0;
int g_many[TalkHintCache::kMaxCapacity + 4];

void EmptyCacheMisses()
{
    const char* name = "talk hint: an empty cache knows nobody";
    TalkHintCache cache;
    bool answer = true;
    CHECK(!cache.Lookup(&g_alice, 1000, &answer));
    CHECK(cache.Size() == 0);
}

void StoredAnswerIsRemembered()
{
    const char* name = "talk hint: a stored answer is returned while fresh";
    TalkHintCache cache(2000, 8);
    cache.Store(&g_alice, 1000, true);
    cache.Store(&g_bob, 1000, false);

    bool answer = false;
    CHECK(cache.Lookup(&g_alice, 1500, &answer));
    CHECK(answer);
    answer = true;
    CHECK(cache.Lookup(&g_bob, 2999, &answer));
    CHECK(!answer);
    CHECK(cache.Size() == 2);
}

void TwoNpcsDoNotThrash()
{
    const char* name = "talk hint: asking about two NPCs in turn keeps both answers";
    TalkHintCache cache(2000, 8);
    cache.Store(&g_alice, 100, true);
    cache.Store(&g_bob, 101, false);
    bool answer = false;
    for (unsigned int t = 102; t < 112; ++t)
    {
        CHECK(cache.Lookup(&g_alice, t, &answer) && answer);
        CHECK(cache.Lookup(&g_bob, t, &answer) && !answer);
    }
}

void AnswerExpiresAfterTtl()
{
    const char* name = "talk hint: an answer older than the TTL is asked again";
    TalkHintCache cache(2000, 8);
    cache.Store(&g_alice, 1000, true);
    bool answer = false;
    CHECK(!cache.Lookup(&g_alice, 3000, &answer));
    CHECK(!cache.Lookup(&g_alice, 50000, &answer));
}

void StoreRefreshesTheClock()
{
    const char* name = "talk hint: storing again refreshes the answer and its age";
    TalkHintCache cache(2000, 8);
    cache.Store(&g_alice, 1000, true);
    cache.Store(&g_alice, 2500, false);
    bool answer = true;
    CHECK(cache.Lookup(&g_alice, 4000, &answer));
    CHECK(!answer);
    CHECK(cache.Size() == 1);
}

void WraparoundIsSafe()
{
    const char* name = "talk hint: the 49-day GetTickCount wrap does not freeze an answer";
    TalkHintCache cache(2000, 8);
    unsigned int now = 0xFFFFFF00u;
    cache.Store(&g_alice, now, true);
    now += 1000u; // wrapped past zero to a small value
    bool answer = false;
    CHECK(cache.Lookup(&g_alice, now, &answer));
    CHECK(answer);
    now += 2000u;
    CHECK(!cache.Lookup(&g_alice, now, &answer));
}

void FullCacheEvictsTheOldest()
{
    const char* name = "talk hint: a full cache replaces the entry stored longest ago";
    TalkHintCache cache(10000, 3);
    cache.Store(&g_many[0], 100, true);
    cache.Store(&g_many[1], 200, true);
    cache.Store(&g_many[2], 300, true);
    cache.Store(&g_many[0], 400, true); // refreshed: now [1] is the oldest
    cache.Store(&g_many[3], 500, false);

    bool answer = false;
    CHECK(cache.Size() == 3);
    CHECK(!cache.Lookup(&g_many[1], 600, &answer));
    CHECK(cache.Lookup(&g_many[0], 600, &answer));
    CHECK(cache.Lookup(&g_many[2], 600, &answer));
    CHECK(cache.Lookup(&g_many[3], 600, &answer) && !answer);
}

void CapacityIsBounded()
{
    const char* name = "talk hint: the cache never grows past its maximum";
    TalkHintCache cache(10000, 100000);
    for (size_t i = 0; i < TalkHintCache::kMaxCapacity + 4; ++i)
    {
        cache.Store(&g_many[i], (unsigned int)(100 + i), true);
    }
    CHECK(cache.Size() == TalkHintCache::kMaxCapacity);
    bool answer = false;
    CHECK(!cache.Lookup(&g_many[0], 1000, &answer));
    CHECK(cache.Lookup(&g_many[TalkHintCache::kMaxCapacity + 3], 1000, &answer));

    TalkHintCache tiny(10000, 0); // clamped to one entry
    tiny.Store(&g_alice, 1, true);
    tiny.Store(&g_bob, 2, true);
    CHECK(tiny.Size() == 1);
    CHECK(tiny.Lookup(&g_bob, 3, &answer));
}

void ClearForgetsEverything()
{
    const char* name = "talk hint: clearing (a load) forgets every answer";
    TalkHintCache cache;
    cache.Store(&g_alice, 1000, true);
    cache.Store(&g_bob, 1000, true);
    cache.Clear();
    bool answer = false;
    CHECK(cache.Size() == 0);
    CHECK(!cache.Lookup(&g_alice, 1001, &answer));
    CHECK(!cache.Lookup(&g_bob, 1001, &answer));
}
} // namespace

void RunTalkHintCacheTests()
{
    EmptyCacheMisses();
    StoredAnswerIsRemembered();
    TwoNpcsDoNotThrash();
    AnswerExpiresAfterTtl();
    StoreRefreshesTheClock();
    WraparoundIsSafe();
    FullCacheEvictsTheOldest();
    CapacityIsBounded();
    ClearForgetsEverything();
}
