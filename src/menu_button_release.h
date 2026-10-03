#pragma once

// Port of TotalLoot/src/click_gesture (namespace renamed): the pure rule for one
// context-menu click gesture. No Kenshi / MyGUI includes so it is unit-tested.
//
// Kenshi's own menu acts on RELEASE: hold the right button, slide onto an
// entry, let go. A button bound only to "pressed" never sees that gesture, so
// the Hablar button listens to pressed AND released, like the TotalSparring and
// TotalLoot buttons. An ordinary click delivers both events back to back, which
// must act exactly once: the event that acts consumes the gesture and the second
// one is an echo.

namespace totalrecruit
{
namespace clickgesture
{
enum Verdict
{
    ACT,    // the gesture is armed: do the work
    ECHO,   // the paired event of a gesture that already acted: ignore
    STRAY   // nothing armed and nothing acted: ignore (worth a log line)
};

struct State
{
    bool armed;         // a menu with the Hablar button is open and unspent
    bool echoPending;   // the gesture acted; its paired event may still arrive

    State() : armed(false), echoPending(false) {}
};

// A menu with the Hablar button just opened: a fresh gesture can act.
void Arm(State* state);

// One mouse event reached the button.
Verdict Classify(State* state);
} // namespace clickgesture
} // namespace totalrecruit
