#include "haggle_rules.h"
#include "chance_rules.h"

#include <cmath>
#include <cstdlib>

namespace totalrecruit
{
namespace
{
const int kLeverageCap = 20;
const int kDiscountCeiling = 95;

int RoundToInt(float value)
{
    return (int)std::floor(value + 0.5f);
}

int ClampInt(int value, int lo, int hi)
{
    if (value < lo) return lo;
    if (value > hi) return hi;
    return value;
}
} // namespace

int RoundUpToStep(int value, int step)
{
    if (step <= 0 || value <= 0)
    {
        return value < 0 ? 0 : value;
    }
    return ((value + step - 1) / step) * step;
}

HaggleBounds HaggleSliderBounds(int ask, const Prices& prices)
{
    HaggleBounds bounds;
    bounds.step = kHaggleStep;
    if (ask <= 0)
    {
        return bounds;
    }

    const int floorCats = (int)std::ceil((double)ask * prices.haggleMinFraction - 1e-3);
    bounds.maxOffer = ask;
    bounds.minOffer = RoundUpToStep(floorCats, kHaggleStep);
    if (bounds.minOffer > ask)
    {
        bounds.minOffer = ask;
    }
    if (bounds.minOffer < 0)
    {
        bounds.minOffer = 0;
    }
    return bounds;
}

int ClampOffer(int offer, const HaggleBounds& bounds)
{
    if (offer >= bounds.maxOffer) return bounds.maxOffer;
    if (offer <= bounds.minOffer) return bounds.minOffer;

    int snapped = offer;
    if (bounds.step > 0)
    {
        snapped = ((offer + bounds.step / 2) / bounds.step) * bounds.step;
    }
    // The top is a slider position of its own even off the step grid: an offer
    // nearer the ask than the snapped step lands on the ask.
    if (bounds.maxOffer - offer < std::abs(snapped - offer))
    {
        return bounds.maxOffer;
    }
    return ClampInt(snapped, bounds.minOffer, bounds.maxOffer);
}

int HagglePersonalityShift(Temperament personality)
{
    switch (personality)
    {
    case Temperament_Fearful:
        return 15;
    case Temperament_Brave:
    case Temperament_ColdCruel:
    case Temperament_Maniacal:
        return -10;
    case Temperament_Honorable:
        return -15;
    default:
        return 0;
    }
}

int HaggleLeverage(int speakerBestStat, int npcLevel, const Prices& prices)
{
    const float raw = (float)(speakerBestStat - npcLevel) * prices.haggleLeverageWeight;
    return ClampInt(RoundToInt(raw), -kLeverageCap, kLeverageCap);
}

int AcceptOdds(int ask, int offer, int leverage, Temperament personality,
               const Prices& prices)
{
    if (ask <= 0 || offer >= ask)
    {
        return 100;
    }

    const float discountPercent = (float)(ask - offer) * 100.0f / (float)ask;
    const int odds = RoundToInt(100.0f - discountPercent * prices.haggleDiscountWeight)
                   + leverage
                   + HagglePersonalityShift(personality);
    return ClampInt(odds, 0, kDiscountCeiling);
}

HaggleOutcome ResolveHaggle(int odds, const CharKey& rollKey, int dayIndex)
{
    return DailyRoll(rollKey, dayIndex, kHaggleRollSalt, odds) ? Haggle_Accepted
                                                                : Haggle_Offended;
}

int OffendedAsk(int ask, bool offended, const Prices& prices)
{
    if (!offended || prices.haggleOffenceMarkup <= 0.0f || ask <= 0)
    {
        return ask;
    }
    const double raised = (double)ask * (1.0 + (double)prices.haggleOffenceMarkup);
    return RoundUpToStep((int)std::ceil(raised - 1e-3), kHaggleStep);
}
} // namespace totalrecruit
