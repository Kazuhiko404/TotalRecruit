#include "temp_member_rules.h"

#include <cmath>

namespace totalrecruit
{
namespace
{
const double kHoursPerDay = 24.0;
// Rounding slack: an hour count that is a whole number up to float noise stays it.
const double kEpsilon = 1e-6;

bool CooledDown(const TempMembership& m, TempStrikeKind kind, double now, float cooldownHours)
{
    const double last = m.lastStrikeHour[kind];
    return last < 0.0 || now - last >= static_cast<double>(cooldownHours) - kEpsilon;
}

// The strikes this step's signals earn before cooldowns, one bit per kind. Also
// moves the hunger streak and the knockout edge on.
int EarnedStrikes(TempMembership* m, const TempMemberSignals& s, const TempMembers& config)
{
    int earned = 0;

    if (s.hungry)
    {
        if (m->hungrySince < 0.0)
            m->hungrySince = s.nowHours;
        if (s.nowHours - m->hungrySince >= static_cast<double>(config.hungryHours) - kEpsilon)
            earned |= 1 << TempStrike_Hunger;
    }
    else
    {
        m->hungrySince = -1.0;
    }

    if (s.attackedBySquad)
        earned |= 1 << TempStrike_Attacked;

    // Only the edge into a knockout: lying there for an hour is one knockout.
    if (s.knockedOut && !m->wasKnockedOut && s.knockedOutBySquad)
        earned |= 1 << TempStrike_KnockedOut;
    m->wasKnockedOut = s.knockedOut;

    return earned;
}
} // namespace

TempMembership::TempMembership()
    : expiryHour(0), strikes(0), warned(false), expiryWarned(false), hungrySince(-1.0),
      wasKnockedOut(false)
{
    for (int i = 0; i < TempStrike_KindCount; ++i)
        lastStrikeHour[i] = -1.0;
}

int TempExpiryHour(double nowHours, int days)
{
    if (days < 1)
        days = 1;
    const double end = std::ceil(nowHours + days * kHoursPerDay - kEpsilon);
    return end < 1.0 ? 1 : static_cast<int>(end);
}

int TempDaysLeft(int expiryHour, double nowHours)
{
    const double left = static_cast<double>(expiryHour) - nowHours;
    if (expiryHour <= 0 || left <= 0.0)
        return 0;
    return static_cast<int>(std::ceil(left / kHoursPerDay - kEpsilon));
}

bool IsTempMemberHungry(float fullness01, bool engineStarving, float hungerThreshold)
{
    if (engineStarving)
        return true;
    return hungerThreshold > 0.0f && fullness01 < hungerThreshold;
}

TempMembership NewTempMembership(int expiryHour)
{
    TempMembership m;
    m.expiryHour = expiryHour;
    return m;
}

TempMemberStep StepTempMember(TempMembership* m, const TempMemberSignals& s,
                              const TempMembers& config)
{
    TempMemberStep step;
    if (m == NULL || !m->IsMember())
        return step;

    const int earned = EarnedStrikes(m, s, config);
    for (int kind = 0; kind < TempStrike_KindCount; ++kind)
    {
        if ((earned & (1 << kind)) == 0 ||
            !CooledDown(*m, static_cast<TempStrikeKind>(kind), s.nowHours, config.strikeCooldownHours))
            continue;
        m->lastStrikeHour[kind] = s.nowHours;
        ++m->strikes;
        step.struckMask |= 1 << kind;
    }

    const int maxStrikes = config.maxStrikes < 1 ? 1 : config.maxStrikes;
    if (m->strikes >= maxStrikes)
    {
        step.leave = TempLeave_Mistreated;
        return step;
    }
    if (s.nowHours >= static_cast<double>(m->expiryHour))
    {
        step.leave = TempLeave_Expired;
        return step;
    }

    if (step.Struck() && m->strikes >= maxStrikes - 1 && !m->warned)
    {
        m->warned = true;
        step.warn = true;
    }
    const double warnFrom = static_cast<double>(m->expiryHour) - config.expiryWarningHours;
    if (!m->expiryWarned && s.nowHours >= warnFrom - kEpsilon)
    {
        m->expiryWarned = true;
        step.expiryWarn = true;
    }
    return step;
}

int SanitiseSavedTempValue(int raw)
{
    return raw < 0 ? 0 : raw;
}

// ------------------------------------------------------------ The book
void TempMemberBook::Join(const CharKey& who, int expiryHour)
{
    records_[who] = NewTempMembership(expiryHour);
}

const TempMembership* TempMemberBook::Find(const CharKey& who) const
{
    std::map<CharKey, TempMembership>::const_iterator it = records_.find(who);
    return (it != records_.end() && it->second.IsMember()) ? &it->second : NULL;
}

void TempMemberBook::Update(const CharKey& who, const TempMembership& m)
{
    std::map<CharKey, TempMembership>::iterator it = records_.find(who);
    if (it != records_.end() && it->second.IsMember())
        it->second = m;
}

void TempMemberBook::Remove(const CharKey& who)
{
    records_.erase(who);
}

std::vector<CharKey> TempMemberBook::Members() const
{
    std::vector<CharKey> out;
    for (std::map<CharKey, TempMembership>::const_iterator it = records_.begin();
         it != records_.end(); ++it)
    {
        if (it->second.IsMember())
            out.push_back(it->first);
    }
    return out;
}

size_t TempMemberBook::Count() const
{
    return Members().size();
}

int TempMemberBook::SavedExpiry(const CharKey& who) const
{
    const TempMembership* m = Find(who);
    return m != NULL ? m->expiryHour : 0;
}

int TempMemberBook::SavedStrikes(const CharKey& who) const
{
    const TempMembership* m = Find(who);
    return m != NULL ? m->strikes : 0;
}

void TempMemberBook::RestoreExpiry(const CharKey& who, int expiryHour)
{
    const int expiry = SanitiseSavedTempValue(expiryHour);
    std::map<CharKey, TempMembership>::iterator it = records_.find(who);
    if (it != records_.end())
    {
        it->second.expiryHour = expiry;
        return;
    }
    if (expiry > 0)
        records_[who] = NewTempMembership(expiry);
}

void TempMemberBook::RestoreStrikes(const CharKey& who, int strikes)
{
    const int count = SanitiseSavedTempValue(strikes);
    std::map<CharKey, TempMembership>::iterator it = records_.find(who);
    if (it != records_.end())
    {
        it->second.strikes = count;
        return;
    }
    // Before the expiry (a save may list them in any order): kept, no member yet.
    if (count > 0)
    {
        TempMembership pending;
        pending.strikes = count;
        records_[who] = pending;
    }
}

void TempMemberBook::Clear()
{
    records_.clear();
}
} // namespace totalrecruit
