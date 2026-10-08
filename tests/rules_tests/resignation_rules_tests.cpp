// Tests for src/resignation_rules.cpp: resignation in the Balanced cage
// (balanced-tuning T4), full game days held turned into a capped chance term
// divided by resolve.

#include "test_support.h"
#include "resignation_rules.h"
#include "chance_rules.h"
#include "config.h"

using namespace totalrecruit;

namespace
{
const float kDay = 24.0f;

Submission Resignation(int perDay, int maxPct)
{
    Submission sub;
    sub.resignationPerDay = perDay;
    sub.resignationMax = maxPct;
    return sub;
}

void ShippedResignationIsOneADayUpToTen()
{
    const char* name = "resignation: the shipped values are +1 a day up to +10";
    const Submission sub;
    CHECK(sub.resignationPerDay == 1);
    CHECK(sub.resignationMax == 10);
    CHECK(kResignationPerDayMin == 0 && kResignationPerDayMax == 5);
    CHECK(kResignationMaxMin == 0 && kResignationMaxMax == 30);
}

void OnlyFullDaysCount()
{
    const char* name = "resignation: only full game days held count";
    CHECK(FullDaysHeld(0.0f) == 0);
    CHECK(FullDaysHeld(-5.0f) == 0);
    CHECK(FullDaysHeld(23.9f) == 0);
    CHECK(FullDaysHeld(24.0f) == 1);
    CHECK(FullDaysHeld(71.9f) == 2);
    CHECK(FullDaysHeld(72.0f) == 3);
    CHECK(FullDaysHeld(14.0f * kDay + 5.0f) == 14);
}

void ANormalPrisonerResignsOneADay()
{
    const char* name = "resignation: a normal prisoner gains +1 a full day, capped at +10";
    const Submission sub;
    CHECK(ResignationChance(0.0f, Temperament_Normal, sub) == 0);
    CHECK(ResignationChance(23.0f, Temperament_Normal, sub) == 0);
    CHECK(ResignationChance(kDay, Temperament_Normal, sub) == 1);
    CHECK(ResignationChance(7.0f * kDay + 2.0f, Temperament_Normal, sub) == 7);
    CHECK(ResignationChance(10.0f * kDay, Temperament_Normal, sub) == 10);
    CHECK(ResignationChance(30.0f * kDay, Temperament_Normal, sub) == 10);
}

void ResolveDividesIt()
{
    const char* name = "resignation: divided by resolve like stress and defeats, never past the cap";
    const Submission sub;
    // brave 1.6: 7 / 1.6 = 4.4 -> 4; the cap 10 / 1.6 = 6.
    CHECK(ResignationChance(7.0f * kDay, Temperament_Brave, sub) == 4);
    CHECK(ResignationChance(30.0f * kDay, Temperament_Brave, sub) == 6);
    // honourable 1.8: 10 / 1.8 = 5.
    CHECK(ResignationChance(30.0f * kDay, Temperament_Honorable, sub) == 5);
    // fearful 0.6: 3 / 0.6 = 5; 10 / 0.6 = 16 would pass the cap: 10.
    CHECK(ResignationChance(3.0f * kDay, Temperament_Fearful, sub) == 5);
    CHECK(ResignationChance(30.0f * kDay, Temperament_Fearful, sub) == 10);
}

void TheConfigDecidesTheRate()
{
    const char* name = "resignation: per day and cap come from the config, clamped to their ranges";
    CHECK(ResignationChance(3.0f * kDay, Temperament_Normal, Resignation(2, 10)) == 6);
    CHECK(ResignationChance(30.0f * kDay, Temperament_Normal, Resignation(2, 4)) == 4);
    CHECK(ResignationChance(30.0f * kDay, Temperament_Normal, Resignation(0, 10)) == 0);
    CHECK(ResignationChance(30.0f * kDay, Temperament_Normal, Resignation(2, 0)) == 0);
    CHECK(ResignationChance(1.0f * kDay, Temperament_Normal, Resignation(50, 99)) == kResignationPerDayMax);
    CHECK(ResignationChance(100.0f * kDay, Temperament_Normal, Resignation(50, 99)) == kResignationMaxMax);
    CHECK(ResignationChance(30.0f * kDay, Temperament_Normal, Resignation(-3, 10)) == 0);
    CHECK(ResignationChance(30.0f * kDay, Temperament_Normal, Resignation(1, -4)) == 0);
}

void JoinChanceAddsIt()
{
    const char* name = "resignation: JoinChance adds the term as it is";
    const Config config;
    ChanceInputs in;
    in.npcLevel = 40;
    in.playerLevel = 40;
    in.situation = config.chance.intimidationBonus;
    const int without = JoinChance(in, config.chance, config.submission);
    in.resignation = 7;
    CHECK(JoinChance(in, config.chance, config.submission) == without + 7);
}
} // namespace

void RunResignationRulesTests()
{
    ShippedResignationIsOneADayUpToTen();
    OnlyFullDaysCount();
    ANormalPrisonerResignsOneADay();
    ResolveDividesIt();
    TheConfigDecidesTheRate();
    JoinChanceAddsIt();
}
