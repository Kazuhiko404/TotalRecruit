#include "cheat_waits.h"

namespace totalrecruit
{
const float kCheatWaitHours = 1.0f / 60.0f;

float EffectiveWaitHours(int preset, float configuredHours)
{
    if (preset != Preset_Cheat)
    {
        return configuredHours;
    }
    if (configuredHours <= 0.0f)
    {
        return 0.0f;
    }
    return configuredHours < kCheatWaitHours ? configuredHours : kCheatWaitHours;
}

float CageWaitHours(const Config& config)
{
    return EffectiveWaitHours(config.preset, config.cheatHoursCaged);
}

float HailFirstHours(const Config& config)
{
    return EffectiveWaitHours(config.preset, config.hailFirstHours);
}

float HailRepeatHours(const Config& config)
{
    return EffectiveWaitHours(config.preset, config.hailRepeatHours);
}

float RejectCooldownHours(const Config& config)
{
    return EffectiveWaitHours(config.preset, config.submission.rejectCooldownHours);
}

float DuelLossCooldownHours(const Config& config)
{
    return EffectiveWaitHours(config.preset, config.duelChallenges.cooldownHours);
}

float BeatingCooldownHours(const Config& config)
{
    return EffectiveWaitHours(config.preset, config.submission.beatingCooldownHours);
}

float KnockoutCooldownHours(const Config& config)
{
    return EffectiveWaitHours(config.preset, config.submission.knockoutCooldownHours);
}
} // namespace totalrecruit
