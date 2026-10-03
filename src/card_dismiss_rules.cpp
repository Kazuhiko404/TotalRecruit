#include "card_dismiss_rules.h"

namespace totalrecruit
{
MousePress NextPressEdge(PressEdgeState* state, bool leftDown, bool rightDown)
{
    const bool wasPrimed = state->primed;
    const bool leftStarted = leftDown && !state->leftWasDown;
    const bool rightStarted = rightDown && !state->rightWasDown;
    state->primed = true;
    state->leftWasDown = leftDown;
    state->rightWasDown = rightDown;

    if (!wasPrimed)
    {
        return Press_None;
    }
    if (leftStarted)
    {
        return Press_Left;
    }
    return rightStarted ? Press_Right : Press_None;
}

void ResetPressEdges(PressEdgeState* state)
{
    *state = PressEdgeState();
}

bool PointOnBody(int x, int y, const BodySpan& body)
{
    const int top = body.headY < body.footY ? body.headY : body.footY;
    const int bottom = body.headY < body.footY ? body.footY : body.headY;
    if (y < top || y > bottom)
    {
        return false;
    }

    int halfWidth = (int)((float)(bottom - top) * kBodyHalfWidthPerHeight + 0.5f);
    if (halfWidth < kBodyMinHalfWidthPx)
    {
        halfWidth = kBodyMinHalfWidthPx;
    }
    const int centreX = (body.footX + body.headX) / 2;
    const int dx = x - centreX;
    return dx >= -halfWidth && dx <= halfWidth;
}

bool NextCardDismissed(bool dismissed, const CardDismissInput& input)
{
    // A new subject (another selection, or Hablar) always starts shown; the click
    // that selected them is not an outside click for their card.
    if (input.selectionChanged || input.pinRequested)
    {
        return false;
    }
    if (input.press == Press_None || input.overMyGui || input.pinnedWithOffer)
    {
        return dismissed;
    }
    if (input.onNpc)
    {
        // Left: clicked them again, show it. Right: the context menu, unchanged.
        return input.press == Press_Left ? false : dismissed;
    }
    return true;
}
} // namespace totalrecruit
