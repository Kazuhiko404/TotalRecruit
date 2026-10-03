#pragma once

// Haggling over a cats price, without the engine.
//
// Only a quote in cats can be haggled (a free-world bribe, the captive cats
// route); rescue, mercy, field-yield and surrender are free and have nothing to
// bargain over. The player picks an offer on a slider between a floor and the
// asking price; the full price is accepted exactly as before, and anything less
// is one attempt: the NPC takes it, or is offended -- and an offended NPC will
// not haggle again that in-game day and asks more until the day turns.
//
// Every number is a Prices field (config.h), the stat reads live on the engine
// side (character_facts.cpp), and the per-day offence state is haggle_ledger.h.

#include "config.h"
#include "char_key.h"
#include "temperament.h"

namespace totalrecruit
{
// What the slider may show: [minOffer, maxOffer] in steps of `step` cats.
struct HaggleBounds
{
    int minOffer;
    int maxOffer;
    int step;

    HaggleBounds() : minOffer(0), maxOffer(0), step(10) {}
};

// The slider step, in cats. Prices are rounded to 10 (price_rules.cpp), so the
// slider moves in the same unit.
const int kHaggleStep = 10;

// `value` rounded up to a multiple of `step` (a step of 0 or less leaves it).
int RoundUpToStep(int value, int step);

// min = RoundUpToStep(ask * haggleMinFraction, 10), max = ask. Never inverted
// (a floor that would pass the ask becomes the ask) and never negative.
HaggleBounds HaggleSliderBounds(int ask, const Prices& prices);

// An offer held on the slider: clamped into the bounds and snapped to the
// nearest step, except that the ask itself (the top) is always reachable even
// when it is not a multiple of the step.
int ClampOffer(int offer, const HaggleBounds& bounds);

// The slider's positions: 0 is minOffer, each next one `step` cats more, and the
// last (HaggleStepCount) is maxOffer even when the ask is off the step grid. A
// range with nothing in it (min == max) is a single position, 0.
int HaggleStepCount(const HaggleBounds& bounds);
// The offer at slider position `step` (clamped to 0..HaggleStepCount).
int OfferAtStep(const HaggleBounds& bounds, int step);
// The slider position an offer sits at, after ClampOffer.
int StepOfOffer(const HaggleBounds& bounds, int offer);

// What the NPC's temperament does to the odds of accepting a lowball. The
// Temperament enum has no "proud", so the design's "honourable/proud -15" maps to
// Temperament_Honorable alone:
//   Fearful +15; Brave, ColdCruel, Maniacal -10; Honorable -15; anyone else 0.
// Fixed shifts, not config keys: they are the design's personality table for
// this one roll and are pinned by tests.
int HagglePersonalityShift(Temperament personality);

// The speaker's weight in the room: their best of Strength, MeleeAttack and
// Toughness against the NPC's level, times haggleLeverageWeight, held within
// +-20 and rounded to the nearest point.
int HaggleLeverage(int speakerBestStat, int npcLevel, const Prices& prices);

// The chance, 0..100, that the NPC accepts `offer` against `ask`:
//   offer >= ask (or no ask at all)   -> 100, the full price works as before;
//   otherwise 100 - discount% * haggleDiscountWeight + leverage
//             + HagglePersonalityShift, clamped to 0..95 (a real discount is
//             never certain).
// Never rises as the offer falls.
int AcceptOdds(int ask, int offer, int leverage, Temperament personality,
               const Prices& prices);

enum HaggleOutcome
{
    Haggle_Accepted,
    Haggle_Offended
};

// The haggle attempt itself, drawn from the same daily seed as the join roll but
// under its own salt (kHaggleRollSalt), so it is a different coin on the same
// day: DailyRoll(rollKey, dayIndex, kHaggleRollSalt, odds). The caller supplies
// a key that was actually read (MintRollKey); 100 is always accepted, 0 never.
HaggleOutcome ResolveHaggle(int odds, const CharKey& rollKey, int dayIndex);

// The asking price an NPC quotes today: the plain quote, or -- when they were
// offended today -- the quote times (1 + haggleOffenceMarkup), rounded up to 10.
// The one place the markup is applied (route_cats.cpp calls it).
int OffendedAsk(int ask, bool offended, const Prices& prices);
} // namespace totalrecruit
