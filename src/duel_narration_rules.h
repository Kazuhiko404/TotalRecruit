#pragma once

// How a duel is told to the player, without the engine.
//
// User request (2026-10-04, animal-duel-submission D2): a free animal can be challenged
// to a TotalSparring duel like a person. A person tells the outcome in their own voice
// ("Me ganaste. Me uno a ti" over their head) and the roller line names them. An animal
// never speaks (mercy.cpp: an animal has no words to beg with), so it gets narrator
// lines of its own (messages.json duel.animal.*) and no notice over its head:
//
//   line         person            animal
//   Challenged   duel.challenged   duel.animal.challenged
//   Joined       duel.joined       duel.animal.joined
//   Stays        duel.stays        duel.animal.stays   ({hours}, {chance})
//   Lost         duel.lost         duel.animal.lost    ({hours})
//   NeedMoreWins duel.needMoreWins duel.animal.needMoreWins ({wins})
//   head notice  yes               never
//
// Each line carries its messages.json key and the built-in text used when the file has
// none ({name}, {hours}, {chance} filled by the caller). The engine side (recruit_core.cpp
// OnDuelWon / OnDuelLost / ChallengeToDuel) only asks; tests/rules_tests/
// duel_narration_rules_tests.cpp.

namespace totalrecruit
{
enum DuelNarrationId
{
    DuelNarration_Challenged,
    DuelNarration_Joined,
    DuelNarration_Stays,
    DuelNarration_Lost,
    // A win under the minimum wins (duel_join_rules.h, duel-any-source U4): {wins} left.
    DuelNarration_NeedMoreWins,
    // Not a line: how many there are.
    DuelNarration_Count
};

struct DuelNarration
{
    const char* key;
    const char* fallback;
};

// The line for `id`, in a person's or an animal's form. An id out of range reads as the
// challenge line.
DuelNarration DuelNarrationFor(DuelNarrationId id, bool animal);

// The built-in minute twin of a line that shows a wait (Stays, Lost), for a wait under
// an hour (wait_format.h; messages.json "<key>Minutes", {minutes} in place of
// {hours}); NULL for the lines with no wait.
const char* DuelMinutesFallback(DuelNarrationId id, bool animal);

// Whether the outcome is also said over the NPC's head (duel.notice.*): a person, yes;
// an animal, never.
bool DuelRaisesHeadNotice(bool animal);
} // namespace totalrecruit
