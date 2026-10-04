// Tests for src/cage_command_rules: the button block beside the context menu, the
// watch that decides when the knockout order needs its fallback, the credit bridge
// for knockouts the player ordered, the "NOQUEAR 72%" label and Golpear's one
// capped blow.

#include "test_support.h"
#include "cage_command_rules.h"
#include "config.h"

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
    credit.Remember(prisoner, 1000u, OrderedCredit_Knockout);
    CHECK(credit.Size() == 1);
    CHECK(credit.Claim(other, 2000u) == OrderedCredit_None);
    CHECK(credit.Claim(prisoner, 2000u) == OrderedCredit_Knockout);
    CHECK(credit.Claim(prisoner, 2100u) == OrderedCredit_None);
    CHECK(credit.Size() == 0);
}

void AnExpiredOrderIsNotCredited()
{
    const char* name = "credit bridge: past the window the knockout is not the ordered one";
    OrderedKnockoutCredit credit;
    const CharKey prisoner(7u, 1u);
    credit.Remember(prisoner, 1000u, OrderedCredit_Knockout);
    CHECK(credit.Claim(prisoner, 1000u + kOrderedKnockoutWindowMs) == OrderedCredit_None);
    CHECK(credit.Size() == 0);

    // GetTickCount wraps: the difference is still the elapsed time.
    credit.Remember(prisoner, 0xFFFFFF00u, OrderedCredit_Knockout);
    CHECK(credit.Claim(prisoner, 0x00000100u) == OrderedCredit_Knockout);

    credit.Remember(prisoner, 5u, OrderedCredit_Knockout);
    credit.Clear();
    CHECK(credit.Claim(prisoner, 6u) == OrderedCredit_None);
}

// cage-beating-stress: the hook needs to know which command caused the knockout
// (Noquear's starts its rest and its stress; Golpear's blow already did both).
void TheClaimSaysWhichCommandCausedIt()
{
    const char* name = "credit bridge: the claim names the command (beat or knockout order)";
    OrderedKnockoutCredit credit;
    const CharKey a(7u, 1u);
    const CharKey b(8u, 1u);
    credit.Remember(a, 1000u, OrderedCredit_Beat);
    credit.Remember(b, 1000u, OrderedCredit_Knockout);
    CHECK(credit.Claim(a, 1500u) == OrderedCredit_Beat);
    CHECK(credit.Claim(b, 1500u) == OrderedCredit_Knockout);
    CHECK(CreditWindowMs(OrderedCredit_Beat) == kBeatCreditWindowMs);
    CHECK(CreditWindowMs(OrderedCredit_Knockout) == kOrderedKnockoutWindowMs);
    CHECK(CreditWindowMs(OrderedCredit_None) == 0u);
}

void ANewOrderRestartsTheWindow()
{
    const char* name = "credit bridge: ordering again restarts the window";
    OrderedKnockoutCredit credit;
    const CharKey prisoner(7u, 1u);
    credit.Remember(prisoner, 0u, OrderedCredit_Knockout);
    credit.Remember(prisoner, kOrderedKnockoutWindowMs, OrderedCredit_Knockout);
    CHECK(credit.Size() == 1);
    CHECK(credit.Claim(prisoner, kOrderedKnockoutWindowMs + 10u) == OrderedCredit_Knockout);
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

// R2.3: an interval between beatings, and between knockouts. Golpear rests per
// prisoner for the configured game hours after a blow, Noquear after a knockout
// that landed; each command keeps its own ledger.
void GolpearRestsForTheCooldownAfterABlow()
{
    const char* name = "cage cooldown: game hours left since the last blow, per prisoner";
    CageCooldownLedger ledger;
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

void TheRestLabelCarriesTheWait()
{
    const char* name = "rest label: the plain verb when ready, the hours left rounded up while resting";
    CHECK(RestLabel("Golpear", 0.0f) == "Golpear");
    CHECK(RestLabel("Golpear", -1.0f) == "Golpear");
    CHECK(RestLabel("Golpear", 3.2f) == "Golpear (4 h)");
    CHECK(RestLabel("GOLPEAR", 4.0f) == "GOLPEAR (4 h)");
    CHECK(RestLabel("Golpear", 0.1f) == "Golpear (1 h)");
    CHECK(RestHoursShown(0.0f) == 0);
    CHECK(RestHoursShown(0.01f) == 1);
    CHECK(RestHoursShown(5.5f) == 6);
}

// Noquear's caption: the engine's odds while it is ready, the wait while it rests
// (a resting command has no odds worth showing).
void TheKnockoutCaptionShowsTheWaitOrTheOdds()
{
    const char* name = "knockout caption: \"Noquear 72%\" when ready, \"Noquear (1 h)\" while resting";
    CHECK(KnockoutCaption("Noquear", 0.0f, true, 0.72f) == "Noquear 72%");
    CHECK(KnockoutCaption("Noquear", 0.0f, false, 0.72f) == "Noquear");
    CHECK(KnockoutCaption("Noquear", 0.4f, true, 0.72f) == "Noquear (1 h)");
    CHECK(KnockoutCaption("NOQUEAR", 1.0f, false, 0.0f) == "NOQUEAR (1 h)");
    CHECK(KnockoutCaption("Noquear", -2.0f, true, 0.5f) == "Noquear 50%");
}

// The 1 h default the user asked for ("4h es mucho"), for both commands.
void BothCooldownsDefaultToOneHour()
{
    const char* name = "config: Golpear and Noquear rest 1 game hour by default";
    const Submission sub;
    CHECK(sub.beatingCooldownHours == 1.0f);
    CHECK(sub.knockoutCooldownHours == 1.0f);
}

// The click re-reads the facts the button was shown for: a stale block or popup
// never acts on someone freed, knocked out or out of the cage since, and Golpear
// never acts while it rests.
void TheClickReChecksTheFacts()
{
    const char* name = "cage click: not allowed any more, resting (each command its own), or go";
    CageCommandFacts f = CagedAwake();
    CHECK(JudgeCageClick(f, true) == CageClick_Go);
    CHECK(JudgeCageClick(f, false) == CageClick_Go);

    f.beatHoursLeft = 2.5f;
    CHECK(JudgeCageClick(f, true) == CageClick_Resting);
    CHECK(JudgeCageClick(f, false) == CageClick_Go);
    CHECK(!BeatReady(f));
    CHECK(KnockoutReady(f));

    f = CagedAwake();
    f.knockoutHoursLeft = 0.5f;
    CHECK(JudgeCageClick(f, false) == CageClick_Resting);
    CHECK(JudgeCageClick(f, true) == CageClick_Go);
    CHECK(!KnockoutReady(f));
    CHECK(BeatReady(f));
    CHECK(CageCommandsAllowed(f));   // a rest never hides the buttons

    f = CagedAwake();
    f.down = true;
    CHECK(JudgeCageClick(f, true) == CageClick_NotAllowed);
    CHECK(JudgeCageClick(f, false) == CageClick_NotAllowed);
    f = CagedAwake();
    f.inPlayerCage = false;
    f.beatHoursLeft = 2.5f;   // not allowed wins over resting
    f.knockoutHoursLeft = 2.5f;
    CHECK(JudgeCageClick(f, true) == CageClick_NotAllowed);
    CHECK(JudgeCageClick(f, false) == CageClick_NotAllowed);
    CHECK(!KnockoutReady(f));
    f = CagedAwake();
    f.allowBeating = false;
    CHECK(JudgeCageClick(f, false) == CageClick_NotAllowed);
    CHECK(BeatReady(CagedAwake()));
}

// The bug: a part already below the floor (an earlier fight, a bleed) was "held"
// at the floor after a zero blow -- raised, so the blow healed. The hold only ever
// pulls a part the blow pushed past the floor back to it, never above where the
// blow found it.
void TheBlowNeverRaisesHealth()
{
    const char* name = "beat hold: back to the floor only if the blow crossed it, never above the start";
    // The engine took more than the cap: held at the floor.
    CHECK(BeatHeldHealth(10.0f, -70.0f, -50.0f) == -50.0f);
    // Inside the room: untouched.
    CHECK(BeatHeldHealth(10.0f, -20.0f, -50.0f) == -20.0f);
    // Already below the floor before the blow: never raised.
    CHECK(BeatHeldHealth(-80.0f, -80.0f, -50.0f) == -80.0f);
    CHECK(BeatHeldHealth(-80.0f, -85.0f, -50.0f) == -85.0f);
    // Exactly at the floor: stays.
    CHECK(BeatHeldHealth(-50.0f, -50.0f, -50.0f) == -50.0f);
    // A part already below the floor gets no blow at all.
    CHECK(CapBeatBlunt(30.0f, -80.0f, 100.0f) == 0.0f);
}

// The bug: Golpear remembered the credit for the knockout order's 2 minutes, so
// any knockout in that time (a starvation collapse) counted as the player's.
// A blow knocks out at once, if at all: its credit lasts kBeatCreditWindowMs.
void ABlowsCreditLastsOnlyAMoment()
{
    const char* name = "credit: a blow's credit lasts the short beat window, the knockout order's 2 min";
    CHECK(kBeatCreditWindowMs <= 10000u);
    CHECK(kBeatCreditWindowMs < kOrderedKnockoutWindowMs);

    OrderedKnockoutCredit credit;
    const CharKey prisoner = CharKey::FromId("p");
    credit.Remember(prisoner, 1000u, OrderedCredit_Beat);
    CHECK(credit.Claim(prisoner, 1000u + kBeatCreditWindowMs - 1u) == OrderedCredit_Beat);

    credit.Remember(prisoner, 1000u, OrderedCredit_Beat);
    CHECK(credit.Claim(prisoner, 1000u + kBeatCreditWindowMs) == OrderedCredit_None);

    // A knockout order keeps its long window.
    credit.Remember(prisoner, 1000u, OrderedCredit_Knockout);
    CHECK(credit.Claim(prisoner, 1000u + 60000u) == OrderedCredit_Knockout);

    // The newest remember decides the window: a blow after an order shortens it.
    credit.Remember(prisoner, 1000u, OrderedCredit_Knockout);
    credit.Remember(prisoner, 2000u, OrderedCredit_Beat);
    CHECK(credit.Claim(prisoner, 2000u + kBeatCreditWindowMs + 1u) == OrderedCredit_None);
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
    TheClaimSaysWhichCommandCausedIt();
    TheKnockoutLabelCarriesTheNativeOdds();
    TheFallbackKnockoutReadsTheSkillAsAFraction();
    TheBeatHitIsTheAttackersOwnDamageAsBlunt();
    AnUnreadableAttackFallsBackToTheDefaultBlow();
    TheHealthFloorIsHalfTheMaxBelowZero();
    TheBlowIsCappedAtTheFloor();
    TheBlowLandsOnTheTorso();
    GolpearRestsForTheCooldownAfterABlow();
    TheRestLabelCarriesTheWait();
    TheKnockoutCaptionShowsTheWaitOrTheOdds();
    BothCooldownsDefaultToOneHour();
    TheClickReChecksTheFacts();
    TheBlowNeverRaisesHealth();
    ABlowsCreditLastsOnlyAMoment();
}
