#pragma once

// Temporary squad members: a Balanced ally of level 80 or more who joins for a few
// days only (ally_tier_rules.h, AllyTierMode_Temporary).
//
// The recruit itself is the ordinary one (recruit_core.cpp CompleteOffer); this
// module owns everything after it:
//
//   - The stay. On joining, the expiry hour (now + days * 24) and the strike count
//     go into the character's save (saved_ints.h rows total_recruit.temp_expiry_hour
//     and total_recruit.temp_strikes), and the faction they came from -- read by
//     CompleteOffer BEFORE the recruit changed it -- as a saved string
//     (saved_strings.h, total_recruit.temp_home_faction).
//   - The watch. TempMemberTick (frame tick, every ~2 s) reads each member's hunger,
//     whether one of the squad is attacking them and whether one of the squad just
//     knocked them out, and lets temp_member_rules.h decide: strikes, warnings and
//     the leave (TempMembers in config.h, keys tempMember*).
//   - What they say, over their head (head_notice.h RaiseSquadHeadNotice): on
//     joining, on each strike, the last warning, "me ire pronto", and the farewell
//     (a roller line too). messages.json block "tempMember".
//   - The join spot: where they stood when they joined, saved as "x,z"
//     (saved_strings.h, total_recruit.temp_join_spot; leave_homing_rules.h).
//   - The leave (squad_leave.h, KEP's dismiss sequence): back to their home faction,
//     in a platoon of their own with the Wandering Squad's package; without that
//     package they walk back to the join spot (leave_homing.h). Then the reject cooldown (submission_tracker.h) runs
//     for tempMemberRejoinCooldownDays: they want nothing to do with the player for
//     a while, so they cannot be taken on again straight away.
//   - The card: a selected temporary member shows "Temporal: le quedan N dias"
//     (readout.cpp, TempMemberDaysLeft).
//
// The table is keyed by CharKey and locked: the serialise hook that reads it may
// run off the game thread. The load boundary clears it (ForgetAllTempMembers) and
// each character's load hook puts back what its save held.

#include <string>

class Character;

namespace totalrecruit
{
// Called once, right after `npc` joined the squad as a temporary member who stays
// `days` in-game days (Verdict::temporaryDays). `homeFactionId` is the string id of
// the faction they belonged to before the recruit (engine_reads.h FactionStringId).
void OnRecruitedTemporarily(Character* npc, int days, const std::string& homeFactionId);

// The watch, from the frame tick; does its work every ~2 real seconds.
void TempMemberTick();

// Whether `member` is a temporary member, and the whole days they have left.
bool TempMemberDaysLeft(Character* member, int* daysLeft);

// The character the player has selected when it is a temporary member, else NULL.
Character* SelectedTempMember();

// Load begin: every member belongs to the world that was just abandoned.
void ForgetAllTempMembers();

// The save bridges (saved_ints.cpp, saved_strings.cpp). Thread-safe.
int SavedTempExpiryHour(const std::string& persistedId);
void RestoreSavedTempExpiryHour(const std::string& persistedId, int expiryHour);
int SavedTempStrikes(const std::string& persistedId);
void RestoreSavedTempStrikes(const std::string& persistedId, int strikes);
std::string SavedTempHomeFaction(const std::string& persistedId);
void RestoreSavedTempHomeFaction(const std::string& persistedId, const std::string& factionId);
std::string SavedTempJoinSpot(const std::string& persistedId);
void RestoreSavedTempJoinSpot(const std::string& persistedId, const std::string& spot);
} // namespace totalrecruit
