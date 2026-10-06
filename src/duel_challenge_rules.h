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
//   duel, side A (the player's squad member) won, B = the NPC  Won: whether they join is
//       duel_join_rules.h's call (Balanced: a seeded chance; Cheat: always)
//   duel, B won or a draw                                      Lost: "vuelve cuando seas
//       mas fuerte", and no new challenge for cooldownHours (saved with the NPC as
//       total_recruit.duel_cooldown_until)
//
// A duel opened from TotalSparring's own window, with nothing pending, resolves the
// same way when JudgeExternalBout says it counts (duel-any-source U2).
//
// A challenge nobody finished is forgotten after expiryHours. A win that does not join
// (duel_join_rules.h) puts its own cooldown into the same book (SetDuelCooldownUntil),
// so it is saved with the NPC the same way.
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
    // Side B is one of the player's characters too (a squad sparring pair). An
    // unreadable B reads as one (ts_bridge.cpp), so it never counts as an outsider.
    bool bIsSquadMember;

    DuelBoutRecord()
        : isDuel(false), outcome(DuelBout_Undecided), aKnown(false), a(),
          aIsSquadMember(false), bKnown(false), b(), bIsSquadMember(false)
    {
    }
};

// A TotalSparring duel with no TotalRecruit challenge pending (duel-any-source U2): the
// player opened it from TotalSparring's own window. It counts as if RETAR A DUELO had
// opened it when duelCountExternalBouts is on, it is a decided duel, side A is a squad
// member and side B a known outsider, and B passes the same offer rule the button uses
// (CanOfferDuel, read by the engine at bout time). Otherwise it is ignored, as before.
enum ExternalBoutReason
{
    ExternalBout_Counts,
    // The bout's shape passes; the engine still has to read the offer rule for B.
    ExternalBout_CheckOffer,
    ExternalBout_NotDuel,
    // A TotalRecruit challenge is pending for B: Resolve answers it the usual way.
    ExternalBout_Pending,
    ExternalBout_SwitchOff,
    ExternalBout_Undecided,
    // Side A or B unreadable, or side A not a squad member.
    ExternalBout_SideUnknown,
    // Both sides are the player's: squad sparring never counts.
    ExternalBout_SquadPair,
    // B would not be offered RETAR A DUELO (level, tier, cooldown, animal rules...).
    ExternalBout_NotOffered
};

struct ExternalBoutFacts
{
    // duelChallenges.countExternalBouts.
    bool countExternal;
    // A TotalRecruit challenge is pending for side B.
    bool hasPending;
    // Whether `offered` was read yet, and what it said (CanOfferDuel for B).
    bool offerChecked;
    bool offered;

    ExternalBoutFacts() : countExternal(false), hasPending(false), offerChecked(false), offered(false) {}
};

// Checks in order: duel, pending, switch, decided, sides, squad pair, then the offer
// (CheckOffer while it was not read, so the engine reads B only for a real candidate).
ExternalBoutReason JudgeExternalBout(const DuelBoutRecord& bout, const ExternalBoutFacts& facts);

// For the log: "counts", "not a duel", "pending challenge", "switch off", "undecided",
// "side unknown", "squad pair", "not offered", "check offer".
const char* ExternalBoutReasonWord(ExternalBoutReason reason);

enum DuelResolution
{
    DuelResolution_Ignored,
    // A squad member won: duel_join_rules.h decides whether they join.
    DuelResolution_Won,
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
    // The Cheat preset: no tiers, the level alone decides (any person, not an animal).
    bool cheatPreset;
    bool isAnimal;
    int npcLevel;
    // allyTiers.eliteFromLevel: the 80+ cut (Balanced people).
    int eliteFromLevel;
    // duelChallenges.cheatPeopleFromLevel: the Cheat cut for people (duel-any-source
    // U1, 1: every level).
    int cheatPeopleFromLevel;
    // A lost duel's cooldown still runs for this NPC.
    bool onCooldown;
    // The animal branch (animal-duel-submission D1), read only when `isAnimal`:
    // duelChallenges.animalsEnabled with the animal not excluded (exclude.animals) and,
    // in Balanced, animals switched on; whether it belongs to someone (the taming
    // route's BelongsToSomeone, outside the player's cage); and the level it needs
    // (duelChallenges.animalsFromLevel, 40).
    bool animalAllowed;
    bool animalOwned;
    int animalFromLevel;
    // duel-any-source U7: the question is about a TotalSparring duel nobody challenged
    // through TotalRecruit (JudgeExternalBout), not about the popup button. Then a
    // Balanced person also counts without an ally tier when `recruitable` (the
    // exclusions pass) at duelChallenges.externalBalancedFromLevel (1) or above.
    bool externalBout;
    bool recruitable;
    int externalBalancedFromLevel;

    DuelOfferFacts()
        : enabled(false), tsAvailable(false), allyTiered(false), cheatPreset(false), isAnimal(false),
          npcLevel(0), eliteFromLevel(80), cheatPeopleFromLevel(1), onCooldown(false), animalAllowed(false),
          animalOwned(false), animalFromLevel(40), externalBout(false), recruitable(false),
          externalBalancedFromLevel(1)
    {
    }
};

// The one predicate: enabled, TotalSparring available and no cooldown running; then
//   a person:  a Balanced tiered ally at the elite cut level or above, or, in the Cheat
//              preset, anyone at cheatPeopleFromLevel or above; for an external bout
//              (U7) a Balanced person also counts when recruitable at
//              externalBalancedFromLevel or above, tier or not;
//   an animal: allowed, owned by nobody, at animalFromLevel or above (both presets: an
//              animal has no ally tier and no 80+ cut).
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
    // The challenged NPC (Won / Lost), and who challenged them.
    CharKey npc;
    CharKey challenger;
    // Won: the squad member who won (side A). TotalSparring's window may switch
    // fighters, so this is whose head-to-head record counts the win.
    CharKey winner;
    // Lost: the game hour the cooldown ends.
    int cooldownUntilHour;
    // Resolved as an external TotalSparring duel (no pending challenge; the
    // challenger is side A).
    bool external;

    DuelResolveResult()
        : resolution(DuelResolution_Ignored), npc(), challenger(), winner(), cooldownUntilHour(0),
          external(false)
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

    // What a finished bout means (see the table at the top). Won and Lost end the
    // challenge; Lost also starts the cooldown. With no pending challenge for B,
    // `externalEligible` (JudgeExternalBout said Counts) resolves a decided duel of a
    // squad member against an outsider the same way, side A as the challenger.
    DuelResolveResult Resolve(const DuelBoutRecord& bout, double nowHours,
                              const DuelChallenges& config, bool externalEligible = false);

    bool OnCooldown(const CharKey& npc, double nowHours) const;
    // 0 when none.
    int CooldownUntil(const CharKey& npc) const;
    // 0 or less forgets it.
    void SetCooldownUntil(const CharKey& npc, int untilHour);

    // Everything (load begin).
    void Clear();

private:
    DuelResolveResult ResolveExternal(const DuelBoutRecord& bout, double nowHours,
                                      const DuelChallenges& config);

    std::map<CharKey, PendingDuel> pending_;
    std::map<CharKey, int> cooldownUntil_;
};

// ----------------------------------------------- The process-wide instance
void RecordDuelChallenge(const CharKey& npc, const CharKey& challenger, double nowHours);
void WithdrawDuelChallenge(const CharKey& npc);
int ExpireDuelChallenges(double nowHours, float expiryHours);
DuelResolveResult ResolveDuelBout(const DuelBoutRecord& bout, double nowHours,
                                  const DuelChallenges& config, bool externalEligible = false);
// A challenge waits for `npc` (expired or not).
bool DuelChallengePending(const CharKey& npc);
bool DuelOnCooldown(const CharKey& npc, double nowHours);
// A win that does not join (duel_join_rules.h): its cooldown end hour; 0 or less
// forgets it.
void SetDuelCooldownUntil(const CharKey& npc, int untilHour);

// The saved-int bridge (saved_ints.cpp, row total_recruit.duel_cooldown_until):
// the end hour to write for the character carrying `persistedId` (0 = none), and
// the one a loading save held.
int SavedDuelCooldownUntil(const std::string& persistedId);
void RestoreSavedDuelCooldownUntil(const std::string& persistedId, int untilHour);

// Load begin: the pending challenges and the cooldowns of the abandoned world.
void ForgetAllDuelChallenges();
} // namespace totalrecruit
