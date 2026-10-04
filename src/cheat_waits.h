#pragma once

// The waits a Cheat player meets, without the engine.
//
// Cheat keeps every gesture and drops every cost (R1.2), so each wait the player
// can run into in Cheat lasts one game minute: the cage wait before the prisoner
// joins, the prisoner calling out ("te llama"), the Golpear / Noquear rests, the
// reject cooldown and the lost-duel cooldown (a won duel always joins in
// Cheat, so its rolled cooldown never runs). No logic is removed for it: each
// existing read of those hours goes through here, and Balanced gets back the
// configured value untouched.
//
// In Cheat the wait is min(configured, kCheatWaitHours): a configured wait already
// shorter than a minute (or 0, no wait) stays as it is, so Cheat is never slower
// than the config. Configuration clamps (hailFirstHours >= 8 h, ...) bound the
// config value; they do not apply to the Cheat minute.
//
// Two of these waits are stored as whole game hours (the reject cooldown and the
// lost-duel cooldown end at a rounded-up game hour, saved_ints): in Cheat they end at
// the next whole game hour, never more than about an hour away.

#include "config.h"

namespace totalrecruit
{
// One game minute, in game hours.
extern const float kCheatWaitHours;

// The wait to use for `configuredHours` under `preset`: in Cheat
// min(configured, kCheatWaitHours), a negative value reading as 0; any other
// preset gets `configuredHours` back unchanged.
float EffectiveWaitHours(int preset, float configuredHours);

// Every hours-based wait a Cheat player can meet, read through EffectiveWaitHours.
float CageWaitHours(const Config& config);          // cheatHoursCaged (route_cheat)
float HailFirstHours(const Config& config);         // hailFirstHours (hail.cpp)
float HailRepeatHours(const Config& config);        // hailRepeatHours (hail.cpp)
float RejectCooldownHours(const Config& config);    // submission.rejectCooldownHours
float DuelLossCooldownHours(const Config& config);  // duelChallenges.cooldownHours
float BeatingCooldownHours(const Config& config);   // submission.beatingCooldownHours
float KnockoutCooldownHours(const Config& config);  // submission.knockoutCooldownHours
} // namespace totalrecruit
