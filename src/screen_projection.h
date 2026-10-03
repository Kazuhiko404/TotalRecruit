#pragma once

// Screen geometry for the readout card, without the engine: where a point of the
// world lands on the screen, and where the card goes over it.
//
// Projection. Kenshi renders on a relative origin (Ogre SceneManager::
// setRelativeOrigin): the Ogre camera's view matrix lives in render space, while
// every position the game hands out (Character::getPosition, physics, AI) lives in
// game space, and render = game - relativeOrigin. The engine's own world-to-screen
// subtracts the origin before the view matrix, and CameraClass::getCameraPos adds it
// back to the camera node (both read from kenshi_x64.exe, GOG 1.0.65 layout: RVAs
// 0x9B12C0 and 0x1008B0). Projecting a game-space point through the render-space
// matrix without that step aims at the point's direction from the render origin,
// which lands on the horizon: the card "flying at the top of the screen".
//
// Placement. The card sits centred over the anchor, its bottom edge `gap` above it,
// kept inside the screen; with no room above the head it hangs below it. When it
// would cover the offer window it steps HORIZONTALLY to the window's nearer side at
// the same height, else the far side; with no room on either side it is not shown.
// It never moves up or down to dodge the window and is never pinned to the top of
// the screen.

namespace totalrecruit
{
// A 4x4 matrix laid out like Ogre::Matrix4: m[row][col], applied to column vectors
// (translation in the last column).
struct Mat4
{
    float m[4][4];
};

struct Vec3
{
    float x;
    float y;
    float z;

    Vec3() : x(0.0f), y(0.0f), z(0.0f) {}
    Vec3(float ax, float ay, float az) : x(ax), y(ay), z(az) {}
};

struct ScreenRect
{
    int left;
    int top;
    int width;
    int height;

    ScreenRect() : left(0), top(0), width(0), height(0) {}
    ScreenRect(int l, int t, int w, int h) : left(l), top(t), width(w), height(h) {}
};

enum ProjectionResult
{
    Projection_Ok,
    // No view size to project into.
    Projection_NoView,
    // Behind the camera (or on its plane).
    Projection_Behind,
    // In front, but outside the screen by more than a small margin.
    Projection_OffScreen
};

// A game-space point to screen pixels (origin top-left, y down). `relativeOrigin` is
// the scene's relative origin (SceneManager::getRelativeOrigin), `view` and `proj`
// the camera's view and projection matrices (Ogre conventions: the camera looks
// down -Z, projection to [-1, 1]). The pixel is written even when off the screen.
ProjectionResult ProjectToScreen(const Mat4& view, const Mat4& proj, const Vec3& gamePoint,
                                 const Vec3& relativeOrigin, int viewW, int viewH,
                                 int* x, int* y);

struct CardMetrics
{
    // Between the anchor and the card's bottom edge, and between the card and the
    // window it steps beside.
    int gap;
    // Kept free along every screen edge.
    int margin;
    // How far below the anchor the card hangs when there is no room above it.
    int belowGap;

    CardMetrics() : gap(8), margin(4), belowGap(32) {}
};

enum CardPlacement
{
    // Over the head (or under it, when the top of the screen leaves no room).
    Placement_OverHead,
    // Beside the window it would have covered, at the same height.
    Placement_BesideWindow,
    // It would cover the window and neither side has room: not shown.
    Placement_NoRoom
};

// Where the card's top-left goes for an anchor (the projected point over the head).
// `avoid` is the window not to cover (the offer), or NULL. `left` / `top` are
// written for every result, Placement_NoRoom included (the over-head spot).
CardPlacement PlaceCard(int anchorX, int anchorY, int cardW, int cardH, int viewW, int viewH,
                        const ScreenRect* avoid, const CardMetrics& metrics, int* left,
                        int* top);
} // namespace totalrecruit
