#pragma once

// A temporary squad member's stay, without the engine.
//
// A Balanced ally of level 80 or more joins for a few days only (ally_tier_rules.h,
// AllyTierMode_Temporary). From the moment they join, this decides what happens:
//
//   - Their time: expiryHour = the game hour they joined + days * 24. When the clock
//     reaches it, they leave (TempLeave_Expired). TempMembers::expiryWarningHours
//     before that they warn once ("me ire pronto").
//   - Mistreatment strikes, three kinds:
//       hunger      hungry (stomach under hungerThreshold, or the engine says
//                   starving) for hungryHours in a row;
//       attacked    one of the player's characters is attacking them;
//       knockedOut  they were just put down (the edge into a knockout) and the one
//                   who put them down is one of the player's.
//     Each kind counts at most once per strikeCooldownHours. At maxStrikes they
//     leave (TempLeave_Mistreated, before any expiry); the strike that brings them
//     to maxStrikes - 1 comes with a warning ("si sigues asi me voy"), once.
//
//   - The daily notice (TempMembers::dailyNotice): once per game day they say how
//     many days they have left ("Me quedan 4 dias"), never on the day they joined
//     (the join notice said the total), never on the day "me ire pronto" goes up,
//     and never on a step that already says something (it waits for the next one).
//
// The expiry hour and the strike count are saved with the character (saved_ints.h
// rows, temp_member.cpp); the rest (cooldowns, hunger streak, warnings, the
// knockout edge, the join day and the last announced day) is session state and
// starts over after a load: at worst a warning or the day's count is said once more.
//
// TempMemberBook is the plain table of who is a member, keyed by CharKey; the engine
// side (temp_member.cpp) holds one behind a lock, because the serialise hook that
// reads it is not guaranteed to run on the game thread.

#include "char_key.h"
#include "config.h"

#include <map>
#include <vector>

namespace totalrecruit
{
enum TempStrikeKind
{
    TempStrike_Hunger,
    TempStrike_Attacked,
    TempStrike_KnockedOut,
    TempStrike_KindCount
};

enum TempLeave
{
    TempLeave_Stay,
    TempLeave_Expired,
    TempLeave_Mistreated
};

struct TempMembership
{
    // Saved. The game hour their stay ends; 0 or less: not a member.
    int expiryHour;
    // Saved. Mistreatment strikes so far.
    int strikes;
    // Session. When each kind last struck; negative: never.
    double lastStrikeHour[TempStrike_KindCount];
    // Session. The "si sigues asi me voy" and "me ire pronto" warnings were given.
    bool warned;
    bool expiryWarned;
    // Session. Since when they have been hungry without a break; negative: not now.
    double hungrySince;
    // Session. They were down at the last step (a knockout strikes on the edge only).
    bool wasKnockedOut;
    // Session. The game day they joined (no daily notice that day); -1: unknown, as
    // after a load.
    int joinDay;
    // Session. The game day of the last daily notice; -1: none yet.
    int lastDailyDay;

    TempMembership();
    bool IsMember() const { return expiryHour > 0; }
};

// What the engine read about a member this step.
struct TempMemberSignals
{
    double nowHours;
    bool hungry;
    bool attackedBySquad;
    bool knockedOut;
    // Down now and the one who put them down is one of the player's.
    bool knockedOutBySquad;

    TempMemberSignals()
        : nowHours(0.0), hungry(false), attackedBySquad(false), knockedOut(false),
          knockedOutBySquad(false)
    {
    }
};

// What one step decided.
struct TempMemberStep
{
    // One bit per TempStrikeKind that struck this step.
    int struckMask;
    // The strike that brought them to maxStrikes - 1: say "si sigues asi me voy".
    bool warn;
    // Their time is nearly up: say "me ire pronto".
    bool expiryWarn;
    // Today's "me quedan {days} dias" (dailyDays days left).
    bool dailyNotice;
    int dailyDays;
    TempLeave leave;

    TempMemberStep()
        : struckMask(0), warn(false), expiryWarn(false), dailyNotice(false), dailyDays(0),
          leave(TempLeave_Stay)
    {
    }
    bool Struck() const { return struckMask != 0; }
    bool StruckBy(TempStrikeKind kind) const { return (struckMask & (1 << (int)kind)) != 0; }
};

// The game hour a stay of `days` that starts at `nowHours` ends (rounded up, at
// least 1). Days under 1 count as 1.
int TempExpiryHour(double nowHours, int days);

// Whole days left until `expiryHour`, rounded up (23 hours left is 1 day); 0 once
// the time is up.
int TempDaysLeft(int expiryHour, double nowHours);

// Hungry for the hunger strike: the engine says starving, or the stomach (0..1 of
// full) is under the threshold. A threshold of 0 leaves only the engine's word.
bool IsTempMemberHungry(float fullness01, bool engineStarving, float hungerThreshold);

// A new member whose stay ends at `expiryHour`, who joined on game day `joinDay`
// (-1: unknown).
TempMembership NewTempMembership(int expiryHour, int joinDay = -1);

// What decides the daily days-left notice.
struct TempDailyNoticeFacts
{
    // Today (chance_rules.h DayIndexFromHours).
    int dayIndex;
    // The day of the last daily notice (-1: none) and the join day (-1: unknown).
    int lastAnnouncedDay;
    int joinDay;
    int daysLeft;
    // "Me ire pronto" goes up today (or just did).
    bool warningToday;
    // Something else is said this step: the daily notice waits.
    bool otherNoticeNow;

    TempDailyNoticeFacts()
        : dayIndex(0), lastAnnouncedDay(-1), joinDay(-1), daysLeft(0), warningToday(false),
          otherNoticeNow(false)
    {
    }
};

// True once per game day: not the join day, not a day already announced, not the
// expiry warning's day, not while another notice is said, and only with days left.
bool ShouldAnnounceDaysLeft(const TempDailyNoticeFacts& facts);

// True when the expiry warning (expiryHour - warningHours) falls on game day
// `dayIndex`.
bool TempExpiryWarningFallsOnDay(int expiryHour, float warningHours, int dayIndex);

// One step of a member's stay: updates `m` (strikes, cooldowns, warnings, hunger
// streak, knockout edge) and says what to tell the player and whether they leave.
TempMemberStep StepTempMember(TempMembership* m, const TempMemberSignals& signals,
                              const TempMembers& config);

// A saved value as read back from a save: negative is none.
int SanitiseSavedTempValue(int raw);

// Who is a temporary member right now.
class TempMemberBook
{
public:
    // `who` joined on game day `joinDay` (-1: unknown); their stay ends at
    // `expiryHour`. Replaces any earlier record.
    void Join(const CharKey& who, int expiryHour, int joinDay = -1);
    // The record, or NULL when `who` is not a member.
    const TempMembership* Find(const CharKey& who) const;
    // Replaces the record of a member; nothing for a non-member.
    void Update(const CharKey& who, const TempMembership& m);
    // They are no longer a member (they left, or died).
    void Remove(const CharKey& who);
    // Every member, oldest key first.
    std::vector<CharKey> Members() const;
    size_t Count() const;

    // The save bridge: what to write for `who` (0 when not a member) and what a
    // loading save held. Restoring a strike count without an expiry yet keeps it
    // for the expiry that follows; a record whose expiry never comes is no member.
    int SavedExpiry(const CharKey& who) const;
    int SavedStrikes(const CharKey& who) const;
    void RestoreExpiry(const CharKey& who, int expiryHour);
    void RestoreStrikes(const CharKey& who, int strikes);

    void Clear();

private:
    std::map<CharKey, TempMembership> records_;
};
} // namespace totalrecruit
