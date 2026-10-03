#pragma once

// Pure geometry: picks a free slot beside the native context menu for a
// vertical block of buttons. No Kenshi / MyGUI includes so it is unit-tested.

#include <vector>

namespace totalrecruit
{
namespace menuslot
{
struct Rect
{
    int left;
    int top;
    int width;
    int height;

    Rect() : left(0), top(0), width(0), height(0) {}
    Rect(int l, int t, int w, int h) : left(l), top(t), width(w), height(h) {}
};

enum Side
{
    SIDE_NONE = 0,
    SIDE_LEFT,
    SIDE_RIGHT
};

struct Request
{
    Rect menu;                      // native options list, absolute coords
    int screenWidth;
    int screenHeight;
    std::vector<Rect> obstacles;    // foreign widgets that must not be covered
    int buttonCount;                // N buttons stacked vertically
    int buttonWidth;
    int buttonHeight;
    int gapX;                       // horizontal gap from the menu
    int gapY;                       // vertical gap between buttons
    Side previousSide;              // last chosen side (hysteresis), or SIDE_NONE

    Request()
        : screenWidth(0), screenHeight(0), buttonCount(0), buttonWidth(0),
          buttonHeight(0), gapX(2), gapY(5), previousSide(SIDE_NONE) {}
};

struct Placement
{
    Side side;
    bool free;                      // false when only the clamped fallback fit
    std::vector<Rect> rects;        // one per button, top to bottom

    Placement() : side(SIDE_NONE), free(false) {}
};

bool Intersects(const Rect& a, const Rect& b);

// Size limits for a root widget to count as a foreign overlay button; larger
// roots are HUD layers and windows that would leave no slot at all.
const int kMaxObstacleWidth = 400;
const int kMaxObstacleHeight = 300;

// True when a visible root widget at `widget` should be avoided: it has a real
// but small size and does not overlap the native menu itself (which also
// excludes full-screen roots that contain the menu).
bool IsPlausibleObstacle(const Rect& widget, const Rect& menu);

// Candidate order: LEFT top-aligned, RIGHT top-aligned, LEFT pushed below the
// obstacles in its column, RIGHT pushed below, then LEFT clamped to the
// screen. A candidate is valid when the whole block is on-screen and touches
// no obstacle. If previousSide is still valid it wins (anti-flicker).
Placement ResolveSlot(const Request& request);
} // namespace menuslot
} // namespace totalrecruit
