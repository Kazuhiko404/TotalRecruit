#pragma once

// Who was offended by a lowball, and on which in-game day, without the engine.
//
// An offended NPC will not haggle again for the rest of that day and asks more
// (haggleOffenceMarkup) until the day turns; the next day both reset by
// themselves, because every query is "offended ON this day". It lives only in
// memory, like the hold clock: nothing is written to the save, and the load
// boundary clears it (load_boundary.cpp), so a reload forgives the insult rather
// than carrying it into another save's world.

#include "char_key.h"
#include "config.h"

#include <map>

namespace totalrecruit
{
class HaggleLedger
{
public:
    // The NPC refused a haggle on `dayIndex`. A later offence replaces an earlier
    // one; there is only ever one day that matters per NPC.
    void RecordOffence(const CharKey& who, int dayIndex);

    // Haggling is closed with them for the rest of `dayIndex`.
    bool IsHaggleLocked(const CharKey& who, int dayIndex) const;

    // What their asking price is multiplied by on `dayIndex`: 1 + haggleOffenceMarkup
    // on the offending day, 1 on every other.
    float OffendedMarkup(const CharKey& who, int dayIndex, const Prices& prices) const;

    int Count() const;
    void Clear();

private:
    std::map<CharKey, int> offendedOnDay_;
};

// The one ledger the plugin keeps for the session. The engine-bound side reads
// it by (key, day) (character_facts.cpp) and the popup records into it; the load
// boundary clears it.
HaggleLedger& SessionHaggleLedger();
} // namespace totalrecruit
