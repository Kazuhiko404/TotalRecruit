#pragma once

// The duel challenge, without the engine.
//
// User request (2026-10-03): "The duel for the 80+ tier: challenge him to a duel so
// he joins you. TotalSparring already has the duel logic (it decides friendly vs to
// the death); we only need to keep the counter after the duel and communicate
// between plugins."
//
// So TotalRecruit never fights anything itself. The recruit popup of a Balanced ally
// at the elite cut level or above (allyTiers.eliteFromLevel, 80) offers RETAR A
// DUELO next to the tier's own answer when TotalSparring is there (CanOfferDuel, the
// one predicate). The click records a pending challenge for that NPC and opens
// TotalSparring's own Dueling window (ts_bridge.h). TotalSparring reports every
// finished bout; this decides what one means for a pending challenge
// (DuelChallengeBook::Resolve):
//
//   spar, undecided (cancelled, interrupted), unrelated pair   Ignored
//       (an undecided duel keeps the challenge until it expires)
//   duel, side A (the player's squad member) won, B = the NPC  Joined: recruited for good
//   duel, B won or a draw                                      Lost: "vuelve cuando seas
//       mas fuerte", and no new challenge for cooldownHours (saved with the NPC as
//       total_recruit.duel_cooldown_until)
//
// A challenge nobody finished is forgotten after expiryHours.
//
// DuelChallengeBook is the plain table (tests/rules_tests/duel_challenge_rules_tests.cpp);
// the free functions at the end are the one process-wide instance behind a lock,
// because the save bridge may run off the game thread.

#include <map>
#include <string>
#include <vector>

#include "char_key.h"
#include "config.h"

namespace totalrecruit
{
// TS_BoutEvent::outcome, mirrored so this stays free of the TotalSparring header.
enum DuelBoutOutcome
{
    DuelBout_Undecided,
    DuelBout_AWon,
    DuelBout_BWon,
    DuelBout_Draw
};

// One finished bout as TotalRecruit keeps it: the plain copy ts_bridge.cpp makes
// inside TotalSparring's callback (the Character pointers die with the callback, the
// keys do not).
struct DuelBoutRecord
{
    bool isDuel;
    DuelBoutOutcome outcome;
    // Side A, the player's squad member, and whether the key could be read.
    bool aKnown;
    CharKey a;
    bool aIsSquadMember;
    // Side B, the NPC.
    bool bKnown;
    CharKey b;

    DuelBoutRecord()
        : isDuel(false), outcome(DuelBout_Undecided), aKnown(false), a(),
          aIsSquadMember(false), bKnown(false), b()
    {
    }
};

enum DuelResolution
{
    DuelResolution_Ignored,
    DuelResolution_Joined,
    DuelResolution_Lost
};

// What decides whether the popup offers RETAR A DUELO.
struct DuelOfferFacts
{
    // duelChallengeEnabled.
    bool enabled;
    // TotalSparring is loaded, attached, listening and ready (ts_bridge.h TsAvailable).
    bool tsAvailable;
    // The verdict came from the Balanced ally tiers (Verdict::allyTiered): an ally in
    // Balanced with the tiers on.
    bool allyTiered;
    bool isAnimal;
    int npcLevel;
    // allyTiers.eliteFromLevel: the 80+ cut.
    int eliteFromLevel;
    // A lost duel's cooldown still runs for this NPC.
    bool onCooldown;

    DuelOfferFacts()
        : enabled(false), tsAvailable(false), allyTiered(false), isAnimal(false),
          npcLevel(0), eliteFromLevel(80), onCooldown(false)
    {
    }
};

// The one predicate: enabled, TotalSparring available, a Balanced tiered ally (not
// an animal) at the elite cut level or above, and no cooldown running.
bool CanOfferDuel(const DuelOfferFacts& facts);

// The game hour a lost duel's cooldown ends: ceil(now + hours). Hours of 0 or less
// end it now (no cooldown).
int DuelCooldownUntilHour(double nowHours, float cooldownHours);

// True while `untilHour` lies ahead of `nowHours`.
bool DuelCooldownActive(int untilHour, double nowHours);

struct PendingDuel
{
    CharKey challenger;
    double sinceHour;

    PendingDuel() : challenger(), sinceHour(0.0) {}
};

// True once a challenge has waited longer than `expiryHours` (a clock that went
// backwards, after a load, never expires it).
bool DuelPendingExpired(const PendingDuel& pending, double nowHours, float expiryHours);

// The head-to-head record as the popup and the card print it: "2-1", or "2-1-1"
// when there were draws. Negative counts read as 0.
std::string FormatDuelRecord(int wins, int losses, int draws);

struct DuelResolveResult
{
    DuelResolution resolution;
    // The challenged NPC (Joined / Lost), and who challenged them.
    CharKey npc;
    CharKey challenger;
    // Lost: the game hour the cooldown ends.
    int cooldownUntilHour;

    DuelResolveResult()
        : resolution(DuelResolution_Ignored), npc(), challenger(), cooldownUntilHour(0)
    {
    }
};

class DuelChallengeBook
{
public:
    // `challenger` challenged `npc` at `nowHours`; replaces an older challenge.
    void Challenge(const CharKey& npc, const CharKey& challenger, double nowHours);
    bool Pending(const CharKey& npc, PendingDuel* out) const;
    void Withdraw(const CharKey& npc);

    // Drops every challenge older than `expiryHours`; their NPCs go to `expired`
    // (optional). Returns how many.
    int ExpirePending(double nowHours, float expiryHours, std::vector<CharKey>* expired);

    // What a finished bout means (see the table at the top). Joined and Lost end the
    // challenge; Lost also starts the cooldown.
    DuelResolveResult Resolve(const DuelBoutRecord& bout, double nowHours,
                              const DuelChallenges& config);

    bool OnCooldown(const CharKey& npc, double nowHours) const;
    // 0 when none.
    int CooldownUntil(const CharKey& npc) const;
    // 0 or less forgets it.
    void SetCooldownUntil(const CharKey& npc, int untilHour);

    // Everything (load begin).
    void Clear();

private:
    std::map<CharKey, PendingDuel> pending_;
    std::map<CharKey, int> cooldownUntil_;
};

// ----------------------------------------------- The process-wide instance
void RecordDuelChallenge(const CharKey& npc, const CharKey& challenger, double nowHours);
void WithdrawDuelChallenge(const CharKey& npc);
int ExpireDuelChallenges(double nowHours, float expiryHours);
DuelResolveResult ResolveDuelBout(const DuelBoutRecord& bout, double nowHours,
                                  const DuelChallenges& config);
bool DuelOnCooldown(const CharKey& npc, double nowHours);

// The saved-int bridge (saved_ints.cpp, row total_recruit.duel_cooldown_until):
// the end hour to write for the character carrying `persistedId` (0 = none), and
// the one a loading save held.
int SavedDuelCooldownUntil(const std::string& persistedId);
void RestoreSavedDuelCooldownUntil(const std::string& persistedId, int untilHour);

// Load begin: the pending challenges and the cooldowns of the abandoned world.
void ForgetAllDuelChallenges();
} // namespace totalrecruit
