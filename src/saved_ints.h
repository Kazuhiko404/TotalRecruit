#pragma once

// The per-character ints this mod keeps inside the Kenshi save.
//
// character_identity.cpp writes the persisted id into a character's save state
// (_NV_serialise) and reads it back on load (_NV_loadFromSerialise). Every
// per-character count that must survive a reload rides along on the same two
// hooks as an idata int next to the id. This table is the one list of them: the
// identity hooks loop over it, and a module that needs a saved count adds one row
// here (its key and its two callbacks) instead of another copy of the read/write
// bridge.
//
//   total_recruit.player_defeats   defeats.h       knockouts the player dealt
//   total_recruit.fed_by_player    feeding_ledger  meals the player gave (animals)
//   total_recruit.healed_by_player healing_ledger  1 once the player treated it (animals)
//   total_recruit.reject_cooldown_until
//                                  submission_tracker  game hour the reject cooldown ends
//                                                  (legacy: the minute rounded up)
//   total_recruit.reject_cooldown_until_min
//                                  submission_tracker  game minute it ends on
//                                                  (cooldown_minutes.h; wins over the hour)
//   total_recruit.temp_expiry_hour temp_member.h   game hour a temporary member's stay ends
//   total_recruit.temp_strikes     temp_member.h   a temporary member's mistreatment strikes
//   total_recruit.helper_expiry_hour
//                                  helper.h        game hour a free helper's escort ends
//   total_recruit.helper_reject_until
//                                  helper.h        game hour a helper may offer again
//   total_recruit.duel_cooldown_until
//                                  duel_challenge_rules.h  game hour an NPC who won a
//                                                  duel may be challenged again
//                                                  (legacy: the minute rounded up)
//   total_recruit.duel_cooldown_until_min
//                                  duel_challenge_rules.h  the same, as a game minute
//   total_recruit.beating_stress   beating_stress.h  the beating stress bar, in
//                                                  thousandths, as last written
//   total_recruit.beating_stress_hour
//                                  beating_stress.h  the game hour (tenths) it was
//                                                  last written at
//   total_recruit.beating_day      beating_stress.h  Balanced: the game day (+1; 0 =
//                                                  none) of the prisoner's day record
//                                                  (beating_daily_rules.h)
//   total_recruit.beating_day_blows
//                                  beating_stress.h  landed blows that day
//   total_recruit.beating_day_gained
//                                  beating_stress.h  bar gained that day (thousandths)
//   total_recruit.hold_start_hour
//                                  hold_tracking.h  the game hour (tenths) the
//                                                  prisoner's hold in the player's
//                                                  cage started; 0 when not held
//   total_recruit.care_streak_day  care_streak.h   Balanced: the game day (+1; 0 =
//                                                  none) of the prisoner's latest
//                                                  care sighting (care_streak_rules.h)
//   total_recruit.care_streak_days care_streak.h   kept cared-for days in a row
//                                                  before that day
//   total_recruit.care_streak_kept care_streak.h   1 when every sighting that day
//                                                  found them cared for
//
// Strings ride the same hooks through saved_strings.h.
//
// Contract for each row:
//   saved(id)          the value to write for the character carrying `id`. Always
//                      written, 0 included: a state that once held a value must
//                      not keep a stale one.
//   restore(id, value) the value a loading save state held; only called when the
//                      key is present. The module cleared its table at load begin
//                      (load_boundary.cpp), so this puts back exactly that save's
//                      value.
// Both may run off the game thread (the serialise hook): they must be
// thread-safe.

#include <string>

namespace totalrecruit
{
struct SavedIntField
{
    const char* key;
    int (*saved)(const std::string& persistedId);
    void (*restore)(const std::string& persistedId, int value);
};

// How many rows there are, and one of them (0 <= index < count).
int SavedIntFieldCount();
const SavedIntField& SavedIntFieldAt(int index);
} // namespace totalrecruit
