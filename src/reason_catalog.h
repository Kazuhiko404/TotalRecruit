#pragma once

// Every reason the player can be told, without the engine: which messages.json
// key holds its text, what the built-in text is, and what tone an NPC takes when
// the reason is a refusal.
//
// The tone is the part that used to be a guess. SpeakOpeningLine picked the line a
// refusing NPC speaks by searching the reason's finished Spanish for words
// ("honor", "lealtad", "mataste"...), so rewording a message in messages.json (or
// translating it) quietly changed what the NPC said. The rules pick a ReasonKey
// instead, and the tone follows from the key.

#include <string>

#include "verdict.h"

namespace totalrecruit
{
// How a refusing NPC sounds. Not a reason in itself: several reasons share one.
enum RefusalTone
{
    // "Who are you? I have nothing to say to you."
    RefusalTone_Default,
    // Pestered: "You are wearing me out."
    RefusalTone_Insistence,
    // Too weak to give orders.
    RefusalTone_TooWeak,
    // Honour is not for sale.
    RefusalTone_Honour,
    // Loyal to someone else, and the player is nobody to them.
    RefusalTone_Loyalty,
    // The one refusal that really is about something the player did.
    RefusalTone_Grudge
};

// The messages.json path of this reason, with "{band}" replaced by the NPC's
// faction band ("aliado", "hostil", ...) for the reasons that vary by standing.
// Empty for Reason_None.
std::string ReasonMessageKey(ReasonKey key, const std::string& band);

// The built-in line, in Spanish (UTF-8), with {cats} and {purse} left in for
// Fill. Empty for Reason_None.
const char* ReasonDefaultText(ReasonKey key);

// The tone an NPC takes when this is why they said no.
RefusalTone RefusalToneFor(ReasonKey key);

// The table itself, for the tests: it has one row per ReasonKey, in enum order.
int ReasonCatalogSize();
ReasonKey ReasonCatalogKeyAt(int index);
} // namespace totalrecruit
