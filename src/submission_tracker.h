#pragma once

// What the submission event remembers per character, without the engine.
//
//  - Who was already asked, and at what defeat count: an NPC asks once, and
//    only a new defeat opens the question again (submission_rules MayAskAgain).
//  - Whose field-yield roll already failed today: the sweep does not draw the
//    same daily roll again every two seconds.
//  - The reject cooldown: the game hour until which the NPC wants nothing to do
//    with the player. Saved per character as idata
//    "total_recruit.reject_cooldown_until" through the saved-int table
//    (saved_ints.h), next to the persisted id, so it survives a reload.
//
// Keyed by CharKey (char_key.h). The asked and rolled marks are session-only;
// the cooldowns are refilled from the save by each character's load hook after
// the load boundary clears everything.
//
// SubmissionMemory is the plain table (tested in
// tests/rules_tests/submission_tracker_tests.cpp); the free functions are the
// one process-wide instance behind a lock, because the serialise hook is not
// guaranteed to run on the game thread.

#include "char_key.h"

#include <map>
#include <string>

namespace totalrecruit
{
class SubmissionMemory
{
public:
    // The NPC was asked while the player had `defeats` over them.
    void MarkAsked(const CharKey& key, int defeats);
    // Whether the NPC may ask now, with `defeatsNow` (MayAskAgain).
    bool MayAsk(const CharKey& key, int defeatsNow) const;

    // Their field-yield roll failed on `day` (DayIndexFromHours).
    void NoteRollFailed(const CharKey& key, int day);
    bool RollFailedOn(const CharKey& key, int day) const;

    // The reject cooldown's end hour; 0 or less forgets it.
    void SetCooldownUntil(const CharKey& key, int untilHour);
    // 0 when none.
    int CooldownUntil(const CharKey& key) const;

    // Everything (load begin).
    void Clear();

private:
    std::map<CharKey, int> asked_;
    std::map<CharKey, int> rollFailedDay_;
    std::map<CharKey, int> cooldownUntil_;
};

// ----------------------------------------------- The process-wide instance
void MarkSubmissionAsked(const CharKey& key, int defeats);
bool MaySubmissionAsk(const CharKey& key, int defeatsNow);
void NoteSubmissionRollFailed(const CharKey& key, int day);
bool SubmissionRollFailedOn(const CharKey& key, int day);
// The reject cooldown's end, as a game MINUTE (cooldown_minutes.h, priority-one B).
void SetRejectCooldownUntil(const CharKey& key, int untilMinute);
int RejectCooldownUntil(const CharKey& key);

// The saved-int bridge (saved_ints.cpp). Row total_recruit.reject_cooldown_until_min:
// the end minute to write into the save state of the character carrying
// `persistedId`, and the one that save held (sanitised: negative is none). Row
// total_recruit.reject_cooldown_until (legacy, whole hours): the hour the minute
// rounds up to, and an old save's hour read back as that hour's first minute; it
// is restored first, so the minute row wins when a save has both.
int SavedRejectCooldown(const std::string& persistedId);
void RestoreSavedRejectCooldown(const std::string& persistedId, int untilMinute);
int SavedRejectCooldownLegacyHour(const std::string& persistedId);
void RestoreSavedRejectCooldownLegacyHour(const std::string& persistedId, int untilHour);

// Load begin: all of it belongs to the world that was just abandoned.
void ForgetAllSubmissionMemory();
} // namespace totalrecruit
