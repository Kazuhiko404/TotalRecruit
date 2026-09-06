#pragma once

#include <string>

class Character;

namespace totalrecruit
{
// What an NPC wants before they will join. Every NPC has a currency; none has a
// closed door. "Not yet, and here is why" is a state this mod can express --
// that is the whole point of doing it in code instead of dialogue data.
enum Currency
{
    Currency_None,        // nothing owed; they will come
    Currency_Cats,        // bandits, traders: pay them
    Currency_Care,        // the starving and the wounded: feed and heal them
    Currency_Freedom,     // slaves: let them go
    Currency_Submission,  // soldiers, the proud: beat them, then wait
    Currency_Refused      // the one honest no
};

// The result of asking "will this NPC join, and if not, what is missing?".
struct Verdict
{
    bool willJoin;
    Currency currency;
    // Cats demanded, when currency is Currency_Cats.
    int cats;
    // Player-facing explanation. Always populated -- telling the player WHY is
    // the feature, not a debug aid.
    std::string reason;
    // Which route recognised this NPC, for logging and for choosing the
    // dialogue record to fire.
    bool viaRescue;
    bool viaCaptivity;

    Verdict()
        : willJoin(false),
          currency(Currency_None),
          cats(0),
          reason(),
          viaRescue(false),
          viaCaptivity(false)
    {
    }
};

// Evaluates an NPC against the active preset. Stateless: everything is read
// from the world at the moment of the call, so there is nothing to persist,
// nothing to desynchronise on load, and nothing to save-scum.
Verdict Evaluate(Character* character);

// True when the player's faction owns the cage this character is locked in.
// There is no getCaptor(); this is derived from inSomething and inWhat.
bool IsInPlayerCage(Character* character);

// True when this character is dying rather than merely knocked out, and is not
// faking it (PS_PLAYING_DEAD exists).
bool IsGenuinelyDying(Character* character);

// Records that the player saved this character's life, so the debt survives
// even if the player walks away before they wake.
void MarkRescuedBy(Character* character, Character* saviour);

// True when the character remembers the player as their lifesaver.
bool RemembersPlayerAsSaviour(Character* character);

// Human-readable currency name, for logs and status lines.
const char* CurrencyName(Currency currency);

// What this NPC says about their own situation, in their own voice. Varies by
// archetype and personality, so a maniacal bandit and a fearful farmer do not
// open with the same line. Used to fill in a placeholder dialogue at runtime.
std::string SpeakOpeningLine(Character* character, const Verdict& verdict);

// The label for the "yes" reply, naming the price so the player sees what they
// are agreeing to.
std::string SpeakAcceptLine(const Verdict& verdict);

// What the offer window says about the situation, in the narrator's voice
// rather than the NPC's. The NPC already speaks in a bubble over their head, so
// repeating those words in the window wastes the only line it has -- this
// states what is happening and why instead.
std::string DescribeOffer(Character* character, const Verdict& verdict);

// A short line a captive calls out when they are ready to be recruited, so the
// player learns it without having to right-click on spec. Varies by personality
// -- a fearful prisoner pleads, a proud one concedes grudgingly -- and picks
// from several so the same prisoner does not repeat himself word for word.
std::string HailLine(Character* character);
} // namespace totalrecruit
