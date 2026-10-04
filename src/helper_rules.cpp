#include "helper_rules.h"
#include "personality_groups.h"

namespace totalrecruit
{
bool HelperPersonalityAllowed(Temperament temperament)
{
    const PersonalityGroup group = GroupOf(temperament);
    return group == PersonalityGroup_Good || group == PersonalityGroup_Neutral;
}

HelperSkip ScreenHelperCandidate(const HelperCandidate& c, const Helpers& config,
                                 float friendlyRelation)
{
    if (!c.enabled || !config.enabled) return HelperSkip_Disabled;
    if (!c.balanced) return HelperSkip_NotBalanced;
    if (c.mine) return HelperSkip_Mine;
    if (c.animal) return HelperSkip_Animal;
    if (!c.up) return HelperSkip_Down;
    if (c.caged) return HelperSkip_Caged;
    if (c.busy) return HelperSkip_Busy;
    if (c.inCombat) return HelperSkip_InCombat;
    if (c.distanceMeters < 0.0f || c.distanceMeters > kHelperTriggerMeters) return HelperSkip_TooFar;
    if (!c.relationKnown || c.relation < friendlyRelation) return HelperSkip_NotAllied;
    if (!HelperPersonalityAllowed(c.temperament)) return HelperSkip_Personality;
    if (c.npcLevel <= c.leaderLevel + config.minLevelLead) return HelperSkip_LevelLead;
    if (c.onCooldown) return HelperSkip_Cooldown;
    if (c.rolledToday) return HelperSkip_RolledToday;
    return HelperSkip_None;
}

const char* HelperSkipText(HelperSkip skip)
{
    switch (skip)
    {
    case HelperSkip_None:        return "eligible";
    case HelperSkip_Disabled:    return "helpers off";
    case HelperSkip_NotBalanced: return "not Balanced";
    case HelperSkip_Mine:        return "one of the squad";
    case HelperSkip_Animal:      return "an animal";
    case HelperSkip_Down:        return "not up";
    case HelperSkip_Caged:       return "caged";
    case HelperSkip_Busy:        return "busy";
    case HelperSkip_InCombat:    return "fighting";
    case HelperSkip_TooFar:      return "too far";
    case HelperSkip_NotAllied:   return "not allied";
    case HelperSkip_Personality: return "greedy or cruel";
    case HelperSkip_LevelLead:   return "level lead too small";
    case HelperSkip_Cooldown:    return "on cooldown";
    case HelperSkip_RolledToday: return "already rolled today";
    }
    return "unknown";
}

bool HelperOfferRoll(unsigned int seed, float chancePerDay)
{
    if (chancePerDay <= 0.0f)
        return false;
    if (chancePerDay >= 1.0f)
        return true;
    const unsigned int threshold = (unsigned int)(chancePerDay * 10000.0f + 0.5f);
    return (seed % 10000u) < threshold;
}

int HelperDaysFromSeed(unsigned int seed, const Helpers& config)
{
    const int low = config.daysMin < 1 ? 1 : config.daysMin;
    const int high = config.daysMax < low ? low : config.daysMax;
    return low + (int)(seed % (unsigned int)(high - low + 1));
}

HelperContract NewHelperContract(int expiryHour)
{
    HelperContract c;
    c.expiryHour = expiryHour;
    return c;
}

HelperStep StepHelper(HelperContract* c, const HelperSignals& s, const Helpers& config)
{
    HelperStep step;
    if (s.dead)
        step.end = HelperEnd_Died;
    else if (s.joinedSquad)
        step.end = HelperEnd_Joined;
    else if (s.knockedOutBySquad)
        step.end = HelperEnd_KnockedOut;
    else if (s.attackedBySquad)
        step.end = HelperEnd_Attacked;
    else if (s.nowHours >= (double)c->expiryHour)
        step.end = HelperEnd_Expired;
    if (step.end != HelperEnd_None)
        return step;

    if (!c->expiryWarned && config.expiryWarningHours > 0.0f &&
        (double)c->expiryHour - s.nowHours <= (double)config.expiryWarningHours)
    {
        c->expiryWarned = true;
        step.expiryWarn = true;
    }
    return step;
}

const char* HelperEndText(HelperEnd end)
{
    switch (end)
    {
    case HelperEnd_None:       return "stays";
    case HelperEnd_Expired:    return "time up";
    case HelperEnd_Attacked:   return "attacked by the squad";
    case HelperEnd_KnockedOut: return "knocked out by the squad";
    case HelperEnd_Died:       return "died";
    case HelperEnd_Joined:     return "joined the squad";
    }
    return "unknown";
}

bool HelperEndIsBetrayal(HelperEnd end)
{
    return end == HelperEnd_Attacked || end == HelperEnd_KnockedOut;
}

const char* EscortRungName(EscortRung rung)
{
    switch (rung)
    {
    case EscortRung_Bodyguard:   return "BODYGUARD";
    case EscortRung_FollowOrder: return "FOLLOW_PLAYER_ORDER";
    case EscortRung_Walk:        return "walk (MOVE_CUS_ORDERED)";
    }
    return "unknown";
}

int NextFarChecks(int farChecks, float distanceMeters, bool inCombat)
{
    if (inCombat || distanceMeters < 0.0f)
        return farChecks;
    if (distanceMeters <= kHelperFarMeters)
        return 0;
    return farChecks + 1;
}

EscortRung NextEscortRung(EscortRung rung, int farChecks)
{
    if (farChecks < kHelperFarChecksToEscalate)
        return rung;
    switch (rung)
    {
    case EscortRung_Bodyguard:   return EscortRung_FollowOrder;
    case EscortRung_FollowOrder: return EscortRung_Walk;
    default:                     return EscortRung_Walk;
    }
}

bool ShouldAssist(const AssistFacts& f)
{
    return f.leaderInCombat && f.haveTarget && !f.targetIsSquad && !f.targetIsHelper &&
           !f.targetSameFaction && !f.targetDown;
}

// ------------------------------------------------------------- HelperBook
void HelperBook::Start(const CharKey& who, int expiryHour)
{
    contracts_[who] = NewHelperContract(expiryHour);
}

const HelperContract* HelperBook::Find(const CharKey& who) const
{
    std::map<CharKey, HelperContract>::const_iterator it = contracts_.find(who);
    return it != contracts_.end() ? &it->second : NULL;
}

void HelperBook::Update(const CharKey& who, const HelperContract& c)
{
    std::map<CharKey, HelperContract>::iterator it = contracts_.find(who);
    if (it != contracts_.end())
        it->second = c;
}

void HelperBook::End(const CharKey& who)
{
    contracts_.erase(who);
}

std::vector<CharKey> HelperBook::Keys() const
{
    std::vector<CharKey> out;
    for (std::map<CharKey, HelperContract>::const_iterator it = contracts_.begin();
         it != contracts_.end(); ++it)
        out.push_back(it->first);
    return out;
}

size_t HelperBook::Count() const
{
    return contracts_.size();
}

void HelperBook::SetRejectedUntil(const CharKey& who, int untilHour)
{
    if (untilHour > 0)
        rejectedUntil_[who] = untilHour;
    else
        rejectedUntil_.erase(who);
}

int HelperBook::RejectedUntil(const CharKey& who) const
{
    std::map<CharKey, int>::const_iterator it = rejectedUntil_.find(who);
    return it != rejectedUntil_.end() ? it->second : 0;
}

bool HelperBook::OnRejectCooldown(const CharKey& who, double nowHours) const
{
    const int until = RejectedUntil(who);
    return until > 0 && nowHours < (double)until;
}

bool HelperBook::RolledOn(const CharKey& who, int day) const
{
    std::map<CharKey, int>::const_iterator it = rolledDay_.find(who);
    return it != rolledDay_.end() && it->second == day;
}

void HelperBook::NoteRolled(const CharKey& who, int day)
{
    rolledDay_[who] = day;
}

int HelperBook::SavedExpiry(const CharKey& who) const
{
    const HelperContract* c = Find(who);
    return c != NULL ? c->expiryHour : 0;
}

void HelperBook::RestoreExpiry(const CharKey& who, int expiryHour)
{
    if (expiryHour > 0)
        Start(who, expiryHour);
}

int HelperBook::SavedRejectedUntil(const CharKey& who) const
{
    return RejectedUntil(who);
}

void HelperBook::RestoreRejectedUntil(const CharKey& who, int untilHour)
{
    if (untilHour > 0)
        rejectedUntil_[who] = untilHour;
}

void HelperBook::Clear()
{
    contracts_.clear();
    rejectedUntil_.clear();
    rolledDay_.clear();
}
} // namespace totalrecruit
