// Tests for src/screen_projection.cpp: a game-space point to screen pixels through
// the camera (with the scene's relative origin), and where the card goes over it.

#include "test_support.h"
#include "screen_projection.h"

using namespace totalrecruit;

namespace
{
Mat4 Identity()
{
    Mat4 m;
    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 4; ++c)
        {
            m.m[r][c] = (r == c) ? 1.0f : 0.0f;
        }
    }
    return m;
}

// Ogre's perspective matrix for a 90 degree field of view, square aspect, near 1,
// far 1000: what Frustum::getProjectionMatrix returns.
Mat4 Perspective()
{
    const float n = 1.0f;
    const float f = 1000.0f;
    Mat4 m;
    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 4; ++c)
        {
            m.m[r][c] = 0.0f;
        }
    }
    m.m[0][0] = 1.0f;
    m.m[1][1] = 1.0f;
    m.m[2][2] = -(f + n) / (f - n);
    m.m[2][3] = -2.0f * f * n / (f - n);
    m.m[3][2] = -1.0f;
    return m;
}

const Vec3 kNoOrigin;

void APointStraightAheadIsTheCentre()
{
    const char* name = "projection: straight ahead of the camera is the centre of the screen";
    int x = -1, y = -1;
    CHECK(ProjectToScreen(Identity(), Perspective(), Vec3(0.0f, 0.0f, -10.0f), kNoOrigin, 1000,
                          1000, &x, &y) == Projection_Ok);
    CHECK(x == 500 && y == 500);
}

void RightIsRightAndUpIsUp()
{
    const char* name = "projection: +x goes right, +y goes UP the screen (pixel y shrinks)";
    int x = 0, y = 0;
    CHECK(ProjectToScreen(Identity(), Perspective(), Vec3(5.0f, 0.0f, -10.0f), kNoOrigin, 1000,
                          1000, &x, &y) == Projection_Ok);
    CHECK(x == 750 && y == 500);
    CHECK(ProjectToScreen(Identity(), Perspective(), Vec3(0.0f, 5.0f, -10.0f), kNoOrigin, 1000,
                          1000, &x, &y) == Projection_Ok);
    CHECK(x == 500 && y == 250);
}

void BehindAndOffScreenAreReported()
{
    const char* name = "projection: behind the camera and off the screen are not a position";
    int x = 0, y = 0;
    CHECK(ProjectToScreen(Identity(), Perspective(), Vec3(0.0f, 0.0f, 10.0f), kNoOrigin, 1000,
                          1000, &x, &y) == Projection_Behind);
    CHECK(ProjectToScreen(Identity(), Perspective(), Vec3(0.0f, 0.0f, 0.0f), kNoOrigin, 1000,
                          1000, &x, &y) == Projection_Behind);
    CHECK(ProjectToScreen(Identity(), Perspective(), Vec3(30.0f, 0.0f, -10.0f), kNoOrigin, 1000,
                          1000, &x, &y) == Projection_OffScreen);
    CHECK(ProjectToScreen(Identity(), Perspective(), Vec3(0.0f, -30.0f, -10.0f), kNoOrigin, 1000,
                          1000, &x, &y) == Projection_OffScreen);
    CHECK(ProjectToScreen(Identity(), Perspective(), Vec3(0.0f, 0.0f, -10.0f), kNoOrigin, 0,
                          1000, &x, &y) == Projection_NoView);
}

void TheRelativeOriginIsTakenOut()
{
    const char* name = "projection: game space minus the relative origin is what the camera sees";
    // The camera sits at the render origin; the game has moved that origin to
    // (1000, 0, -2000). An NPC 10 units ahead of it in game space is dead centre.
    const Vec3 origin(1000.0f, 0.0f, -2000.0f);
    const Vec3 npc(1000.0f, 0.0f, -2010.0f);
    int x = 0, y = 0;
    CHECK(ProjectToScreen(Identity(), Perspective(), npc, origin, 1000, 1000, &x, &y) ==
          Projection_Ok);
    CHECK(x == 500 && y == 500);

    // The old bug: the same game point through the render-space camera with no origin
    // lands far from the NPC (towards the horizon of its direction).
    ProjectToScreen(Identity(), Perspective(), npc, kNoOrigin, 1000, 1000, &x, &y);
    CHECK(x != 500);
}

void TheViewMatrixMovesTheCamera()
{
    const char* name = "projection: the view matrix (camera 10 units back) is applied before the lens";
    Mat4 view = Identity();
    view.m[2][3] = -10.0f;
    int x = 0, y = 0;
    CHECK(ProjectToScreen(view, Perspective(), Vec3(10.0f, 0.0f, -10.0f), kNoOrigin, 1000, 1000,
                          &x, &y) == Projection_Ok);
    // 10 right at 20 deep: ndc 0.5.
    CHECK(x == 750 && y == 500);
}

// Card placement: a 264 x 100 card on a 1920 x 1080 screen.
const int kW = 264;
const int kH = 100;
const CardMetrics kMetrics;

void TheCardSitsOverTheHead()
{
    const char* name = "card: centred over the anchor, its bottom edge a gap above it";
    int left = 0, top = 0;
    CHECK(PlaceCard(960, 600, kW, kH, 1920, 1080, 0, kMetrics, &left, &top) == Placement_OverHead);
    CHECK(left == 828);
    CHECK(top == 492);
}

void TheCardStaysInsideTheScreenSideways()
{
    const char* name = "card: clamped inside the screen edges, left and right";
    int left = 0, top = 0;
    PlaceCard(10, 600, kW, kH, 1920, 1080, 0, kMetrics, &left, &top);
    CHECK(left == 4);
    CHECK(top == 492);
    PlaceCard(1915, 600, kW, kH, 1920, 1080, 0, kMetrics, &left, &top);
    CHECK(left == 1652);
}

void NoRoomAboveHangsItBelowTheHead()
{
    const char* name = "card: a head near the top hangs the card below it, never pinned to the top";
    int left = 0, top = 0;
    CHECK(PlaceCard(960, 50, kW, kH, 1920, 1080, 0, kMetrics, &left, &top) == Placement_OverHead);
    CHECK(top == 82);
}

void AWindowElsewhereChangesNothing()
{
    const char* name = "card: a window it does not cover leaves it over the head";
    const ScreenRect offer(1300, 100, 400, 200);
    int left = 0, top = 0;
    CHECK(PlaceCard(500, 600, kW, kH, 1920, 1080, &offer, kMetrics, &left, &top) ==
          Placement_OverHead);
    CHECK(left == 368 && top == 492);
}

void ItStepsToTheNearerSideAtTheSameHeight()
{
    const char* name = "card: over the window it steps sideways to the nearer side, same height";
    const ScreenRect offer(690, 390, 540, 300);
    int left = 0, top = 0;
    CHECK(PlaceCard(1000, 600, kW, kH, 1920, 1080, &offer, kMetrics, &left, &top) ==
          Placement_BesideWindow);
    CHECK(left == 1238);
    CHECK(top == 492);

    CHECK(PlaceCard(900, 600, kW, kH, 1920, 1080, &offer, kMetrics, &left, &top) ==
          Placement_BesideWindow);
    CHECK(left == 418);
    CHECK(top == 492);
}

void ItTakesTheFarSideWhenTheNearOneIsFull()
{
    const char* name = "card: the nearer side has no room, so the other side, still the same height";
    const ScreenRect offer(700, 390, 540, 300);
    int left = 0, top = 0;
    CHECK(PlaceCard(1000, 600, kW, kH, 1300, 1080, &offer, kMetrics, &left, &top) ==
          Placement_BesideWindow);
    CHECK(left == 428);
    CHECK(top == 492);
}

void NoSideMeansNoCard()
{
    const char* name = "card: no room on either side of the window: not shown, never moved up";
    const ScreenRect offer(130, 390, 540, 300);
    int left = 0, top = 0;
    CHECK(PlaceCard(400, 600, kW, kH, 800, 1080, &offer, kMetrics, &left, &top) ==
          Placement_NoRoom);
}
} // namespace

void RunScreenProjectionTests()
{
    APointStraightAheadIsTheCentre();
    RightIsRightAndUpIsUp();
    BehindAndOffScreenAreReported();
    TheRelativeOriginIsTakenOut();
    TheViewMatrixMovesTheCamera();
    TheCardSitsOverTheHead();
    TheCardStaysInsideTheScreenSideways();
    NoRoomAboveHangsItBelowTheHead();
    AWindowElsewhereChangesNothing();
    ItStepsToTheNearerSideAtTheSameHeight();
    ItTakesTheFarSideWhenTheNearOneIsFull();
    NoSideMeansNoCard();
}
