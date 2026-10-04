#pragma once

// The free helper, without the engine.
//
// User request (2026-10-03): "There should be a chance that higher-level allied NPCs
// with good personalities offer to help you for some days, but you can't control
// them -- similar to how Kenshi's mercenaries work, but free."
//
// The engine side (helper.cpp) reads, this decides:
//
//   - Who may offer (ScreenHelperCandidate): the event switched on, the Balanced
//     preset, not one of the player's, not an animal, up, not caged, not busy (a
//     temporary member, already a helper, someone else's event), not fighting,
//     within kHelperTriggerMeters of the squad leader, allied (relation known and at
//     or above friendlyRelation), Good or Neutral (personality_groups.h), a level
//     over the leader's + minLevelLead, on no cooldown, and not already rolled today.
//   - The day's roll (HelperOfferRoll, seeded with DailySeed under kHelperOfferSalt)
//     and how many days they stay (HelperDaysFromSeed, under kHelperDaysSalt).
//   - The contract's step (StepHelper): the expiry hour is TempExpiryHour's (now +
//     days * 24, rounded up; temp_member_rules.h), "me ire pronto" once
//     expiryWarningHours before it, and the ways it ends -- dead, recruited into the
//     squad, knocked out by the squad, attacked by the squad, time up, in that order.
//   - The escort ladder (NextEscortRung): BODYGUARD first; left behind (farther than
//     kHelperFarMeters, out of combat) for kHelperFarChecksToEscalate checks in a row
//     it climbs to FOLLOW_PLAYER_ORDER, then to the walk-up's own move order (the one
//     order this mod has seen work on a foreign NPC). The rung never comes down.
//   - Whom it fights for the player (ShouldAssist): the leader's own target while the
//     leader is fighting, never one of the squad, itself, its own faction, or someone
//     already down.
//
// HelperBook is the plain table: the contracts, the reject cooldowns and the day each
// NPC last rolled, keyed by CharKey. helper.cpp holds one behind a lock: the
// serialise hook that reads the saved values is not guaranteed to run on the game
// thread. The expiry hour and the reject cooldown are saved with the character
// (saved_ints.h); the rest is session state.

#include "char_key.h"
#include "config.h"
#include "temperament.h"

#include <map>
#include <vector>

namespace totalrecruit
{
// Salts that keep the helper's draws apart from every other daily draw.
const unsigned int kHelperOfferSalt = 0x48454C50u;   // "HELP"
const unsigned int kHelperDaysSalt  = 0x48444159u;   // "HDAY"

// Who is close enough to walk up and offer.
const float kHelperTriggerMeters = 40.0f;
// Farther than this from the leader, out of combat, counts as left behind.
const float kHelperFarMeters = 15.0f;
// Left behind this many escort checks in a row climbs one rung.
const int kHelperFarChecksToEscalate = 3;
// A helper whose pointer was lost (a load) is looked for this close to the leader.
const float kHelperRearmMeters = 100.0f;

struct HelperCandidate
{
    bool enabled;
    bool balanced;
    bool mine;
    bool animal;
    bool up;
    bool caged;
    // A temporary member, already a helper, or the NPC of an event in progress.
    bool busy;
    bool inCombat;
    // To the squad leader; negative = could not be read.
    float distanceMeters;
    bool relationKnown;
    float relation;
    Temperament temperament;
    int npcLevel;
    int leaderLevel;
    // Any reject cooldown: the helper's own, or the submission one.
    bool onCooldown;
    bool rolledToday;

    HelperCandidate()
        : enabled(false), balanced(false), mine(false), animal(false), up(false), caged(false),
          busy(false), inCombat(false), distanceMeters(-1.0f), relationKnown(false),
          relation(0.0f), temperament(Temperament_Normal), npcLevel(1), leaderLevel(1),
          onCooldown(false), rolledToday(false)
    {
    }
};

// Why a candidate does not offer, in the order the screen checks.
enum HelperSkip
{
    HelperSkip_None,
    HelperSkip_Disabled,
    HelperSkip_NotBalanced,
    HelperSkip_Mine,
    HelperSkip_Animal,
    HelperSkip_Down,
    HelperSkip_Caged,
    HelperSkip_Busy,
    HelperSkip_InCombat,
    HelperSkip_TooFar,
    HelperSkip_NotAllied,
    HelperSkip_Personality,
    HelperSkip_LevelLead,
    HelperSkip_Cooldown,
    HelperSkip_RolledToday
};

HelperSkip ScreenHelperCandidate(const HelperCandidate& c, const Helpers& config,
                                 float friendlyRelation);
// Short English, for the "[helper]" log.
const char* HelperSkipText(HelperSkip skip);

// Good or Neutral (personality_groups.h).
bool HelperPersonalityAllowed(Temperament temperament);

// The day's draw: seed % 10000 under chance * 10000. 0 or less never, 1 or more always.
bool HelperOfferRoll(unsigned int seed, float chancePerDay);

// How many days the help lasts: daysMin..daysMax inclusive. A max under the min reads
// as the min; a min under 1 as 1.
int HelperDaysFromSeed(unsigned int seed, const Helpers& config);

struct HelperContract
{
    // Saved. The game hour the help ends; 0 or less: no contract.
    int expiryHour;
    // Session. "Me ire pronto" was said.
    bool expiryWarned;

    HelperContract() : expiryHour(0), expiryWarned(false) {}
    bool Active() const { return expiryHour > 0; }
};

HelperContract NewHelperContract(int expiryHour);

// What the engine read about a helper this step.
struct HelperSignals
{
    double nowHours;
    bool dead;
    // They are one of the player's now (recruited through Hablar).
    bool joinedSquad;
    bool attackedBySquad;
    // Down now and the one who put them down is one of the player's.
    bool knockedOutBySquad;

    HelperSignals()
        : nowHours(0.0), dead(false), joinedSquad(false), attackedBySquad(false),
          knockedOutBySquad(false)
    {
    }
};

enum HelperEnd
{
    HelperEnd_None,
    HelperEnd_Expired,
    HelperEnd_Attacked,
    HelperEnd_KnockedOut,
    HelperEnd_Died,
    HelperEnd_Joined
};

struct HelperStep
{
    HelperEnd end;
    bool expiryWarn;

    HelperStep() : end(HelperEnd_None), expiryWarn(false) {}
};

// One step of a contract: updates `c` (the warning) and says whether it ends.
HelperStep StepHelper(HelperContract* c, const HelperSignals& signals, const Helpers& config);
const char* HelperEndText(HelperEnd end);
// The squad turned on them: an angry farewell and the reject cooldown.
bool HelperEndIsBetrayal(HelperEnd end);

enum EscortRung
{
    EscortRung_Bodyguard,
    EscortRung_FollowOrder,
    EscortRung_Walk
};

const char* EscortRungName(EscortRung rung);

// The left-behind streak after one check: unknown distance or a fight keeps it, near
// (kHelperFarMeters or closer) resets it, far adds one.
int NextFarChecks(int farChecks, float distanceMeters, bool inCombat);

// The rung after one check: one up once the streak reaches
// kHelperFarChecksToEscalate (the caller resets the streak); Walk is the top.
EscortRung NextEscortRung(EscortRung rung, int farChecks);

// What decides whether the helper goes for the leader's target.
struct AssistFacts
{
    bool leaderInCombat;
    bool haveTarget;
    bool targetIsSquad;
    bool targetIsHelper;
    bool targetSameFaction;
    bool targetDown;

    AssistFacts()
        : leaderInCombat(false), haveTarget(false), targetIsSquad(false), targetIsHelper(false),
          targetSameFaction(false), targetDown(false)
    {
    }
};

bool ShouldAssist(const AssistFacts& facts);

class HelperBook
{
public:
    // `who` helps until `expiryHour`. Replaces any earlier contract.
    void Start(const CharKey& who, int expiryHour);
    // The contract, or NULL.
    const HelperContract* Find(const CharKey& who) const;
    // Replaces the contract of a helper; nothing for anyone else.
    void Update(const CharKey& who, const HelperContract& c);
    void End(const CharKey& who);
    std::vector<CharKey> Keys() const;
    size_t Count() const;

    // The reject cooldown: no offer before `untilHour`.
    void SetRejectedUntil(const CharKey& who, int untilHour);
    int RejectedUntil(const CharKey& who) const;
    bool OnRejectCooldown(const CharKey& who, double nowHours) const;

    // One roll a day per NPC.
    bool RolledOn(const CharKey& who, int day) const;
    void NoteRolled(const CharKey& who, int day);

    // The save bridge: what to write for `who` (0 = none) and what a loading save
    // held (0 or less is none).
    int SavedExpiry(const CharKey& who) const;
    void RestoreExpiry(const CharKey& who, int expiryHour);
    int SavedRejectedUntil(const CharKey& who) const;
    void RestoreRejectedUntil(const CharKey& who, int untilHour);

    void Clear();

private:
    std::map<CharKey, HelperContract> contracts_;
    std::map<CharKey, int> rejectedUntil_;
    std::map<CharKey, int> rolledDay_;
};
} // namespace totalrecruit
