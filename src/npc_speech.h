#pragma once

#include <string>

#include "verdict.h"

class Character;

namespace totalrecruit
{
// What this NPC says about their own situation, in their own voice. Varies by
// archetype and personality, so a maniacal bandit and a fearful farmer do not open
// with the same line. A refusal takes its tone from the verdict's reason KEY
// (reason_catalog.h), never from the words of the finished reason. Used to fill in a
// placeholder dialogue at runtime.
std::string SpeakOpeningLine(Character* character, const Verdict& verdict);

// The label for the "yes" reply, naming the price so the player sees what they are
// agreeing to.
std::string SpeakAcceptLine(const Verdict& verdict);

// What the offer window says about the situation, in the narrator's voice rather
// than the NPC's. The NPC already speaks in a bubble over their head, so repeating
// those words in the window wastes the only line it has -- this states what is
// happening and why instead.
std::string DescribeOffer(Character* character, const Verdict& verdict);

// A short line a captive calls out when they are ready to be recruited, so the
// player learns it without having to right-click on spec. Varies by personality -- a
// fearful prisoner pleads, a proud one concedes grudgingly -- and picks from several
// so the same prisoner does not repeat himself word for word.
std::string HailLine(Character* character);
} // namespace totalrecruit
