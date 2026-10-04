#pragma once

// PURE: the stress a beating in the player's own cage leaves behind.
//
// Golpear (one blow) and Noquear (a knockout that landed) no longer count as
// defeats (R3.2 is beating someone in a fight, not hitting a caged prisoner);
// each adds to this bar instead. It is the fourth stress term, next to hunger,
// wounds and exhaustion (chance_rules.h StressTermsFromState), so maxStressChance
// and the temperament's resolve weigh it exactly as they weigh the others.
//
//   gain   = clamp(base * levelFactor / resolve, beatingStressMin, beatingStressMax)
//   base   = submission.beatStressBase (Golpear) or knockoutStressBase (Noquear)
//   levelFactor = clamp(playerLevel / max(1, npcLevel), beatingStressLevelMin,
//                 beatingStressLevelMax) -- playerLevel is the player's squad
//                 leader, the same level JoinChance compares (character_facts.cpp)
//   resolve = ResolveMultiplier(temperament): a coward (0.6) feels it more, the
//             brave (1.6) and the honourable (1.8) less
//
// The bar is 0..1 per character and fades linearly with game hours: a full bar is
// gone in submission.beatingStressFadeHours. Only the value and the game hour it
// was last written at are kept; the fade is applied on every read, so nothing
// ticks. beating_stress.cpp is the engine side (reads, the lock, the save rows).
//
// No Kenshi include: tested in tests/rules_tests/beating_stress_rules_tests.cpp.

#include "char_key.h"
#include "config.h"
#include "temperament.h"

#include <map>

namespace totalrecruit
{
// Which cage command landed.
enum BeatingBlow
{
    BeatingBlow_Hit,       // Golpear's blow
    BeatingBlow_Knockout   // Noquear's knockout
};

// playerLevel / max(1, npcLevel), clamped to [beatingStressLevelMin,
// beatingStressLevelMax]. A level of 0 or less reads as 1; a max below the min
// reads as the min.
float BeatingLevelFactor(int playerLevel, int npcLevel, const Submission& sub);

// What one blow or knockout adds to the bar, 0..1. A base of 0 or less adds
// nothing (the min does not apply: the term is switched off); otherwise the gain
// is clamped to [beatingStressMin, beatingStressMax] (a max below the min reads as
// the min). A resolve of 0.01 or less reads as 1, as in ChanceFromStress.
float BeatingStressGain(BeatingBlow blow, int playerLevel, int npcLevel,
                        Temperament personality, const Submission& sub);

// `value` (clamped to 0..1) as it stands at `nowHours` when it was written at
// `lastHours`: minus (now - last) / fadeHours, never below 0. A clock earlier
// than `lastHours` (a load of an earlier save) does not decay it and never raises
// it; a fade of 0 or less never decays.
float DecayedBeatingStress(float value, double lastHours, double nowHours, float fadeHours);

// The save keeps ints: the value in thousandths, the game hour in tenths.
extern const int kBeatingStressSavedScale;   // 1000
int BeatingStressToSaved(float value);       // rounded, clamped to 0..1000
float BeatingStressFromSaved(int saved);     // clamped to 0..1
int GameHourToSavedTenths(double hours);     // rounded, 0 for a negative clock
double GameHourFromSavedTenths(int tenths);

// The bar per character. Keyed by CharKey like the defeat ledger: the persisted
// id whenever there is one (only those reach a save). Cleared at load begin and
// refilled from each character's save state.
class BeatingStressLedger
{
public:
    // The bar at `nowHours` (0 for someone never beaten).
    float Get(const CharKey& key, double nowHours, float fadeHours) const;

    // Adds `gain` (nothing when 0 or less) to what is left of the bar at
    // `nowHours`, clamps to 1, and restarts the fade from `nowHours`. Returns the
    // new bar.
    float Add(const CharKey& key, float gain, double nowHours, float fadeHours);

    // The two save rows of `key`: the value as last written (thousandths) and the
    // game hour it was written at (tenths); 0 for someone not in the ledger.
    int SavedValue(const CharKey& key) const;
    int SavedHourTenths(const CharKey& key) const;

    // A loading save state puts the rows back, in either order. A value of 0 or
    // less restores nothing; a value whose hour never arrives is read undecayed.
    void RestoreValue(const CharKey& key, int saved);
    void RestoreHourTenths(const CharKey& key, int tenths);

    void Clear();
    int Size() const;

private:
    struct Entry
    {
        float value;
        double lastHours;
        bool haveHour;

        Entry() : value(0.0f), lastHours(0.0), haveHour(false) {}
    };
    typedef std::map<CharKey, Entry> Entries;
    Entries entries_;
};
} // namespace totalrecruit
