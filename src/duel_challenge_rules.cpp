#include "duel_challenge_rules.h"

#include <cmath>
#include <cstdio>

#include <windows.h>

namespace totalrecruit
{
bool CanOfferDuel(const DuelOfferFacts& facts)
{
    return facts.enabled && facts.tsAvailable && (facts.allyTiered || facts.cheatPreset) &&
           !facts.isAnimal &&
           facts.npcLevel >= facts.eliteFromLevel && !facts.onCooldown;
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
                                             const DuelChallenges& config)
{
    DuelResolveResult result;
    if (!bout.isDuel || !bout.bKnown)
    {
        return result;
    }
    const std::map<CharKey, PendingDuel>::iterator it = pending_.find(bout.b);
    if (it == pending_.end())
    {
        return result;
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
                                  const DuelChallenges& config)
{
    Lock lock;
    return g_book.Resolve(bout, nowHours, config);
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
