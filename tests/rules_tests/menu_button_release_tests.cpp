// Port of TotalLoot/tests/click_gesture_tests.cpp: one context-menu gesture acts
// exactly once, whether it is a click or the hold-right-and-release of the menu.

#include "test_support.h"
#include "menu_button_release.h"

using namespace totalrecruit::clickgesture;

namespace
{
void OrdinaryClickActsOnceThenEchoes()
{
    const char* name = "OrdinaryClickActsOnceThenEchoes";
    State s;
    Arm(&s);
    CHECK(Classify(&s) == ACT);     // press
    CHECK(Classify(&s) == ECHO);    // release of the same click
    CHECK(Classify(&s) == STRAY);   // anything after that
}

void ReleaseOnlyGestureActs()
{
    // Right button held to open the menu, released over the button: the press
    // happened outside, so only the release arrives.
    const char* name = "ReleaseOnlyGestureActs";
    State s;
    Arm(&s);
    CHECK(Classify(&s) == ACT);
}

void NothingArmedNeverActs()
{
    // The click that opened the menu, or a stale event after the menu closed.
    const char* name = "NothingArmedNeverActs";
    State s;
    CHECK(Classify(&s) == STRAY);
    CHECK(Classify(&s) == STRAY);
}

void RearmingStartsAFreshGesture()
{
    const char* name = "RearmingStartsAFreshGesture";
    State s;
    Arm(&s);
    CHECK(Classify(&s) == ACT);     // release-only gesture, no echo follows
    Arm(&s);                        // next menu: stale echo must not survive
    CHECK(Classify(&s) == ACT);
    CHECK(Classify(&s) == ECHO);
}

void ArmingTwiceDoesNotActTwice()
{
    // The menu hook can re-arm while it is open and unspent.
    const char* name = "ArmingTwiceDoesNotActTwice";
    State s;
    Arm(&s);
    Arm(&s);
    CHECK(Classify(&s) == ACT);
    CHECK(Classify(&s) == ECHO);
    CHECK(Classify(&s) == STRAY);
}
} // namespace

void RunMenuButtonReleaseTests()
{
    OrdinaryClickActsOnceThenEchoes();
    ReleaseOnlyGestureActs();
    NothingArmedNeverActs();
    RearmingStartsAFreshGesture();
    ArmingTwiceDoesNotActTwice();
}
