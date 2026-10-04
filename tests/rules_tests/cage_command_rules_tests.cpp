// Tests for src/cage_command_rules: the button block beside the context menu, the
// watch that decides when the knockout order needs its fallback, the credit bridge
// for knockouts the player ordered, the "NOQUEAR 72%" label and Golpear's one
// capped blow.

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

bool Near(float a, float b)
{
    const float d = a - b;
    return d > -0.001f && d < 0.001f;
}

void TheBeatHitIsTheAttackersOwnDamageAsBlunt()
{
    const char* name = "beat hit: the engine's cut + blunt + pierce for this attacker lands as blunt";
    CHECK(Near(BeatBlunt(true, 0.0f, 12.0f, 0.0f), 12.0f));
    CHECK(Near(BeatBlunt(true, 30.0f, 5.0f, 2.0f), 37.0f));
    // Negative components (never seen, but read raw) do not lower the blow.
    CHECK(Near(BeatBlunt(true, -4.0f, 10.0f, 0.0f), 10.0f));
}

void AnUnreadableAttackFallsBackToTheDefaultBlow()
{
    const char* name = "beat hit: unreadable or zero attack damage -> kBeatDefaultBlunt";
    CHECK(kBeatDefaultBlunt > 0.0f);
    CHECK(Near(BeatBlunt(false, 30.0f, 5.0f, 2.0f), kBeatDefaultBlunt));
    CHECK(Near(BeatBlunt(true, 0.0f, 0.0f, 0.0f), kBeatDefaultBlunt));
}

void TheHealthFloorIsHalfTheMaxBelowZero()
{
    const char* name = "beat hit: the part never goes below -0.5 x its max health";
    CHECK(Near(kBeatHealthFloorFraction, 0.5f));
    CHECK(Near(BeatHealthFloor(100.0f), -50.0f));
    CHECK(Near(BeatHealthFloor(80.0f), -40.0f));
    // An unreadable max gives the safest floor there is: zero.
    CHECK(Near(BeatHealthFloor(0.0f), 0.0f));
    CHECK(Near(BeatHealthFloor(-10.0f), 0.0f));
}

void TheBlowIsCappedAtTheFloor()
{
    const char* name = "beat hit: the blow lands whole above the floor, capped at it, nothing below it";
    // Plenty of health: the whole blow.
    CHECK(Near(CapBeatBlunt(20.0f, 100.0f, 100.0f), 20.0f));
    // Would cross the floor (-50): only down to it.
    CHECK(Near(CapBeatBlunt(80.0f, 10.0f, 100.0f), 60.0f));
    CHECK(Near(CapBeatBlunt(80.0f, -30.0f, 100.0f), 20.0f));
    // Already at or below the floor: nothing lands.
    CHECK(Near(CapBeatBlunt(20.0f, -50.0f, 100.0f), 0.0f));
    CHECK(Near(CapBeatBlunt(20.0f, -70.0f, 100.0f), 0.0f));
    // A negative blow never heals.
    CHECK(Near(CapBeatBlunt(-5.0f, 50.0f, 100.0f), 0.0f));
    // Ten blows in a row never get below the floor.
    float health = 100.0f;
    for (int i = 0; i < 10; ++i)
    {
        health -= CapBeatBlunt(37.0f, health, 100.0f);
    }
    CHECK(health >= BeatHealthFloor(100.0f) - 0.001f);
    CHECK(Near(health, -50.0f));
}

BeatPartFacts Part(int type, bool fatal)
{
    BeatPartFacts p;
    p.present = true;
    p.type = type;
    p.fatal = fatal;
    return p;
}

void TheBlowLandsOnTheTorso()
{
    const char* name = "beat hit: first torso part, else first fatal part, else none";
    const int head = 3;
    const int leg = 1;
    BeatPartFacts body[4] = { Part(head, true), Part(kBeatTorsoPartType, true),
                              Part(kBeatTorsoPartType, true), Part(leg, false) };
    CHECK(kBeatTorsoPartType == 0);
    CHECK(PickBeatPart(body, 4) == 1);

    // A missing slot is skipped.
    body[1].present = false;
    CHECK(PickBeatPart(body, 4) == 2);

    // No torso at all: the first fatal part.
    BeatPartFacts odd[3] = { Part(leg, false), Part(head, true), Part(leg, true) };
    CHECK(PickBeatPart(odd, 3) == 1);

    BeatPartFacts limbs[2] = { Part(leg, false), Part(leg, false) };
    CHECK(PickBeatPart(limbs, 2) == -1);
    CHECK(PickBeatPart(limbs, 0) == -1);
    CHECK(PickBeatPart(NULL, 3) == -1);
}

// R2.3: an interval between beatings. Golpear rests per prisoner for the
// configured game hours after a blow; Noquear never rests.
void GolpearRestsForTheCooldownAfterABlow()
{
    const char* name = "beat cooldown: game hours left since the last blow, per prisoner";
    BeatCooldownLedger ledger;
    const CharKey warden = CharKey::FromId("warden");
    const CharKey other = CharKey::FromId("other");
    CHECK(ledger.HoursLeft(warden, 100.0, 6.0f) == 0.0f);

    ledger.Remember(warden, 100.0);
    CHECK(ledger.HoursLeft(warden, 100.0, 6.0f) == 6.0f);
    CHECK(ledger.HoursLeft(warden, 102.0, 6.0f) == 4.0f);
    CHECK(ledger.HoursLeft(warden, 106.0, 6.0f) == 0.0f);
    CHECK(ledger.HoursLeft(warden, 130.0, 6.0f) == 0.0f);
    CHECK(ledger.HoursLeft(other, 102.0, 6.0f) == 0.0f);
    // A cooldown switched to zero, or a clock that went backwards: no wait.
    CHECK(ledger.HoursLeft(warden, 102.0, 0.0f) == 0.0f);
    CHECK(ledger.HoursLeft(warden, 99.0, 6.0f) == 0.0f);
    // A newer blow restarts it; Clear (the load boundary) forgets everyone.
    ledger.Remember(warden, 104.0);
    CHECK(ledger.HoursLeft(warden, 106.0, 6.0f) == 4.0f);
    ledger.Clear();
    CHECK(ledger.HoursLeft(warden, 106.0, 6.0f) == 0.0f);
}

void TheBeatLabelCarriesTheWait()
{
    const char* name = "beat label: the plain verb when ready, the hours left rounded up while resting";
    CHECK(BeatLabel("Golpear", 0.0f) == "Golpear");
    CHECK(BeatLabel("Golpear", -1.0f) == "Golpear");
    CHECK(BeatLabel("Golpear", 3.2f) == "Golpear (4 h)");
    CHECK(BeatLabel("GOLPEAR", 4.0f) == "GOLPEAR (4 h)");
    CHECK(BeatLabel("Golpear", 0.1f) == "Golpear (1 h)");
    CHECK(BeatRestHoursShown(0.0f) == 0);
    CHECK(BeatRestHoursShown(0.01f) == 1);
    CHECK(BeatRestHoursShown(5.5f) == 6);
}

// The click re-reads the facts the button was shown for: a stale block or popup
// never acts on someone freed, knocked out or out of the cage since, and Golpear
// never acts while it rests.
void TheClickReChecksTheFacts()
{
    const char* name = "cage click: not allowed any more, resting (Golpear only), or go";
    CageCommandFacts f = CagedAwake();
    CHECK(JudgeCageClick(f, true) == CageClick_Go);
    CHECK(JudgeCageClick(f, false) == CageClick_Go);

    f.beatHoursLeft = 2.5f;
    CHECK(JudgeCageClick(f, true) == CageClick_Resting);
    CHECK(JudgeCageClick(f, false) == CageClick_Go);
    CHECK(!BeatReady(f));

    f = CagedAwake();
    f.down = true;
    CHECK(JudgeCageClick(f, true) == CageClick_NotAllowed);
    CHECK(JudgeCageClick(f, false) == CageClick_NotAllowed);
    f = CagedAwake();
    f.inPlayerCage = false;
    f.beatHoursLeft = 2.5f;   // not allowed wins over resting
    CHECK(JudgeCageClick(f, true) == CageClick_NotAllowed);
    f = CagedAwake();
    f.allowBeating = false;
    CHECK(JudgeCageClick(f, false) == CageClick_NotAllowed);
    CHECK(BeatReady(CagedAwake()));
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
    TheBeatHitIsTheAttackersOwnDamageAsBlunt();
    AnUnreadableAttackFallsBackToTheDefaultBlow();
    TheHealthFloorIsHalfTheMaxBelowZero();
    TheBlowIsCappedAtTheFloor();
    TheBlowLandsOnTheTorso();
    GolpearRestsForTheCooldownAfterABlow();
    TheBeatLabelCarriesTheWait();
    TheClickReChecksTheFacts();
}
