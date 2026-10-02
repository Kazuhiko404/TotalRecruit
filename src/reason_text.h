#pragma once

#include <string>

#include "verdict.h"

class Character;

namespace totalrecruit
{
// The finished line the player reads for a verdict's reason: the messages.json
// entry of its ReasonKey (one variant chosen stably for this character), or the
// built-in text when the file has none, with {name}, {cats} and {purse} filled in.
// Returns an empty string for a verdict that carries no reason.
std::string RenderReason(const Verdict& verdict, Character* who);
} // namespace totalrecruit
