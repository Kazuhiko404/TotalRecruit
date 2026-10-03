#include "menu_button_release.h"

namespace totalrecruit
{
namespace clickgesture
{
void Arm(State* state)
{
    if (!state->armed)
    {
        state->armed = true;
        state->echoPending = false;
    }
}

Verdict Classify(State* state)
{
    if (state->armed)
    {
        state->armed = false;
        state->echoPending = true;
        return ACT;
    }
    if (state->echoPending)
    {
        state->echoPending = false;
        return ECHO;
    }
    return STRAY;
}
} // namespace clickgesture
} // namespace totalrecruit
