// Tests for src/menu_slot.cpp: the free-slot geometry for the Hablar button.
// Ported from TotalLoot/tests/menu_slot_tests.cpp with the module.

#include "test_support.h"
#include "menu_slot.h"

using namespace totalrecruit::menuslot;

namespace
{
// 1920x1080 screen, menu 200x300 at (800,400), two 120x26 buttons.
Request Base()
{
    Request r;
    r.menu = Rect(800, 400, 200, 300);
    r.screenWidth = 1920;
    r.screenHeight = 1080;
    r.buttonCount = 2;
    r.buttonWidth = 120;
    r.buttonHeight = 26;
    r.gapX = 2;
    r.gapY = 5;
    return r;
}

bool RectEq(const Rect& r, int l, int t, int w, int h)
{
    return r.left == l && r.top == t && r.width == w && r.height == h;
}

void IntersectsBasics()
{
    const char* name = "menu slot: intersects basics";
    CHECK(Intersects(Rect(0, 0, 10, 10), Rect(5, 5, 10, 10)));
    CHECK(!Intersects(Rect(0, 0, 10, 10), Rect(10, 0, 10, 10))); // touching edge
    CHECK(!Intersects(Rect(0, 0, 10, 10), Rect(0, 10, 10, 10)));
    CHECK(!Intersects(Rect(0, 0, 0, 10), Rect(0, 0, 10, 10)));   // empty
}

void PrefersLeftTopAligned()
{
    const char* name = "menu slot: prefers left top aligned";
    Placement p = ResolveSlot(Base());
    CHECK(p.side == SIDE_LEFT);
    CHECK(p.free);
    CHECK(p.rects.size() == 2);
    // left = 800 - 2 - 120 = 678; second button 26 + 5 below the first.
    CHECK(RectEq(p.rects[0], 678, 400, 120, 26));
    CHECK(RectEq(p.rects[1], 678, 431, 120, 26));
}

void FallsToRightWhenLeftOffScreen()
{
    const char* name = "menu slot: falls to right when left off screen";
    Request r = Base();
    r.menu = Rect(50, 400, 200, 300); // left column would start at -72
    Placement p = ResolveSlot(r);
    CHECK(p.side == SIDE_RIGHT);
    CHECK(p.free);
    CHECK(RectEq(p.rects[0], 252, 400, 120, 26));
}

void FallsToRightWhenLeftBlocked()
{
    const char* name = "menu slot: falls to right when left blocked";
    Request r = Base();
    r.obstacles.push_back(Rect(600, 395, 190, 44)); // sits in the left column
    Placement p = ResolveSlot(r);
    CHECK(p.side == SIDE_RIGHT);
    CHECK(RectEq(p.rects[0], 1002, 400, 120, 26));
}

void RightEdgeIsChecked()
{
    const char* name = "menu slot: right edge is checked";
    // Menu hugs the right edge and the left column is blocked: right column
    // would overflow, so the left column must be pushed below the obstacle.
    Request r = Base();
    r.menu = Rect(1700, 400, 200, 300);
    r.obstacles.push_back(Rect(1560, 400, 130, 40));
    Placement p = ResolveSlot(r);
    CHECK(p.side == SIDE_LEFT);
    CHECK(p.free);
    CHECK(p.rects[0].left == 1578);
    CHECK(p.rects[0].top == 445);       // obstacle bottom 440 + gap 5
    CHECK(p.rects[1].top == 476);
}

void StacksBelowLeftObstacle()
{
    const char* name = "menu slot: stacks below left obstacle";
    Request r = Base();
    r.obstacles.push_back(Rect(600, 400, 190, 100));  // blocks left
    r.obstacles.push_back(Rect(1002, 400, 130, 60));  // blocks right
    r.obstacles.push_back(Rect(1002, 470, 130, 60));  // and below it
    Placement p = ResolveSlot(r);
    // Left pushed below: bottom 500 + 5. Right would need to clear 530.
    CHECK(p.side == SIDE_LEFT);
    CHECK(p.free);
    CHECK(p.rects[0].top == 505);
}

void StacksBelowRightWhenLeftCannotFit()
{
    const char* name = "menu slot: stacks below right when left cannot fit";
    Request r = Base();
    r.menu = Rect(50, 400, 200, 300);          // left off-screen
    r.obstacles.push_back(Rect(252, 390, 130, 60));
    Placement p = ResolveSlot(r);
    CHECK(p.side == SIDE_RIGHT);
    CHECK(p.free);
    CHECK(p.rects[0].top == 455);
}

void PushBelowRespectsScreenBottom()
{
    const char* name = "menu slot: push below respects screen bottom";
    Request r = Base();
    r.obstacles.push_back(Rect(600, 380, 190, 690));   // left column to bottom
    r.obstacles.push_back(Rect(1002, 380, 130, 690));  // right too
    Placement p = ResolveSlot(r);
    CHECK(!p.free);
    CHECK(p.side == SIDE_LEFT);
    CHECK(p.rects.size() == 2);
    // Clamped fallback stays fully on-screen.
    CHECK(p.rects[0].left >= 0);
    CHECK(p.rects[1].top + p.rects[1].height <= 1080);
}

void ClampedFallbackKeepsBlockOnScreen()
{
    const char* name = "menu slot: clamped fallback keeps block on screen";
    Request r = Base();
    r.menu = Rect(10, 1060, 200, 300);     // off the bottom, left column <0
    r.obstacles.push_back(Rect(212, 1000, 130, 80));
    Placement p = ResolveSlot(r);
    CHECK(!p.free);
    CHECK(p.rects[0].left >= 0);
    CHECK(p.rects[0].top >= 0);
    CHECK(p.rects[1].top + p.rects[1].height <= 1080);
}

void HysteresisKeepsPreviousSideWhenStillValid()
{
    const char* name = "menu slot: hysteresis keeps previous side when still valid";
    Request r = Base();
    r.previousSide = SIDE_RIGHT;       // left is free, but right still valid
    Placement p = ResolveSlot(r);
    CHECK(p.side == SIDE_RIGHT);
    CHECK(p.free);
    CHECK(RectEq(p.rects[0], 1002, 400, 120, 26));
}

void HysteresisKeepsPreviousSidePushedBelow()
{
    const char* name = "menu slot: hysteresis keeps previous side pushed below";
    Request r = Base();
    r.previousSide = SIDE_RIGHT;
    r.obstacles.push_back(Rect(1002, 390, 130, 50)); // right top blocked
    // Right can still be valid pushed below; previous side wins that way.
    Placement p = ResolveSlot(r);
    CHECK(p.side == SIDE_RIGHT);
    CHECK(p.rects[0].top == 445);
}

void HysteresisYieldsWhenPreviousSideImpossible()
{
    const char* name = "menu slot: hysteresis yields when previous side impossible";
    Request r = Base();
    r.menu = Rect(1700, 400, 200, 300);   // right column off-screen
    r.previousSide = SIDE_RIGHT;
    Placement p = ResolveSlot(r);
    CHECK(p.side == SIDE_LEFT);
    CHECK(p.free);
}

void ObstacleTouchingEdgeIsNotOverlap()
{
    const char* name = "menu slot: obstacle touching edge is not overlap";
    Request r = Base();
    r.obstacles.push_back(Rect(558, 400, 120, 26)); // ends exactly at x=678
    Placement p = ResolveSlot(r);
    CHECK(p.side == SIDE_LEFT);
    CHECK(p.free);
    CHECK(p.rects[0].top == 400);
}

void ZeroButtonsGivesEmptyPlacement()
{
    const char* name = "menu slot: zero buttons gives empty placement";
    Request r = Base();
    r.buttonCount = 0;
    Placement p = ResolveSlot(r);
    CHECK(p.rects.empty());
}

void SingleButtonBlock()
{
    const char* name = "menu slot: single button block";
    Request r = Base();
    r.buttonCount = 1;
    Placement p = ResolveSlot(r);
    CHECK(p.rects.size() == 1);
    CHECK(RectEq(p.rects[0], 678, 400, 120, 26));
}

void ObstacleFilter()
{
    const char* name = "menu slot: obstacle filter";
    const Rect menu(800, 400, 200, 300);
    CHECK(IsPlausibleObstacle(Rect(1002, 400, 160, 44), menu));   // sparring button
    CHECK(IsPlausibleObstacle(Rect(400, 100, 400, 300), menu));   // at the limit
    CHECK(!IsPlausibleObstacle(Rect(0, 0, 1920, 1080), menu));    // fullscreen root
    CHECK(!IsPlausibleObstacle(Rect(100, 100, 401, 50), menu));   // too wide
    CHECK(!IsPlausibleObstacle(Rect(100, 100, 50, 301), menu));   // too tall
    CHECK(!IsPlausibleObstacle(Rect(850, 450, 50, 20), menu));    // inside the menu
    CHECK(!IsPlausibleObstacle(Rect(700, 500, 120, 26), menu));   // overlaps menu edge
    CHECK(!IsPlausibleObstacle(Rect(100, 100, 0, 20), menu));     // zero width
    CHECK(!IsPlausibleObstacle(Rect(100, 100, 20, 0), menu));     // zero height
    CHECK(IsPlausibleObstacle(Rect(678, 400, 122, 26), menu));    // touching only
}

void ActionButtonSizedBlock()
{
    const char* name = "menu slot: action button sized block";
    // The shared look (button_style.h): three 160x44 buttons, 2px gaps, the
    // size TotalSparring uses for its own action buttons.
    Request r = Base();
    r.buttonCount = 3;
    r.buttonWidth = 160;
    r.buttonHeight = 44;
    r.gapX = 2;
    r.gapY = 2;
    Placement p = ResolveSlot(r);
    CHECK(p.side == SIDE_LEFT);
    CHECK(p.free);
    CHECK(p.rects.size() == 3);
    CHECK(RectEq(p.rects[0], 638, 400, 160, 44));
    CHECK(RectEq(p.rects[1], 638, 446, 160, 44));
    CHECK(RectEq(p.rects[2], 638, 492, 160, 44));

    // A TotalSparring button in the left column pushes the whole block right.
    r.obstacles.push_back(Rect(638, 400, 160, 44));
    p = ResolveSlot(r);
    CHECK(p.side == SIDE_RIGHT);
    CHECK(RectEq(p.rects[0], 1002, 400, 160, 44));
}

// TotalRecruit's own case: the Hablar button (one 160x44 block) with
// TotalSparring's two buttons right of the menu, top-aligned (they avoid
// nothing), and TotalLoot's block already left of it. Hablar must cover
// neither: it goes below TotalLoot's block in the left column.
void HablarAvoidsSparringAndLoot()
{
    const char* name = "menu slot: hablar avoids sparring and loot buttons";
    Request r = Base();
    r.buttonCount = 1;
    r.buttonWidth = 160;
    r.buttonHeight = 44;
    r.gapX = 2;
    r.gapY = 2;
    r.obstacles.push_back(Rect(1002, 400, 160, 44));  // TS_SparringButton
    r.obstacles.push_back(Rect(1002, 446, 160, 44));  // TS_ProfileButton
    r.obstacles.push_back(Rect(638, 400, 160, 44));   // TL_LootButton
    r.obstacles.push_back(Rect(638, 446, 160, 44));   // TL_LootAllButton
    Placement p = ResolveSlot(r);
    CHECK(p.free);
    CHECK(p.side == SIDE_LEFT);
    CHECK(p.rects.size() == 1);
    CHECK(RectEq(p.rects[0], 638, 492, 160, 44));
    for (size_t i = 0; i < r.obstacles.size(); ++i)
    {
        CHECK(!Intersects(p.rects[0], r.obstacles[i]));
    }

    // Only TotalSparring installed: Hablar takes the free left column, top-aligned.
    r.obstacles.resize(2);
    p = ResolveSlot(r);
    CHECK(p.side == SIDE_LEFT);
    CHECK(RectEq(p.rects[0], 638, 400, 160, 44));
}
} // namespace

void RunMenuSlotTests()
{
    HablarAvoidsSparringAndLoot();
    ActionButtonSizedBlock();
    ObstacleFilter();
    IntersectsBasics();
    PrefersLeftTopAligned();
    FallsToRightWhenLeftOffScreen();
    FallsToRightWhenLeftBlocked();
    RightEdgeIsChecked();
    StacksBelowLeftObstacle();
    StacksBelowRightWhenLeftCannotFit();
    PushBelowRespectsScreenBottom();
    ClampedFallbackKeepsBlockOnScreen();
    HysteresisKeepsPreviousSideWhenStillValid();
    HysteresisKeepsPreviousSidePushedBelow();
    HysteresisYieldsWhenPreviousSideImpossible();
    ObstacleTouchingEdgeIsNotOverlap();
    ZeroButtonsGivesEmptyPlacement();
    SingleButtonBlock();
}
