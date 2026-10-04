#pragma once

// A temporary member who left the squad, walking back to where they joined, without
// the engine (leave_homing.h does the engine side).
//
// User request (2026-10-03): when a temporary member's time is up and they leave,
// they "stand in place, idle, forever"; they should go back to where they were.
// The native fix is the Wandering Squad package (squad_leave.h); this is the fallback
// when that package could not be given:
//
//   - The join spot (FormatJoinSpot / ParseJoinSpot): where they stood when they
//     joined, x and z in whole world units, saved with the character as "x,z"
//     (saved_strings.h, total_recruit.temp_join_spot).
//   - The walk (StepHoming): the move order is re-issued every kHomingReissueSeconds
//     until they are within kHomingArriveMeters of the spot (arrived) or
//     kHomingTimeoutSeconds have passed (gave up); either way their own AI takes over.
//     Seconds are real seconds with the game unpaused.

#include <string>

namespace totalrecruit
{
const float kHomingReissueSeconds = 4.0f;
const float kHomingArriveMeters = 6.0f;
const float kHomingTimeoutSeconds = 240.0f;

enum HomingAction
{
    Homing_Wait,
    Homing_Reissue,
    Homing_Arrived,
    Homing_GaveUp
};

// One check: arrived first (a negative, unread distance never arrives), then the
// timeout, then the re-issue.
HomingAction StepHoming(float sinceStartSeconds, float sinceOrderSeconds, float distanceMeters);
const char* HomingActionText(HomingAction action);

// "x,z". Parse takes exactly two integers separated by one comma (spaces around them
// allowed); anything else, "" included, is no spot.
std::string FormatJoinSpot(int x, int z);
bool ParseJoinSpot(const std::string& text, int* x, int* z);
} // namespace totalrecruit
