// Tests for src/card_dismiss_rules.cpp: a click elsewhere puts the readout card
// away, a left click on its NPC brings it back.

#include "test_support.h"
#include "card_dismiss_rules.h"

using namespace totalrecruit;

namespace
{
CardDismissInput Press(MousePress press, bool onNpc)
{
    CardDismissInput in;
    in.press = press;
    in.onNpc = onNpc;
    return in;
}

void TheFirstReadOnlyPrimes()
{
    const char* name = "TheFirstReadOnlyPrimes";
    PressEdgeState s;
    // Held when the card came up: not a press.
    CHECK(NextPressEdge(&s, true, false) == Press_None);
    CHECK(NextPressEdge(&s, true, false) == Press_None);
    CHECK(NextPressEdge(&s, false, false) == Press_None);
    CHECK(NextPressEdge(&s, true, false) == Press_Left);
}

void APressIsReportedOnlyOnItsFirstFrame()
{
    const char* name = "APressIsReportedOnlyOnItsFirstFrame";
    PressEdgeState s;
    NextPressEdge(&s, false, false);
    CHECK(NextPressEdge(&s, false, true) == Press_Right);
    CHECK(NextPressEdge(&s, false, true) == Press_None);
    CHECK(NextPressEdge(&s, false, false) == Press_None);
    CHECK(NextPressEdge(&s, true, true) == Press_Left);
}

void AResetPrimesAgain()
{
    const char* name = "AResetPrimesAgain";
    PressEdgeState s;
    NextPressEdge(&s, false, false);
    ResetPressEdges(&s);
    CHECK(NextPressEdge(&s, true, false) == Press_None);
}

void TheBodyBoxRunsFromHeadToFeet()
{
    const char* name = "TheBodyBoxRunsFromHeadToFeet";
    // 100 px tall: half-width 30.
    const BodySpan body(500, 600, 500, 500);
    CHECK(PointOnBody(500, 550, body));
    CHECK(PointOnBody(500, 500, body));
    CHECK(PointOnBody(500, 600, body));
    CHECK(PointOnBody(530, 550, body));
    CHECK(PointOnBody(470, 550, body));
    CHECK(!PointOnBody(531, 550, body));
    CHECK(!PointOnBody(469, 550, body));
    CHECK(!PointOnBody(500, 499, body));
    CHECK(!PointOnBody(500, 601, body));
}

void AFarCharacterKeepsAMinimumWidth()
{
    const char* name = "AFarCharacterKeepsAMinimumWidth";
    const BodySpan body(200, 210, 200, 200);
    CHECK(PointOnBody(210, 205, body));
    CHECK(!PointOnBody(211, 205, body));
}

void AnOutsidePressDismisses()
{
    const char* name = "AnOutsidePressDismisses";
    CHECK(NextCardDismissed(false, Press(Press_Right, false)));
    CHECK(NextCardDismissed(false, Press(Press_Left, false)));
}

void NoPressChangesNothing()
{
    const char* name = "NoPressChangesNothing";
    CHECK(!NextCardDismissed(false, CardDismissInput()));
    CHECK(NextCardDismissed(true, CardDismissInput()));
}

void APressOnAWidgetIsIgnored()
{
    const char* name = "APressOnAWidgetIsIgnored";
    CardDismissInput in = Press(Press_Left, false);
    in.overMyGui = true;
    CHECK(!NextCardDismissed(false, in));
    CHECK(NextCardDismissed(true, in));
}

void ALeftPressOnTheNpcBringsItBack()
{
    const char* name = "ALeftPressOnTheNpcBringsItBack";
    CHECK(!NextCardDismissed(true, Press(Press_Left, true)));
    CHECK(!NextCardDismissed(false, Press(Press_Left, true)));
}

void ARightPressOnTheNpcIsTheMenu()
{
    const char* name = "ARightPressOnTheNpcIsTheMenu";
    CHECK(!NextCardDismissed(false, Press(Press_Right, true)));
    CHECK(NextCardDismissed(true, Press(Press_Right, true)));
}

void APinnedCardBesideTheOfferStays()
{
    const char* name = "APinnedCardBesideTheOfferStays";
    CardDismissInput in = Press(Press_Right, false);
    in.pinnedWithOffer = true;
    CHECK(!NextCardDismissed(false, in));
}

void ANewSelectionShowsTheCard()
{
    const char* name = "ANewSelectionShowsTheCard";
    CardDismissInput in;
    in.selectionChanged = true;
    CHECK(!NextCardDismissed(true, in));
    // The click that selected someone else is not an outside click for them.
    in.press = Press_Left;
    CHECK(!NextCardDismissed(true, in));
}

void HablarShowsTheCard()
{
    const char* name = "HablarShowsTheCard";
    CardDismissInput in;
    in.pinRequested = true;
    CHECK(!NextCardDismissed(true, in));
}
} // namespace

void RunCardDismissRulesTests()
{
    TheFirstReadOnlyPrimes();
    APressIsReportedOnlyOnItsFirstFrame();
    AResetPrimesAgain();
    TheBodyBoxRunsFromHeadToFeet();
    AFarCharacterKeepsAMinimumWidth();
    AnOutsidePressDismisses();
    NoPressChangesNothing();
    APressOnAWidgetIsIgnored();
    ALeftPressOnTheNpcBringsItBack();
    ARightPressOnTheNpcIsTheMenu();
    APinnedCardBesideTheOfferStays();
    ANewSelectionShowsTheCard();
    HablarShowsTheCard();
}
