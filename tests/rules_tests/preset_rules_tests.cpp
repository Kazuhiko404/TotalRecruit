// Tests for src/preset_rules.cpp: only Cheat and Balanced exist; anything else
// (a saved 2 from the retired Vanilla+ included) loads as Balanced.

#include "test_support.h"
#include "preset_rules.h"
#include "config.h"

using namespace totalrecruit;

namespace
{
void KnownPresetsAreKept()
{
    const char* name = "preset: Cheat (0) and Balanced (1) load as they are";
    CHECK(ClampPreset(Preset_Cheat) == Preset_Cheat);
    CHECK(ClampPreset(Preset_Balanced) == Preset_Balanced);
    CHECK(Preset_Cheat == 0);
    CHECK(Preset_Balanced == 1);
    CHECK(IsKnownPreset(0));
    CHECK(IsKnownPreset(1));
}

void RetiredVanillaPlusLoadsAsBalanced()
{
    const char* name = "preset: a saved 2 (the retired Vanilla+) loads as Balanced";
    CHECK(!IsKnownPreset(2));
    CHECK(ClampPreset(2) == Preset_Balanced);
}

void AnyOutOfRangeValueLoadsAsBalanced()
{
    const char* name = "preset: negative or absurd values load as Balanced";
    CHECK(ClampPreset(-1) == Preset_Balanced);
    CHECK(ClampPreset(3) == Preset_Balanced);
    CHECK(ClampPreset(1000000) == Preset_Balanced);
    CHECK(!IsKnownPreset(-1));
}

void TheDefaultIsBalanced()
{
    const char* name = "preset: a fresh config runs Balanced";
    CHECK(Config().preset == Preset_Balanced);
}
} // namespace

void RunPresetRulesTests()
{
    KnownPresetsAreKept();
    RetiredVanillaPlusLoadsAsBalanced();
    AnyOutOfRangeValueLoadsAsBalanced();
    TheDefaultIsBalanced();
}
