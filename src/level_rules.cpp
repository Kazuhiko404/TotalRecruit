#include "level_rules.h"

namespace totalrecruit
{
int RoundLevel(float level)
{
    return static_cast<int>(level >= 0.0f ? level + 0.5f : level - 0.5f);
}
} // namespace totalrecruit
