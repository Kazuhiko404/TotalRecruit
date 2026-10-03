#pragma once

// Personality groups, without the engine.
//
// Ten temperaments are too many for a rule table the player can hold in their
// head, so the ally tiers (ally_tier_rules.h) read them as four groups. The
// mapping lives here and nowhere else; REQUISITOS.md documents it.
//
//   Good     Honorable, WarmKind              -- loyal by nature
//   Neutral  Normal, Dumb, Fearful, Brave     -- ordinary people
//   Greedy   Traitorous, Smart                -- loyalty was never the point
//   Bad      ColdCruel, Maniacal              -- cruel or psychopathic

#include "temperament.h"

namespace totalrecruit
{
enum PersonalityGroup
{
    PersonalityGroup_Good,
    PersonalityGroup_Neutral,
    PersonalityGroup_Greedy,
    PersonalityGroup_Bad
};

// The group a temperament belongs to. Anything outside the enum reads as Neutral,
// the same as an unknown temperament reads as Normal everywhere else.
PersonalityGroup GroupOf(Temperament temperament);
} // namespace totalrecruit
