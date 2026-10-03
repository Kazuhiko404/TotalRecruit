// Tests for src/cage_command_rules: the button block beside the context menu, the
// watch that decides when a native order needs its fallback, the credit bridge
// for knockouts the player ordered, and the "NOQUEAR 72%" label.

#include "test_support.h"
#include "cage_command_rules.h"

using namespace totalrecruit;

namespace
{
CageCommandFacts CagedAwake()
{
    CageCommandFacts f;
    f.allowBeating = true;
    f.inPlayerCage = true;
    f.down = false;
    return f;
}

void ACagedAwakePrisonerGetsTheCommands()
{
    const char* name = "cage commands: a prisoner in the player's cage, awake, with beating allowed";
    CHECK(CageCommandsAllowed(CagedAwake()));
}

void NoCommandsOutsideTheCageOrForTheDown()
{
    const char* name = "cage commands: not caged, already down or beating switched off -> none";
    CageCommandFacts f = CagedAwake();
    f.inPlayerCage = false;
    CHECK(!CageCommandsAllowed(f));

    f = CagedAwake();
    f.down = true;
    CHECK(!CageCommandsAllowed(f));

    f = CagedAwake();
    f.allowBeating = false;
    CHECK(!CageCommandsAllowed(f));
}

void TheBlockIsHablarThenGolpearThenNoquear()
{
    const char* name = "block: [Hablar, Golpear, Noquear] for a caged prisoner, [Hablar] otherwise";
    const std::vector<ContextButtonKind> caged = ComposeContextBlock(true);
    CHECK(caged.size() == 3);
    CHECK(caged.size() == 3 && caged[0] == ContextButton_Talk);
    CHECK(caged.size() == 3 && caged[1] == ContextButton_Beat);
    CHECK(caged.size() == 3 && caged[2] == ContextButton_Knockout);

    const std::vector<ContextButtonKind> free = ComposeContextBlock(false);
    CHECK(free.size() == 1);
    CHECK(free.size() == 1 && free[0] == ContextButton_Talk);
}

void TheWatchWaitsThenFallsBack()
{
    const char* name = "order watch: unchanged task is pending until the timeout, then timed out";
    const int idle = 10;
    const int beat = 475;
    CHECK(JudgeOrderWatch(idle, beat, true, idle, 0, 1500) == OrderWatch_Pending);
    CHECK(JudgeOrderWatch(idle, beat, true, idle, 1499, 1500) == OrderWatch_Pending);
    CHECK(JudgeOrderWatch(idle, beat, true, idle, 1500, 1500) == OrderWatch_TimedOut);
    CHECK(JudgeOrderWatch(idle, beat, false, 0, 1500, 1500) == OrderWatch_TimedOut);
    CHECK(JudgeOrderWatch(idle, beat, false, 0, 200, 1500) == OrderWatch_Pending);
    CHECK(kOrderWatchTimeoutMs == 1500);
}

void TheWatchSeesTheOrderTaken()
{
    const char* name = "order watch: the ordered task, or any change of task, means the order was taken";
    const int idle = 10;
    const int beat = 475;
    const int walk = 3;
    CHECK(JudgeOrderWatch(idle, beat, true, beat, 100, 1500) == OrderWatch_Taken);
    CHECK(JudgeOrderWatch(idle, beat, true, walk, 100, 1500) == OrderWatch_Taken);
    // Taken wins over the timeout: the frame that sees it decides.
    CHECK(JudgeOrderWatch(idle, beat, true, beat, 4000, 1500) == OrderWatch_Taken);
    // Already on the ordered task when it went in.
    CHECK(JudgeOrderWatch(beat, beat, true, beat, 0, 1500) == OrderWatch_Taken);
}

void AnOrderedKnockoutIsCreditedOnceInsideTheWindow()
{
    const char* name = "credit bridge: an ordered knockout is claimed once, inside the window";
    OrderedKnockoutCredit credit;
    const CharKey prisoner(7u, 1u);
    const CharKey other(8u, 1u);
    credit.Remember(prisoner, 1000u);
    CHECK(credit.Size() == 1);
    CHECK(!credit.Claim(other, 2000u));
    CHECK(credit.Claim(prisoner, 2000u));
    CHECK(!credit.Claim(prisoner, 2100u));
    CHECK(credit.Size() == 0);
}

void AnExpiredOrderIsNotCredited()
{
    const char* name = "credit bridge: past the window the knockout is not the ordered one";
    OrderedKnockoutCredit credit;
    const CharKey prisoner(7u, 1u);
    credit.Remember(prisoner, 1000u);
    CHECK(!credit.Claim(prisoner, 1000u + kOrderedKnockoutWindowMs));
    CHECK(credit.Size() == 0);

    // GetTickCount wraps: the difference is still the elapsed time.
    credit.Remember(prisoner, 0xFFFFFF00u);
    CHECK(credit.Claim(prisoner, 0x00000100u));

    credit.Remember(prisoner, 5u);
    credit.Clear();
    CHECK(!credit.Claim(prisoner, 6u));
}

void ANewOrderRestartsTheWindow()
{
    const char* name = "credit bridge: ordering again restarts the window";
    OrderedKnockoutCredit credit;
    const CharKey prisoner(7u, 1u);
    credit.Remember(prisoner, 0u);
    credit.Remember(prisoner, kOrderedKnockoutWindowMs);
    CHECK(credit.Size() == 1);
    CHECK(credit.Claim(prisoner, kOrderedKnockoutWindowMs + 10u));
}

void TheKnockoutLabelCarriesTheNativeOdds()
{
    const char* name = "label: NOQUEAR 72% from a 0.72 fraction; plain without odds";
    CHECK(KnockoutLabel("NOQUEAR", true, 0.72f) == "NOQUEAR 72%");
    CHECK(KnockoutLabel("NOQUEAR", false, 0.72f) == "NOQUEAR");
    CHECK(KnockoutPercent(0.0f) == 0);
    CHECK(KnockoutPercent(1.0f) == 100);
    CHECK(KnockoutPercent(0.725f) == 73);
    CHECK(KnockoutPercent(-0.3f) == 0);
    // Already a percent.
    CHECK(KnockoutPercent(55.0f) == 55);
    CHECK(KnockoutPercent(250.0f) == 100);
}
void TheFallbackKnockoutReadsTheSkillAsAFraction()
{
    const char* name = "fallback knockout: a 0..100 skill level becomes the engine's 0..1 skill";
    CHECK(SkillFraction(0.0f) == 0.0f);
    CHECK(SkillFraction(50.0f) > 0.499f && SkillFraction(50.0f) < 0.501f);
    CHECK(SkillFraction(100.0f) == 1.0f);
    // Past the cap (gear bonuses) or below zero (unreadable) stays inside 0..1.
    CHECK(SkillFraction(140.0f) == 1.0f);
    CHECK(SkillFraction(-5.0f) == 0.0f);
}
} // namespace

void RunCageCommandRulesTests()
{
    ACagedAwakePrisonerGetsTheCommands();
    NoCommandsOutsideTheCageOrForTheDown();
    TheBlockIsHablarThenGolpearThenNoquear();
    TheWatchWaitsThenFallsBack();
    TheWatchSeesTheOrderTaken();
    AnOrderedKnockoutIsCreditedOnceInsideTheWindow();
    AnExpiredOrderIsNotCredited();
    ANewOrderRestartsTheWindow();
    TheKnockoutLabelCarriesTheNativeOdds();
    TheFallbackKnockoutReadsTheSkillAsAFraction();
}
