#include "menu_slot.h"

#include <cstddef>

namespace totalrecruit
{
namespace menuslot
{
namespace
{
int BlockHeight(const Request& req)
{
    return req.buttonCount * req.buttonHeight + (req.buttonCount - 1) * req.gapY;
}

int ColumnLeft(const Request& req, Side side)
{
    if (side == SIDE_LEFT)
    {
        return req.menu.left - req.gapX - req.buttonWidth;
    }
    return req.menu.left + req.menu.width + req.gapX;
}

Rect BlockAt(const Request& req, Side side, int top)
{
    return Rect(ColumnLeft(req, side), top, req.buttonWidth, BlockHeight(req));
}

bool OnScreen(const Rect& r, const Request& req)
{
    return r.left >= 0 && r.top >= 0 &&
           r.left + r.width <= req.screenWidth &&
           r.top + r.height <= req.screenHeight;
}

// Bottom edge of the lowest obstacle touching `block`, or -1 when none does.
int LowestBottomHit(const Rect& block, const std::vector<Rect>& obstacles)
{
    int bottom = -1;
    for (size_t i = 0; i < obstacles.size(); ++i)
    {
        if (Intersects(block, obstacles[i]))
        {
            const int b = obstacles[i].top + obstacles[i].height;
            if (b > bottom)
            {
                bottom = b;
            }
        }
    }
    return bottom;
}

bool IsFree(const Rect& block, const Request& req)
{
    return OnScreen(block, req) && LowestBottomHit(block, req.obstacles) < 0;
}

// Slides the block down the column, each time to just below whatever it
// touches, until it is free or runs off the screen.
bool FindTop(const Request& req, Side side, bool pushBelow, int* outTop)
{
    int top = req.menu.top;
    const size_t maxSteps = req.obstacles.size() + 1;
    for (size_t step = 0; step <= maxSteps; ++step)
    {
        const Rect block = BlockAt(req, side, top);
        if (IsFree(block, req))
        {
            *outTop = top;
            return true;
        }
        if (!pushBelow || !OnScreen(block, req))
        {
            return false;
        }
        const int bottom = LowestBottomHit(block, req.obstacles);
        if (bottom < 0)
        {
            return false;
        }
        top = bottom + req.gapY;
    }
    return false;
}

void FillRects(const Request& req, Side side, int top, int left, Placement* out)
{
    out->side = side;
    out->rects.clear();
    for (int i = 0; i < req.buttonCount; ++i)
    {
        out->rects.push_back(Rect(left, top + i * (req.buttonHeight + req.gapY),
                                  req.buttonWidth, req.buttonHeight));
    }
}

bool TrySide(const Request& req, Side side, Placement* out)
{
    int top = 0;
    if (FindTop(req, side, false, &top) || FindTop(req, side, true, &top))
    {
        out->free = true;
        FillRects(req, side, top, ColumnLeft(req, side), out);
        return true;
    }
    return false;
}

int Clamp(int value, int low, int high)
{
    if (value > high) value = high;
    if (value < low) value = low;
    return value;
}
} // namespace

bool Intersects(const Rect& a, const Rect& b)
{
    if (a.width <= 0 || a.height <= 0 || b.width <= 0 || b.height <= 0)
    {
        return false;
    }
    return a.left < b.left + b.width && b.left < a.left + a.width &&
           a.top < b.top + b.height && b.top < a.top + a.height;
}

bool IsPlausibleObstacle(const Rect& widget, const Rect& menu)
{
    if (widget.width <= 0 || widget.height <= 0)
    {
        return false;
    }
    if (widget.width > kMaxObstacleWidth || widget.height > kMaxObstacleHeight)
    {
        return false;
    }
    return !Intersects(widget, menu);
}

Placement ResolveSlot(const Request& req)
{
    Placement result;
    if (req.buttonCount <= 0)
    {
        return result;
    }

    // Anti-flicker: another mod's hook may run before or after ours each
    // frame, so stick to the side chosen last time while it stays valid.
    if (req.previousSide != SIDE_NONE && TrySide(req, req.previousSide, &result))
    {
        return result;
    }

    // Aligned LEFT, aligned RIGHT, then pushed-below LEFT, pushed-below RIGHT.
    int top = 0;
    const Side order[2] = { SIDE_LEFT, SIDE_RIGHT };
    for (int pass = 0; pass < 2; ++pass)
    {
        for (int i = 0; i < 2; ++i)
        {
            if (FindTop(req, order[i], pass == 1, &top))
            {
                result.free = true;
                FillRects(req, order[i], top, ColumnLeft(req, order[i]), &result);
                return result;
            }
        }
    }

    // Nothing is free: keep the block on-screen, LEFT of the menu.
    const int left = Clamp(ColumnLeft(req, SIDE_LEFT), 0, req.screenWidth - req.buttonWidth);
    const int clampedTop = Clamp(req.menu.top, 0, req.screenHeight - BlockHeight(req));
    result.free = false;
    FillRects(req, SIDE_LEFT, clampedTop, left, &result);
    return result;
}
} // namespace menuslot
} // namespace totalrecruit
