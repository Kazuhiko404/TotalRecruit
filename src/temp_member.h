#pragma once

// Temporary squad members: a Balanced ally of level 80 or more who joins for a few
// days only (ally_tier_rules.h, AllyTierMode_Temporary).
//
// The recruit itself is the ordinary one (recruit_core.cpp CompleteOffer); this is
// the single place that learns it was temporary. It will own the rest of the
// story (odd/tasks/ally-tiers-temp-helpers.md, T2): the expiry day and the home
// faction saved with the character, the mistreatment strikes, the warnings over
// their head and the leave. For now it only logs.

class Character;

namespace totalrecruit
{
// Called once, right after `npc` joined the squad as a temporary member who stays
// `days` in-game days (Verdict::temporaryDays, drawn when the verdict was reached).
void OnRecruitedTemporarily(Character* npc, int days);
} // namespace totalrecruit
