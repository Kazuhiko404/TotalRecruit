#include "defeat_ledger.h"

namespace totalrecruit
{
const int kMaxSavedDefeats = 1000;

int SanitiseSavedDefeats(int raw)
{
    if (raw < 0) return 0;
    return raw > kMaxSavedDefeats ? kMaxSavedDefeats : raw;
}

int DefeatLedger::Get(const CharKey& key) const
{
    Counts::const_iterator it = counts_.find(key);
    return it == counts_.end() ? 0 : it->second;
}

void DefeatLedger::Set(const CharKey& key, int count)
{
    const int safe = SanitiseSavedDefeats(count);
    if (safe <= 0)
    {
        counts_.erase(key);
        return;
    }
    counts_[key] = safe;
}

int DefeatLedger::Add(const CharKey& key)
{
    int& count = counts_[key];
    if (count < kMaxSavedDefeats)
    {
        ++count;
    }
    return count;
}

void DefeatLedger::Clear()
{
    counts_.clear();
}

int DefeatLedger::Size() const
{
    return (int)counts_.size();
}
} // namespace totalrecruit
