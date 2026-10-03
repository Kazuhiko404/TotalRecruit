// Tests for the pure half of src/flat_slider.h: the slider position <-> pixel
// mapping ported from TotalSparring (shared/ui/flat_slider_math.h), which the
// haggle slider in the recruit popup is built on.

#include "test_support.h"
#include "flat_slider.h"

using namespace totalrecruit::ui;

namespace
{
const int kThumb = 22;

void ClampHoldsTheRange()
{
    const char* name = "slider clamp: inside [0, max], nothing for an empty range";
    CHECK(ClampSliderValue(-3, 10) == 0);
    CHECK(ClampSliderValue(4, 10) == 4);
    CHECK(ClampSliderValue(11, 10) == 10);
    CHECK(ClampSliderValue(5, 0) == 0);
    CHECK(ClampSliderValue(5, -1) == 0);
}

void TheThumbCentreFollowsTheMouse()
{
    const char* name = "slider value from x: thumb centre under the mouse, ends reachable";
    // A 310 px track, 22 px thumb: 288 px of travel for 478 steps.
    CHECK(ValueFromX(0, 310, kThumb, 478) == 0);
    CHECK(ValueFromX(11, 310, kThumb, 478) == 0);
    CHECK(ValueFromX(310, 310, kThumb, 478) == 478);
    CHECK(ValueFromX(299, 310, kThumb, 478) == 478);
    CHECK(ValueFromX(155, 310, kThumb, 478) == 239);
    CHECK(ValueFromX(-40, 310, kThumb, 478) == 0);
    CHECK(ValueFromX(900, 310, kThumb, 478) == 478);
}

void ThumbLeftIsTheInverse()
{
    const char* name = "slider thumb left: inverse of value from x";
    CHECK(ThumbLeftFor(0, 310, kThumb, 478) == 0);
    CHECK(ThumbLeftFor(478, 310, kThumb, 478) == 288);
    CHECK(ThumbLeftFor(239, 310, kThumb, 478) == 144);
    bool roundTrips = true;
    for (int v = 0; v <= 20; ++v)
    {
        const int left = ThumbLeftFor(v, 310, kThumb, 20);
        if (ValueFromX(left + kThumb / 2, 310, kThumb, 20) != v) roundTrips = false;
    }
    CHECK(roundTrips);
}

void DegenerateSlidersStayAtZero()
{
    const char* name = "slider: a track no wider than the thumb, or no range, is 0";
    CHECK(ValueFromX(50, 20, kThumb, 10) == 0);
    CHECK(ThumbLeftFor(5, 20, kThumb, 10) == 0);
    CHECK(ValueFromX(50, 310, kThumb, 0) == 0);
    CHECK(ThumbLeftFor(5, 310, kThumb, 0) == 0);
}
} // namespace

void RunFlatSliderTests()
{
    ClampHoldsTheRange();
    TheThumbCentreFollowsTheMouse();
    ThumbLeftIsTheInverse();
    DegenerateSlidersStayAtZero();
}
