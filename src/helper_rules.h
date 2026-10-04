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
//   - The native contract (helper_contract.h): its hours (HelperContractHours), the
//     platoon of its own a helper in a squad needs first (HelperNeedsOwnPlatoon), and
//     what each watch does when the contract is gone (NativeWatchStep: reassign once,
//     then fall back).
//   - The fallback escort, when the native contract is off or failed: re-checked every
//     kHelperEscortRepathMs; PlanEscort stays near, walks, or runs when far; each
//     helper aims at its own spot on a small ring around the leader
//     (EscortOffsetMeters); ShouldReissueAssist renews the attack sequence.
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

// ------------------------------------------------------- The native contract
// The hours the engine's Bodyguard contract is given: what is left of the help, in
// whole hours rounded up, never under 1 (the engine ends a 0-hour contract at once).
int HelperContractHours(int expiryHour, double nowHours);

// What one watch does with a helper on the native contract.
enum NativeWatch
{
    // The platoon still carries the contract.
    NativeWatch_Keep,
    // It is gone (a load, the engine dropped it): assign it again, once.
    NativeWatch_Reassign,
    // Gone again after the one reassignment: the code-driven escort takes over.
    NativeWatch_FallBack
};

NativeWatch NativeWatchStep(bool contractPresent, bool reassignedOnce);
const char* NativeWatchText(NativeWatch step);

// A contract belongs to the whole platoon: a helper standing in a squad of others is
// moved to a platoon of its own first, or the whole squad would escort the player.
bool HelperNeedsOwnPlatoon(int platoonMembers);

// ------------------------------------------------------- The fallback escort
// When the native contract is off (helperNativeBodyguard false) or could not be
// assigned: the code-driven escort, re-checked this often (real milliseconds).
const unsigned int kHelperEscortRepathMs = 1000;
// At or within this distance of the leader it stays where it is.
const float kHelperNearMeters = 5.0f;
// Farther than this it runs to catch up; between the two it walks.
const float kHelperRunMeters = 12.0f;
// The radius of the ring of spots around the leader the helpers walk to.
const float kHelperEscortOffsetMeters = 3.0f;
// While it fights for the leader on the same target, the attack sequence is renewed
// every this many checks anyway (an order the engine dropped would leave it idle).
const int kHelperAssistReissueChecks = 3;

struct EscortPlan
{
    // Issue a move to the leader's spot this check.
    bool move;
    // At a run (else at a walk).
    bool run;

    EscortPlan() : move(false), run(false) {}
};

// The plan for one check, from the distance to the leader (negative = unread: walk to
// the leader, the safe guess).
EscortPlan PlanEscort(float distanceMeters);

// The helper's spot by the leader, in metres from the leader: one of four points on a
// ring of kHelperEscortOffsetMeters, by `slot` (the helper's index; any int).
void EscortOffsetMeters(int slot, float* dxMeters, float* dzMeters);

// Whether the attack sequence is issued again this check: a new target, the helper no
// longer fighting, or kHelperAssistReissueChecks checks since the last one.
bool ShouldReissueAssist(bool targetChanged, bool helperInCombat, int checksSinceOrder);

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
