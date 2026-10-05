#include "insistence_rules.h"

namespace totalrecruit
{
bool ReasonCountsAsInsistence(ReasonKey key)
{
    switch (key)
    {
    case Reason_GrudgeNever:
    case Reason_FieldYieldNotYet:
    case Reason_AllyMaybe:
    case Reason_FactionHonour:
    case Reason_FactionGreedy:
    case Reason_HonourRefusesBribe:
    case Reason_MilitaryLoyalty:
    case Reason_Defiant:
    case Reason_UniqueZealot:
    case Reason_AllyLeaderTooWeak:
    case Reason_AllyCruel:
    case Reason_AllyGreedy:
        return true;
    default:
        // States, instructions, lockouts, gates, every yes, and any reason not
        // classified yet (insistence_rules.h).
        return false;
    }
}

bool CountsAsInsistence(const Verdict& verdict, bool animal)
{
    if (animal || verdict.isAnimal || verdict.willJoin)
    {
        return false;
    }
    return ReasonCountsAsInsistence(verdict.reasonKey);
}
} // namespace totalrecruit
