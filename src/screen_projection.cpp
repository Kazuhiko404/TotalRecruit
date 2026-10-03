#include "screen_projection.h"

namespace totalrecruit
{
namespace
{
// A head this far past an edge (in normalised device units, the screen is -1..1)
// still counts as on the screen: the card is clamped inside it anyway.
const float kOnScreenLimit = 1.1f;
// Closer to the camera plane than this is treated as behind it.
const float kMinDepth = 0.0001f;

bool Intersects(int left, int top, int width, int height, const ScreenRect& r)
{
    return left < r.left + r.width && r.left < left + width &&
           top < r.top + r.height && r.top < top + height;
}
} // namespace

ProjectionResult ProjectToScreen(const Mat4& view, const Mat4& proj, const Vec3& gamePoint,
                                 const Vec3& relativeOrigin, int viewW, int viewH,
                                 int* x, int* y)
{
    *x = 0;
    *y = 0;
    if (viewW <= 0 || viewH <= 0)
    {
        return Projection_NoView;
    }

    // Game space to render space: the engine's own world-to-screen does this first.
    const float p[3] = { gamePoint.x - relativeOrigin.x, gamePoint.y - relativeOrigin.y,
                         gamePoint.z - relativeOrigin.z };

    // Render space to camera space (the view matrix is affine: w stays 1).
    float v[4];
    for (int r = 0; r < 3; ++r)
    {
        v[r] = view.m[r][0] * p[0] + view.m[r][1] * p[1] + view.m[r][2] * p[2] + view.m[r][3];
    }
    v[3] = 1.0f;
    // The camera looks down -Z: anything at z >= 0 is behind it.
    if (v[2] > -kMinDepth)
    {
        return Projection_Behind;
    }

    float c[4];
    for (int r = 0; r < 4; ++r)
    {
        c[r] = proj.m[r][0] * v[0] + proj.m[r][1] * v[1] + proj.m[r][2] * v[2] +
               proj.m[r][3] * v[3];
    }
    if (c[3] <= kMinDepth)
    {
        return Projection_Behind;
    }
    const float nx = c[0] / c[3];
    const float ny = c[1] / c[3];

    // Rounded to the nearest pixel; y grows down the screen.
    const float px = (nx * 0.5f + 0.5f) * (float)viewW;
    const float py = (0.5f - ny * 0.5f) * (float)viewH;
    *x = (int)(px < 0.0f ? px - 0.5f : px + 0.5f);
    *y = (int)(py < 0.0f ? py - 0.5f : py + 0.5f);

    if (nx < -kOnScreenLimit || nx > kOnScreenLimit || ny < -kOnScreenLimit ||
        ny > kOnScreenLimit)
    {
        return Projection_OffScreen;
    }
    return Projection_Ok;
}

CardPlacement PlaceCard(int anchorX, int anchorY, int cardW, int cardH, int viewW, int viewH,
                        const ScreenRect* avoid, const CardMetrics& metrics, int* left,
                        int* top)
{
    const int m = metrics.margin;

    // Over the head, centred, kept inside the screen sideways.
    int l = anchorX - cardW / 2;
    int t = anchorY - cardH - metrics.gap;
    if (viewW > 0)
    {
        if (l + cardW > viewW - m) l = viewW - m - cardW;
        if (l < m) l = m;
    }
    // No room above the head: under it, never pinned to the top edge.
    if (t < m)
    {
        t = anchorY + metrics.belowGap;
    }
    if (viewH > 0 && t + cardH > viewH - m)
    {
        t = viewH - m - cardH;
    }
    *left = l;
    *top = t;

    if (avoid == 0 || !Intersects(l, t, cardW, cardH, *avoid))
    {
        return Placement_OverHead;
    }

    // Sideways only, at the same height: the nearer side of the window first.
    const int rightSide = avoid->left + avoid->width + metrics.gap;
    const int leftSide = avoid->left - metrics.gap - cardW;
    const bool rightFits = viewW <= 0 || rightSide + cardW <= viewW - m;
    const bool leftFits = leftSide >= m;
    const bool nearerIsRight = 2 * anchorX >= 2 * avoid->left + avoid->width;

    if (nearerIsRight ? rightFits : leftFits)
    {
        *left = nearerIsRight ? rightSide : leftSide;
        return Placement_BesideWindow;
    }
    if (nearerIsRight ? leftFits : rightFits)
    {
        *left = nearerIsRight ? leftSide : rightSide;
        return Placement_BesideWindow;
    }
    return Placement_NoRoom;
}
} // namespace totalrecruit
