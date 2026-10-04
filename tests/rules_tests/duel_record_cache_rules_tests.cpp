// Tests for src/duel_record_cache_rules.cpp: the readout card reads the duel record
// again when the pair changes or TotalSparring reported a bout since its copy.

#include "test_support.h"
#include "duel_record_cache_rules.h"

using namespace totalrecruit;

namespace
{
int g_npc = 0;
int g_challenger = 0;
int g_other = 0;

DuelRecordStamp Stamp(unsigned generation)
{
    DuelRecordStamp s;
    s.valid = true;
    s.npc = &g_npc;
    s.challenger = &g_challenger;
    s.generation = generation;
    return s;
}

void NoCopyIsRead()
{
    const char* name = "NoCopyIsRead";
    CHECK(DuelRecordNeedsRead(DuelRecordStamp(), &g_npc, &g_challenger, 0));
}

void TheSamePairAndGenerationKeepsTheCopy()
{
    const char* name = "TheSamePairAndGenerationKeepsTheCopy";
    CHECK(!DuelRecordNeedsRead(Stamp(3), &g_npc, &g_challenger, 3));
}

void ABoutSinceTheCopyReadsAgain()
{
    // The play-test bug: a duel leaves the pair unchanged, the record changed.
    const char* name = "ABoutSinceTheCopyReadsAgain";
    CHECK(DuelRecordNeedsRead(Stamp(3), &g_npc, &g_challenger, 4));
}

void ANewNpcOrChallengerReadsAgain()
{
    const char* name = "ANewNpcOrChallengerReadsAgain";
    CHECK(DuelRecordNeedsRead(Stamp(3), &g_other, &g_challenger, 3));
    CHECK(DuelRecordNeedsRead(Stamp(3), &g_npc, &g_other, 3));
}

void AWrappedGenerationStillReadsAgain()
{
    const char* name = "AWrappedGenerationStillReadsAgain";
    CHECK(DuelRecordNeedsRead(Stamp(0xFFFFFFFFu), &g_npc, &g_challenger, 0));
}
} // namespace

void RunDuelRecordCacheRulesTests()
{
    NoCopyIsRead();
    TheSamePairAndGenerationKeepsTheCopy();
    ABoutSinceTheCopyReadsAgain();
    ANewNpcOrChallengerReadsAgain();
    AWrappedGenerationStillReadsAgain();
}
