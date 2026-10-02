#include "common.h"
#include "insistence.h"
#include "engine_reads.h"
#include "char_key_reader.h"
#include "config.h"

#include <map>

namespace totalrecruit
{
namespace
{
struct TalkState
{
    int attempts;
    double lastHour;
    double ignoredUntilHour;

    TalkState() : attempts(0), lastHour(0.0), ignoredUntilHour(0.0) {}
};

std::map<CharKey, TalkState> g_talkStates;
} // namespace

void ResetTalkStates()
{
    g_talkStates.clear();
}

bool IsIgnoringPlayer(Character* character, double* outRemainingHours)
{
    if (character == NULL)
    {
        return false;
    }

    double now = 0.0;
    if (!SafeGameHours(&now))
    {
        return false;
    }

    CharKey key;
    if (!SafeFindCharKey(character, &key))
    {
        return false;
    }

    std::map<CharKey, TalkState>::iterator it = g_talkStates.find(key);
    if (it == g_talkStates.end())
    {
        return false;
    }

    if (it->second.ignoredUntilHour > now)
    {
        if (outRemainingHours != NULL)
        {
            *outRemainingHours = it->second.ignoredUntilHour - now;
        }
        return true;
    }

    return false;
}

bool RecordInteraction(Character* character, double nowHours)
{
    if (character == NULL)
    {
        return false;
    }

    CharKey key;
    if (!SafeCharKey(character, &key))
    {
        return false;
    }

    TalkState& state = g_talkStates[key];
    if (state.ignoredUntilHour > nowHours)
    {
        return true;
    }

    const Config& config = GetConfig();

    // Long enough between attempts and it is a new conversation, not the same one
    // continued.
    if (nowHours - state.lastHour > config.insistence.forgetAfterHours)
    {
        state.attempts = 0;
    }

    state.lastHour = nowHours;
    state.attempts++;

    if (state.attempts >= config.insistence.attemptsBeforeIgnored)
    {
        // R2.4 -- the punishment accumulates. Asking five times running is not the
        // same as asking three, and a flat penalty made pestering free after the
        // first one landed.
        const int over = state.attempts - config.insistence.attemptsBeforeIgnored + 1;
        double hours = config.insistence.baseHours * over;
        if (hours > config.insistence.maxHours)
        {
            hours = config.insistence.maxHours;
        }
        state.ignoredUntilHour = nowHours + hours;
        return true;
    }

    return false;
}
} // namespace totalrecruit
