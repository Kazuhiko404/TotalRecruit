#pragma once

#include <string>

class Character;
class RootObject;

namespace totalrecruit
{
// Resolves the character behind a context-menu target, or null when the target
// is not a character (or has already been freed).
Character* CharacterFromTarget(RootObject* target);

// True when the configured exclusions allow recruiting this character.
// outReason receives a short explanation when the answer is false.
bool IsRecruitable(Character* character, std::string* outReason);

// Moves the character into the player's faction and current squad.
bool Recruit(Character* character, std::string* outMessage);

// The full path behind the overlay button: evaluates the rules, charges
// whatever the NPC wants, and either offers the FCS dialogue or recruits
// directly. Reports the outcome to the player either way -- a refusal always
// comes with the reason for it.
void AttemptRecruit(Character* character);

// Shows a line in Kenshi's own message roller.
void TellPlayer(const std::string& message, bool isProblem);

// Takes the agreed cats out of the player's purse, reporting either way.
// Returns false when the money was not there, in which case no recruit should
// follow. A price of zero always succeeds.
bool SettlePrice(Character* character, int cats, const std::string& name);

// Gives back what SettlePrice took, for a purchase that then failed (the engine
// refused the recruit). Without it the player paid and got nobody.
void RefundPrice(Character* character, int cats, const std::string& name);

// Called once the player has answered the offer window. On acceptance the price
// is paid and the recruit goes through; on refusal the NPC says so and nothing
// changes. Money only ever moves here, so walking away costs nothing.
void CompleteOffer(Character* character, bool accepted);

// Acts on what a captive is short of rather than answering their refusal --
// feeding them, for now. Reopens the offer afterwards so the player sees what
// changed without working through the context menu again.
void HelpCaptive(Character* character);

// Display name, or "<unnamed>" when it cannot be read.
std::string CharacterName(Character* character);
} // namespace totalrecruit
