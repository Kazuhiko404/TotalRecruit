#pragma once

// What the rules answer, without the engine.
//
// "Will this NPC join, and if not, what is missing?" is the whole question the
// mod asks. The answer used to carry its explanation as finished Spanish text
// only, so anything that needed to know WHY (the opening line an NPC speaks, for
// one) had to search that text for words -- and editing messages.json could
// quietly change what the NPC said. The reason now travels as a key as well as
// text: the rules pick the key, reason_text.cpp turns it into the player's line.

#include <string>

#include "chance_rules.h"

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

// Why a verdict came out the way it did. One value per distinct explanation the
// player can be told; reason_catalog.cpp holds the messages.json key and the
// built-in text of each. Add a value here, a row there, and a line in
// messages.json -- nothing else needs to learn about it.
enum ReasonKey
{
    Reason_None,

    // Before any route: the question could not be asked.
    Reason_Nobody,
    Reason_AnimalsDisabled,
    Reason_UnreadableTraits,
    Reason_Pretending,
    Reason_Dead,

    // Cheat preset.
    Reason_CheatJoinsFreely,
    Reason_CheatNeedsCage,
    Reason_CheatWaiting,
    Reason_CheatJoins,

    // Out cold, dying, saved.
    Reason_UnconsciousOwesYou,
    Reason_UnconsciousDying,
    Reason_UnconsciousWait,
    Reason_Dying,
    Reason_RescueJoins,
    Reason_RescueRepays,

    // Beaten.
    Reason_FieldYieldJoins,
    Reason_FieldYieldNotYet,
    Reason_Surrendered,
    Reason_GrudgeNever,

    // The free world.
    Reason_InOthersCage,
    Reason_Insistence,
    Reason_AllyJoins,
    Reason_AllyMaybe,
    Reason_FactionHonour,
    Reason_FactionGreedy,
    Reason_HonourStarving,
    Reason_HonourRefusesBribe,
    Reason_MilitaryLoyalty,

    // A price.
    Reason_CatsAsk,
    Reason_CatsJoin,

    // The cage.
    Reason_CaptivityDisabled,
    Reason_Intimidated,
    Reason_FreedFollows,
    Reason_CaredFor,
    Reason_Starving,
    Reason_Wounded,
    Reason_StarvingAndWounded,
    Reason_BrokenNoPride,
    Reason_UniqueZealot,
    Reason_Defiant,

    // Animals.
    Reason_AnimalOwned,
    Reason_TameWounded,
    Reason_TameReady,
    Reason_TameHungry,
    // Full or not, it never ate from the player's hand (AnimalExtra::fedByPlayer).
    Reason_TameNotFed,

    // The submission event: the player rejected them (RECHAZAR) and the reject
    // cooldown still runs. Checked once, before every route (submission_rules.h
    // DecideRejectCooldown).
    Reason_RejectCooldown,

    // Balanced allies by level (ally_tier_rules.h). A free or paid ally keeps the
    // reasons above (AllyJoins / AllyMaybe, CatsJoin / CatsAsk); these are the deals
    // and refusals only the tiers have.
    Reason_AllyTemporary,
    Reason_AllyLeaderTooWeak,
    Reason_AllyCruel,
    Reason_AllyGreedy,

    // A free animal beaten defeatsToBreak times in the open (animal-duel-submission
    // D3, route_animal_yield.cpp): the field-yield roll passed, or it did not and food
    // is still the other way in.
    Reason_AnimalYieldJoins,
    Reason_AnimalYieldNotYet,

    // Not a reason: the number of them, for tables indexed by one.
    Reason_Count
};

// The result of asking "will this NPC join, and if not, what is missing?".
struct Verdict
{
    bool willJoin;
    bool wantsFood;
    Currency currency;
    // Cats demanded, when currency is Currency_Cats. (The animal route reuses it
    // for "percent of the stomach still missing"; only Currency_Cats is money.)
    int cats;
    // What the player is carrying, when the reason is a price they cannot meet.
    int purse;
    // Why, as a key the rules can branch on and as the finished text the player
    // reads. Always populated -- telling the player WHY is the feature, not a
    // debug aid.
    ReasonKey reasonKey;
    std::string reason;
    // Which route recognised this NPC, for logging and for choosing the
    // dialogue record to fire.
    bool viaRescue;
    bool viaCaptivity;
    // Which faction-relation band this NPC sits in ("aliado", "hostil", ...).
    // Carried on the verdict so the spoken line and the reason cannot disagree
    // about how well the two of you get on.
    std::string band;
    // Animals reach every route a person does, but they do not speak. Carried
    // so the caller knows to draw a sound instead of a sentence.
    bool isAnimal;
    // They will not join, but they want to pay their debt with what they carry.
    // The NPC paying the player, never the reverse.
    bool owesGift;
    // The chance roll behind this verdict, when there was one: what went in and
    // what came out, so the log can say which term produced the answer.
    bool rolled;
    ChanceInputs chanceInputs;
    int chancePercent;
    // The taming bar behind an animal verdict, as 0..1 fractions of a full stomach:
    // how full it is and how full it has to be. Negative when the verdict is not
    // about a stomach, so a readout never has to guess.
    float tameFullness;
    float tameNeeded;
    // A Balanced ally whose level tier decided the deal (ally_tier_rules.h): the
    // popup and the card name the tier, the NPC's level and the friendship
    // discount (a whole percent, already in `cats`).
    bool allyTiered;
    int allyLevel;
    int allyDiscountPercent;
    // They join for a while only (80+ allies). The days were drawn when the verdict
    // was reached (0 when there was no key to draw them from); temp_member.h takes
    // it from there once they are recruited.
    bool temporary;
    int temporaryDays;

    Verdict()
        : willJoin(false),
          wantsFood(false),
          currency(Currency_None),
          cats(0),
          purse(0),
          reasonKey(Reason_None),
          reason(),
          viaRescue(false),
          viaCaptivity(false),
          isAnimal(false),
          owesGift(false),
          rolled(false),
          chanceInputs(),
          chancePercent(0),
          tameFullness(-1.0f),
          tameNeeded(-1.0f),
          allyTiered(false),
          allyLevel(0),
          allyDiscountPercent(0),
          temporary(false),
          temporaryDays(0)
    {
    }
};
} // namespace totalrecruit
