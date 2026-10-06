#include "duel_challenge_rules.h"

#include <cmath>
#include <cstdio>

#include <windows.h>

namespace totalrecruit
{
namespace
{
// An animal (animal-duel-submission D1): no ally tier and no 80+ cut, in either
// preset; it has to be allowed, free and at the animal level.
bool AnimalMayDuel(const DuelOfferFacts& facts)
{
    return facts.animalAllowed && !facts.animalOwned && facts.npcLevel >= facts.animalFromLevel;
}

// A person: in Cheat, anyone at Cheat's own cut (duel-any-source U1: 1, every level);
// otherwise a Balanced tiered ally at the elite cut level.
bool PersonMayDuel(const DuelOfferFacts& facts)
{
    if (facts.cheatPreset)
    {
        return facts.npcLevel >= facts.cheatPeopleFromLevel;
    }
    return facts.allyTiered && facts.npcLevel >= facts.eliteFromLevel;
}
} // namespace

bool CanOfferDuel(const DuelOfferFacts& facts)
{
    if (!facts.enabled || !facts.tsAvailable || facts.onCooldown)
    {
        return false;
    }
    return facts.isAnimal ? AnimalMayDuel(facts) : PersonMayDuel(facts);
}

int DuelCooldownUntilHour(double nowHours, float cooldownHours)
{
    const double hours = cooldownHours > 0.0f ? (double)cooldownHours : 0.0;
    return (int)std::ceil(nowHours + hours);
}

bool DuelCooldownActive(int untilHour, double nowHours)
{
    return untilHour > 0 && nowHours < (double)untilHour;
}

bool DuelPendingExpired(const PendingDuel& pending, double nowHours, float expiryHours)
{
    const double waited = nowHours - pending.sinceHour;
    return waited > (double)expiryHours;
}

std::string FormatDuelRecord(int wins, int losses, int draws)
{
    const int w = wins > 0 ? wins : 0;
    const int l = losses > 0 ? losses : 0;
    const int d = draws > 0 ? draws : 0;
    char buffer[48] = { 0 };
    if (d > 0)
    {
        _snprintf_s(buffer, sizeof(buffer), _TRUNCATE, "%d-%d-%d", w, l, d);
    }
    else
    {
        _snprintf_s(buffer, sizeof(buffer), _TRUNCATE, "%d-%d", w, l);
    }
    return std::string(buffer);
}

ExternalBoutReason JudgeExternalBout(const DuelBoutRecord& bout, const ExternalBoutFacts& facts)
{
    if (!bout.isDuel)
    {
        return ExternalBout_NotDuel;
    }
    if (facts.hasPending)
    {
        return ExternalBout_Pending;
    }
    if (!facts.countExternal)
    {
        return ExternalBout_SwitchOff;
    }
    if (bout.outcome == DuelBout_Undecided)
    {
        return ExternalBout_Undecided;
    }
    if (!bout.aKnown || !bout.aIsSquadMember || !bout.bKnown)
    {
        return ExternalBout_SideUnknown;
    }
    if (bout.bIsSquadMember)
    {
        return ExternalBout_SquadPair;
    }
    if (!facts.offerChecked)
    {
        return ExternalBout_CheckOffer;
    }
    return facts.offered ? ExternalBout_Counts : ExternalBout_NotOffered;
}

const char* ExternalBoutReasonWord(ExternalBoutReason reason)
{
    switch (reason)
    {
    case ExternalBout_Counts:      return "counts";
    case ExternalBout_CheckOffer:  return "check offer";
    case ExternalBout_NotDuel:     return "not a duel";
    case ExternalBout_Pending:     return "pending challenge";
    case ExternalBout_SwitchOff:   return "switch off";
    case ExternalBout_Undecided:   return "undecided";
    case ExternalBout_SideUnknown: return "side unknown";
    case ExternalBout_SquadPair:   return "squad pair";
    case ExternalBout_NotOffered:  return "not offered";
    default:                       return "?";
    }
}

// ------------------------------------------------------------ DuelChallengeBook
void DuelChallengeBook::Challenge(const CharKey& npc, const CharKey& challenger, double nowHours)
{
    PendingDuel pending;
    pending.challenger = challenger;
    pending.sinceHour = nowHours;
    pending_[npc] = pending;
}

bool DuelChallengeBook::Pending(const CharKey& npc, PendingDuel* out) const
{
    const std::map<CharKey, PendingDuel>::const_iterator it = pending_.find(npc);
    if (it == pending_.end())
    {
        return false;
    }
    if (out != NULL)
    {
        *out = it->second;
    }
    return true;
}

void DuelChallengeBook::Withdraw(const CharKey& npc)
{
    pending_.erase(npc);
}

int DuelChallengeBook::ExpirePending(double nowHours, float expiryHours, std::vector<CharKey>* expired)
{
    int count = 0;
    std::map<CharKey, PendingDuel>::iterator it = pending_.begin();
    while (it != pending_.end())
    {
        if (DuelPendingExpired(it->second, nowHours, expiryHours))
        {
            if (expired != NULL)
            {
                expired->push_back(it->first);
            }
            pending_.erase(it++);
            ++count;
        }
        else
        {
            ++it;
        }
    }
    return count;
}

DuelResolveResult DuelChallengeBook::Resolve(const DuelBoutRecord& bout, double nowHours,
                                             const DuelChallenges& config, bool externalEligible)
{
    DuelResolveResult result;
    if (!bout.isDuel || !bout.bKnown)
    {
        return result;
    }
    const std::map<CharKey, PendingDuel>::iterator it = pending_.find(bout.b);
    if (it == pending_.end())
    {
        return externalEligible ? ResolveExternal(bout, nowHours, config) : result;
    }
    // A result that arrives after the challenge ran out is not an answer to it.
    if (DuelPendingExpired(it->second, nowHours, config.expiryHours))
    {
        pending_.erase(it);
        return result;
    }
    // Undecided (cancelled, interrupted, carried off): the challenge still stands.
    if (bout.outcome == DuelBout_Undecided || !bout.aKnown || !bout.aIsSquadMember)
    {
        return result;
    }

    result.npc = bout.b;
    result.challenger = it->second.challenger;
    result.winner = bout.a;
    pending_.erase(it);
    if (bout.outcome == DuelBout_AWon)
    {
        result.resolution = DuelResolution_Won;
        return result;
    }
    result.resolution = DuelResolution_Lost;
    result.cooldownUntilHour = DuelCooldownUntilHour(nowHours, config.cooldownHours);
    SetCooldownUntil(bout.b, result.cooldownUntilHour);
    return result;
}

// A TotalSparring duel nobody challenged through TotalRecruit that the engine found
// eligible: the same Won / Lost as a challenge, side A as the challenger. The shape is
// checked again, so an eligible flag never makes a squad pair or an undecided duel count.
DuelResolveResult DuelChallengeBook::ResolveExternal(const DuelBoutRecord& bout, double nowHours,
                                                     const DuelChallenges& config)
{
    DuelResolveResult result;
    ExternalBoutFacts facts;
    facts.countExternal = true;
    facts.offerChecked = true;
    facts.offered = true;
    if (JudgeExternalBout(bout, facts) != ExternalBout_Counts)
    {
        return result;
    }
    result.external = true;
    result.npc = bout.b;
    result.challenger = bout.a;
    result.winner = bout.a;
    if (bout.outcome == DuelBout_AWon)
    {
        result.resolution = DuelResolution_Won;
        return result;
    }
    result.resolution = DuelResolution_Lost;
    result.cooldownUntilHour = DuelCooldownUntilHour(nowHours, config.cooldownHours);
    SetCooldownUntil(bout.b, result.cooldownUntilHour);
    return result;
}

bool DuelChallengeBook::OnCooldown(const CharKey& npc, double nowHours) const
{
    return DuelCooldownActive(CooldownUntil(npc), nowHours);
}

int DuelChallengeBook::CooldownUntil(const CharKey& npc) const
{
    const std::map<CharKey, int>::const_iterator it = cooldownUntil_.find(npc);
    return it == cooldownUntil_.end() ? 0 : it->second;
}

void DuelChallengeBook::SetCooldownUntil(const CharKey& npc, int untilHour)
{
    if (untilHour <= 0)
    {
        cooldownUntil_.erase(npc);
        return;
    }
    cooldownUntil_[npc] = untilHour;
}

void DuelChallengeBook::Clear()
{
    pending_.clear();
    cooldownUntil_.clear();
}

// ------------------------------------------------- The process-wide instance
namespace
{
struct BookMutex
{
    CRITICAL_SECTION value;
    BookMutex() { InitializeCriticalSection(&value); }
    ~BookMutex() { DeleteCriticalSection(&value); }
} g_mutex;

// RAII: the lock is held for the scope and released on every path out of it.
struct Lock
{
    Lock() { EnterCriticalSection(&g_mutex.value); }
    ~Lock() { LeaveCriticalSection(&g_mutex.value); }

private:
    Lock(const Lock&);
    Lock& operator=(const Lock&);
};

DuelChallengeBook g_book;
} // namespace

void RecordDuelChallenge(const CharKey& npc, const CharKey& challenger, double nowHours)
{
    Lock lock;
    g_book.Challenge(npc, challenger, nowHours);
}

void WithdrawDuelChallenge(const CharKey& npc)
{
    Lock lock;
    g_book.Withdraw(npc);
}

int ExpireDuelChallenges(double nowHours, float expiryHours)
{
    Lock lock;
    return g_book.ExpirePending(nowHours, expiryHours, NULL);
}

DuelResolveResult ResolveDuelBout(const DuelBoutRecord& bout, double nowHours,
                                  const DuelChallenges& config, bool externalEligible)
{
    Lock lock;
    return g_book.Resolve(bout, nowHours, config, externalEligible);
}

bool DuelChallengePending(const CharKey& npc)
{
    Lock lock;
    return g_book.Pending(npc, NULL);
}

bool DuelOnCooldown(const CharKey& npc, double nowHours)
{
    Lock lock;
    return g_book.OnCooldown(npc, nowHours);
}

void SetDuelCooldownUntil(const CharKey& npc, int untilHour)
{
    Lock lock;
    g_book.SetCooldownUntil(npc, untilHour);
}

int SavedDuelCooldownUntil(const std::string& persistedId)
{
    if (persistedId.empty())
    {
        return 0;
    }
    Lock lock;
    return g_book.CooldownUntil(CharKey::FromId(persistedId));
}

void RestoreSavedDuelCooldownUntil(const std::string& persistedId, int untilHour)
{
    if (persistedId.empty())
    {
        return;
    }
    Lock lock;
    g_book.SetCooldownUntil(CharKey::FromId(persistedId), untilHour);
}

void ForgetAllDuelChallenges()
{
    Lock lock;
    g_book.Clear();
}
} // namespace totalrecruit
