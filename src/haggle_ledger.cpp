#include "haggle_ledger.h"

namespace totalrecruit
{
void HaggleLedger::RecordOffence(const CharKey& who, int dayIndex)
{
    offendedOnDay_[who] = dayIndex;
}

bool HaggleLedger::IsHaggleLocked(const CharKey& who, int dayIndex) const
{
    std::map<CharKey, int>::const_iterator found = offendedOnDay_.find(who);
    return found != offendedOnDay_.end() && found->second == dayIndex;
}

float HaggleLedger::OffendedMarkup(const CharKey& who, int dayIndex, const Prices& prices) const
{
    return IsHaggleLocked(who, dayIndex) ? 1.0f + prices.haggleOffenceMarkup : 1.0f;
}

int HaggleLedger::Count() const
{
    return (int)offendedOnDay_.size();
}

void HaggleLedger::Clear()
{
    offendedOnDay_.clear();
}

HaggleLedger& SessionHaggleLedger()
{
    static HaggleLedger ledger;
    return ledger;
}
} // namespace totalrecruit
