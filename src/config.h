#pragma once

#include <string>
#include <vector>

#include "temperament.h"

namespace totalrecruit
{
// Exclusions are opt-in: everything defaults to false so a fresh install has no
// restrictions at all.
struct Exclusions
{
    bool dead;
    bool unconscious;
    bool animals;
    bool sameFaction;
    std::vector<std::string> factions;
    std::vector<std::string> characterNames;

    Exclusions()
        : dead(false), unconscious(false), animals(false), sameFaction(true)
    {
    }
};

// How strict the rule engine is. The engine is the same in both; only the
// numbers and which factors are consulted change. v1 ships these two; a config
// value outside them (the retired Vanilla+ was 2) loads as Balanced
// (preset_rules.h).
enum Preset
{
    // Everything recruitable, click and done. The original behaviour.
    Preset_Cheat,
    // Each archetype charges in its own currency: cats, food, freeing, submission.
    Preset_Balanced
};

// Prices, expressed as what the NPC wants before they will join. A price that
// cannot be paid is never a refusal, only a "not yet" with a reason attached.
struct Prices
{
    // Route B: cats demanded, scaled from the NPC's rough level.
    // Price = (catsPerLevel/10) * level^2 + catsCeiling * logistic(level).
    // The square term keeps low levels affordable; the logistic carries the top
    // and flattens instead of running away. Calibrated in price_rules.cpp to
    // vanilla-like prices (a level-13 named, talented NPC ~2500 cats, the
    // strongest ordinary ~27k); the quote is that curve times the capped premium
    // (talent x uniqueness x personality, at most premiumMaxMultiplier) and
    // faction, rounded to 10 and held between catsMinimum and catsHardCap
    // (FinalPriceInCats).
    int catsPerLevel;
    // What the strongest ordinary characters approach. Not a hard cap: talent,
    // faction, personality and uniqueness still multiply on top, until catsHardCap.
    int catsCeiling;
    // How sharply the price accelerates around the midpoint.
    float priceCurveSteepness;
    // The level at which the curve is steepest -- roughly "a serious fighter".
    float priceCurveMidpoint;
    int catsMinimum;
    // Optional extra trigger for "starving", as a 0..1 fraction of a full
    // stomach: 0.4 means "starving below 40% full". The engine's own
    // isReallyHungry and isHungerKO already decide this, so 0 (off) is the sane
    // default. Note the raw engine field counts fullness and runs 0..3, so it
    // is normalised before comparison -- never compare against it directly.
    float hungerThreshold;
    // Overall health at or below this counts as "hurt enough to listen".
    float woundedThreshold;
    // Multipliers applied to the cats price by personality.
    float fearfulMultiplier;
    float braveMultiplier;
    // How hard an unusual talent moves the price. 0 disables it and prices by
    // level alone. A stat far above the character's own level is the only thing
    // that reads as "they know something you cannot see from here".
    float talentWeight;
    // What a named character multiplies their price by. They are not merely
    // strong -- they are the only one there is, and there is no second attempt
    // if you decide later that you wanted them.
    float uniqueMultiplier;
    // Hard ceiling on the final price after every multiplier. Multipliers stack
    // (talent x unique x personality x faction), so without one a single
    // character could be quoted millions. 0 removes the ceiling (the price is
    // still kept inside the int range). A safety net for hand-edited configs: the
    // premium cap below already keeps shipped quotes far under it.
    int catsHardCap;
    // The most the talent multiplier may add. 1.5 means a specialist costs at most
    // half again what their level alone says.
    float talentMaxMultiplier;
    // The most talent x uniqueness x personality may multiply the base curve by,
    // together. Capped from above only: a coward's discount still applies. The
    // faction multiplier is separate and applies after. 1.6 keeps a named,
    // talented NPC at 1.6x their level price instead of 1.5 x 2.0 x 1.6 = 4.8x.
    float premiumMaxMultiplier;
    // How rare each family of skill is, as the weight the talent edge is scaled
    // by. Combat is the baseline (1.0): Kenshi hands you fighters constantly. A
    // craftsman is worth more than a swordsman of the same level because
    // engineers almost never turn up, a shadow specialist somewhere in between.
    float craftFamilyWeight;
    float shadowFamilyWeight;

    // Haggling over a cats price (haggle_rules.h). Off, the quote is take it or
    // leave it, as before.
    bool haggleEnabled;
    // The lowest offer the slider allows, as a fraction of the asking price
    // (rounded up to 10 cats).
    float haggleMinFraction;
    // How many points of acceptance each percent of discount costs: at 2.0 a 20%
    // lowball starts at 60%.
    float haggleDiscountWeight;
    // How much each point of the speaker's best fighting stat over the NPC's
    // level adds to the acceptance chance (held within +-20).
    float haggleLeverageWeight;
    // What a refused lowball adds to the asking price for the rest of that
    // in-game day, as a fraction: 0.2 is +20%.
    float haggleOffenceMarkup;

    Prices()
        : catsPerLevel(32),
          catsCeiling(10000),
          priceCurveSteepness(0.09f),
          priceCurveMidpoint(31.0f),
          catsMinimum(200),
          hungerThreshold(0.0f),
          woundedThreshold(0.5f),
          fearfulMultiplier(0.6f),
          braveMultiplier(1.6f),
          talentWeight(1.0f),
          uniqueMultiplier(2.0f),
          catsHardCap(150000),
          talentMaxMultiplier(1.5f),
          premiumMaxMultiplier(1.6f),
          craftFamilyWeight(1.6f),
          shadowFamilyWeight(1.3f),
          haggleEnabled(true),
          haggleMinFraction(0.5f),
          haggleDiscountWeight(2.0f),
          haggleLeverageWeight(0.5f),
          haggleOffenceMarkup(0.2f)
    {
    }
};

// The shared chance engine. Level, relation and personality each contribute a
// number here, and no single one of them can decide the answer alone -- which
// is the point: a refusal the player cannot act on is not a rule, it is a wall.
struct Chance
{
    // What an evenly-matched pair starts from, before anything else is said.
    // Kept well under the ceiling on purpose: start it at 100 and every other
    // term lands on a full number and disappears into the clamp.
    int levelBase;
    // Chance lost per level the NPC has over the player.
    float levelGapPenalty;
    // How low the level term can push it. Above zero on purpose: a hopeless
    // gap should be unlikely, not impossible, so relation and personality
    // still have something to work with.
    int levelGapFloor;
    // Chance gained per level the player has over the NPC.
    float levelEdgeBonus;
    // Cap on that bonus, so out-levelling someone is an argument and not an
    // automatic yes.
    int levelEdgeCap;
    // Final clamps on the whole calculation. Never 0 and never 100: certainty
    // in either direction is what turns a negotiation back into a gate.
    int floor;
    int ceiling;
    // What being locked in the player's cage is worth on its own. This is the
    // mod's central mechanic, so it argues loudly: an ordinary prisoner should
    // come round on the first or second try, the way Recruit Prisoners does it.
    int intimidationBonus;
    // Named characters are the one thing there is no second chance at, so they
    // cost roughly twice the work of an ordinary prisoner -- without ever
    // becoming impossible.
    int uniquePenalty;
    // Killing one of theirs. For temperaments that can forgive it at all this
    // is a weight -- the heaviest in the engine -- rather than a closed door.
    int grudgePenalty;
    // Whether the honourable, the brave and the kind refuse outright instead.
    // Loyalty is what defines them; buying it off with cats would cost them the
    // only thing the mod says about who they are.
    bool grudgeIsAbsolute;
    // How much faction standing moves the chance: this fraction of the relation
    // (-100..100), so 0.25 makes a full ally worth +25 points and a sworn enemy -25.
    float relationWeight;
    // What being on the same side is worth on its own, for those at or above
    // friendlyRelation: standing this good is itself an argument.
    int alliedBonus;
    // Temperament as points of chance, indexed by Temperament. Not who is easiest
    // to bully, who is easiest to convince: the frightened want out of wherever
    // this is, the honourable are convinced by conduct and not by odds.
    int personality[Temperament_Count];

    Chance()
        : levelBase(45),
          levelGapPenalty(1.5f),
          levelGapFloor(2),
          levelEdgeBonus(1.5f),
          levelEdgeCap(40),
          floor(5),
          ceiling(95),
          intimidationBonus(15),
          uniquePenalty(25),
          grudgePenalty(20),
          grudgeIsAbsolute(true),
          relationWeight(0.25f),
          alliedBonus(10)
    {
        for (int i = 0; i < Temperament_Count; ++i)
        {
            personality[i] = 0;
        }
        personality[Temperament_Fearful]    =  20;   // wants out of wherever this is
        personality[Temperament_Traitorous] =  15;   // loyalty was never the point
        personality[Temperament_Dumb]       =  10;
        personality[Temperament_Smart]      =  -5;   // sees the offer for what it is
        personality[Temperament_Brave]      = -15;
        personality[Temperament_Honorable]  = -20;   // convinced by conduct, not odds
        personality[Temperament_Maniacal]   = -25;   // convinced by nothing
    }
};

// Submission: being beaten, and how much of a beating it takes. Everything here
// is a number the player can move, because how far a mod should let you push a
// prisoner is a taste question, not a design one.
struct Submission
{
    // Count knockouts the player inflicted (not those Golpear / Noquear cause in
    // the player's cage: those are beating stress). Off makes submission depend on
    // present condition alone, as it did before.
    // One knockout counts once (defeat_episode_rules) and the count is saved in
    // the character's own save state (defeat_ledger), so neither a debounce nor
    // a reload floor is configured any more.
    bool countDefeats;
    // Chance added per recorded defeat.
    int chancePerDefeat;
    // Ceiling on that, so an endless beating is not an endless argument.
    int maxDefeatChance;
    // R3.1: how close to death counts as begging distance, as a 0..1 fraction
    // of blood remaining. getOverallHealthRating cannot be used -- it reads 0
    // for anyone unconscious, however intact -- so blood over max blood is the
    // honest measure, and it keeps working while they are down.
    bool begWhenNearDeath;
    float nearDeathBlood;
    // R3.5: how much the temperament moves the number of beatings needed. A
    // coward yields early; the brave and the honourable make you work.
    float fearfulResolve;
    float braveResolve;
    float honourableResolve;
    // R2.3 -- roughing up a prisoner in your own cage (allowBeating,
    // beatingCooldownHours). Golpear is one direct, capped blow from the
    // selected character (cage_commands.h): it may knock the prisoner out, never
    // kills, and how long they stay down is the engine's. beatingCooldownHours is
    // the interval between beatings: after a blow that landed, Golpear rests on
    // that prisoner for that many game hours; knockoutCooldownHours is the same
    // for Noquear, counted from a knockout order that put the prisoner down. Both
    // 1 h by default (the context menu's buttons and the popup's GOLPEAR /
    // NOQUEAR show the wait). The old submissionBeatingKnockoutSeconds key is no
    // longer read.
    // Route C -- yielding in the field, with no cage involved. How many times
    // the player has to beat the same person before they will consider giving
    // in. This is the route the spec asks for and it had become unreachable:
    // everything that read the defeat count lived behind the cage.
    int defeatsToBreak;
    // What a field yield argues on its own. Lower than the cage, because a cage
    // is a sustained condition and a beating in the open is a single moment.
    int fieldYieldBonus;
    // The Balanced cage's cared-for bonus (v1-presets-close P4): a person in the
    // player's cage who is unhurt, fed and at full health argues for joining with
    // a bonus drawn once per NPC per game day in [caredForBonusMin,
    // caredForBonusMax] (cared_for_rules.h). It used to be an automatic join
    // ("sano gracias a ti"); now it only weighs on the roll. A max below the min
    // means the min. Whole percent.
    int caredForBonusMin;
    int caredForBonusMax;
    // What loyalty to a standing organisation (military, law enforcement) takes
    // off the field-yield roll. A penalty, not a refusal: in the open world they
    // cannot be bought, but being beaten is the one argument they still hear.
    int fieldYieldLoyaltyPenalty;
    // The stress bar: how much each kind of hardship contributes, and how much
    // the whole bar can argue for. Derived from present state, so it falls
    // again when the player feeds or treats them -- keeping someone hungry is
    // a choice that has to be maintained, not a box that stays ticked. The
    // fourth term, the beating stress, is weighed by its own gains (below) and
    // fades on its own clock.
    float stressFromHunger;
    float stressFromWounds;
    float stressFromExhaustion;
    int maxStressChance;
    bool allowBeating;
    float beatingCooldownHours;
    float knockoutCooldownHours;
    // The beating stress (beating_stress_rules.h): the stress Golpear and Noquear
    // leave on a prisoner in the player's cage, where their knockouts no longer
    // count as defeats. One blow adds beatStressBase, one knockout that landed
    // knockoutStressBase, times the level factor (player / npc level, clamped to
    // beatingStressLevelMin..Max), divided by the temperament's resolve, clamped
    // to beatingStressMin..Max. The bar is 0..1 per prisoner, saved with the
    // character, a fourth stress term next to hunger, wounds and exhaustion, and a
    // full bar fades to nothing in beatingStressFadeHours game hours.
    float beatStressBase;
    float knockoutStressBase;
    float beatingStressMin;
    float beatingStressMax;
    float beatingStressLevelMin;
    float beatingStressLevelMax;
    float beatingStressFadeHours;
    // The submission event (submission_director.h): a beaten NPC that yields
    // walks up to the player and asks to join -- Aceptar / Perdonar / Rechazar.
    // `approachSeconds` is how long the walk-up may take (and how long a hostile
    // NPC is kept calm for it), in real seconds that stop while paused;
    // `forgiveSeconds` how long a forgiven (or unanswered) NPC stays calm;
    // `rejectCooldownHours` how many game hours a rejected NPC refuses every
    // recruit route (saved with the character).
    bool eventEnabled;
    float approachSeconds;
    float forgiveSeconds;
    float rejectCooldownHours;

    Submission()
        : countDefeats(true),
          chancePerDefeat(12),
          maxDefeatChance(45),
          begWhenNearDeath(true),
          nearDeathBlood(0.25f),
          fearfulResolve(0.6f),
          braveResolve(1.6f),
          honourableResolve(1.8f),
          defeatsToBreak(2),
          fieldYieldBonus(10),
          caredForBonusMin(5),
          caredForBonusMax(20),
          fieldYieldLoyaltyPenalty(25),
          stressFromHunger(0.5f),
          stressFromWounds(0.35f),
          stressFromExhaustion(0.15f),
          maxStressChance(45),
          allowBeating(true),
          beatingCooldownHours(1.0f),
          knockoutCooldownHours(1.0f),
          beatStressBase(0.08f),
          knockoutStressBase(0.15f),
          beatingStressMin(0.02f),
          beatingStressMax(0.25f),
          beatingStressLevelMin(0.5f),
          beatingStressLevelMax(1.5f),
          beatingStressFadeHours(24.0f),
          eventEnabled(true),
          approachSeconds(60.0f),
          forgiveSeconds(30.0f),
          rejectCooldownHours(72.0f)
    {
    }
};

// Insistence: what happens when the player keeps asking. Pestering should cost
// something, and the cost should grow -- a flat penalty makes the fourth ask
// free once the third has already landed.
struct Insistence
{
    int attemptsBeforeIgnored;
    // Hours per attempt over the limit, so five asks running cost five times
    // what one over costs.
    float baseHours;
    // Ceiling, so a determined player cannot lock an NPC away for a season.
    float maxHours;
    // Quiet for this long and it counts as a new conversation.
    float forgetAfterHours;

    Insistence()
        : attemptsBeforeIgnored(3),
          baseHours(4.0f),
          maxHours(48.0f),
          forgetAfterHours(2.0f)
    {
    }
};

// Taming, expressed as how full an animal's stomach has to be before it will
// follow the player. Every number is a 0..1 fraction of a full belly, so what
// the player types is what the bar has to reach.
struct Animals
{
    // Below this much of its lifespan an animal counts as a cub.
    float cubMaturity;
    // At or below this level an animal counts as weak, whatever its age.
    int weakLevel;
    // What a cub or a weakling needs. Small on purpose: it has no other option
    // and no reason to distrust you.
    float cubFullness;
    // The floor every grown animal starts from.
    float baseFullness;
    // How much being fully grown adds on top of the floor.
    float maturityWeight;
    // Above this level an animal is big enough to want more.
    int strongLevel;
    // How much that adds.
    float strongPenalty;
    // Discounts for out-levelling the animal, which it reads as evidence you
    // can keep feeding it.
    float edgeDiscount;     // 10+ levels above it
    float bigEdgeDiscount;  // 20+ levels above it
    // An animal owned by a real faction cannot be talked round with food -- it
    // has to be taken and caged. Off makes any animal tameable in the open.
    bool ownedNeedCage;
    // How much one meal fills the stomach, on the engine's own 0..3 scale.
    // eatItem does not move the value for animals, so the plugin credits it.
    // 0.40 is what CorpseFeeder grants for a whole fresh body, which makes a
    // piece of meat a generous but not silly mouthful.
    float nutritionPerMeal;
    // R5.4 -- whether temperament changes how much food it takes. Measured and
    // confirmed to vary between individuals of the same species, so it is on by
    // default; off falls back to maturity and level alone.
    bool personalityMatters;
    // How many levels above the animal the player has to be for edgeDiscount and
    // for bigEdgeDiscount.
    int edgeLevel;
    int bigEdgeLevel;
    // The temperament scales the bar by (temperamentBase + resolve * temperamentWeight),
    // where resolve is 1.0 for an ordinary animal: around 1.0 nothing moves, a
    // timid one settles for less, a stubborn one asks for more.
    float temperamentBase;
    float temperamentWeight;

    // Calibrated against measured wildlife, not guessed: a loose bonedog sits
    // at hunger 1.69 of 3.0, so it starts the negotiation around 56% full. Any
    // bar below that is one the animal has already cleared, and taming would
    // cost nothing -- which is exactly the bug these numbers exist to avoid.
    // A cub still bonds cheaply; everyone else has to be fed to well past where
    // they were found.
    Animals()
        : cubMaturity(0.25f),
          weakLevel(10),
          cubFullness(0.70f),
          baseFullness(0.80f),
          maturityWeight(0.15f),
          strongLevel(30),
          strongPenalty(0.05f),
          edgeDiscount(0.05f),
          bigEdgeDiscount(0.10f),
          ownedNeedCage(true),
          nutritionPerMeal(0.40f),
          personalityMatters(true),
          edgeLevel(10),
          bigEdgeLevel(20),
          temperamentBase(0.85f),
          temperamentWeight(0.15f)
    {
    }
};

// Balanced allies by level (ally_tier_rules.h): who joins free, who charges with a
// friendship discount, who comes along only for a few days, and who will not.
// Each cut level belongs to the tier it starts: 20 is already "20-50", 80 is
// already "80-100", 100 is already "100+". Only the Balanced preset reads these.
struct AllyTiers
{
    // Off, Balanced allies keep the single daily roll every preset used to have.
    bool enabled;
    // The levels where the next tier starts.
    int midFromLevel;      // 20: below it the low tier (Good joins free)
    int highFromLevel;     // 50
    int eliteFromLevel;    // 80: temporary members start here
    int legendFromLevel;   // 100
    // The most friendship can take off the price in each tier, as a 0..1 fraction.
    // The elite one is only for the greedy (everyone else at 80+ is temporary or
    // refuses).
    float lowMaxDiscount;
    float midMaxDiscount;
    float highMaxDiscount;
    float eliteMaxDiscount;
    // discount = min(tierMax, relation / 100 * discountScale).
    float discountScale;
    // A temporary member (80+) only follows a leader at most this many levels
    // below them.
    int closeLevelGap;
    // How many days a temporary member stays, drawn once per day from the NPC's
    // seed, inclusive at both ends.
    int tempMemberMinDays;
    int tempMemberMaxDays;

    AllyTiers()
        : enabled(true),
          midFromLevel(20),
          highFromLevel(50),
          eliteFromLevel(80),
          legendFromLevel(100),
          lowMaxDiscount(0.20f),
          midMaxDiscount(0.20f),
          highMaxDiscount(0.10f),
          eliteMaxDiscount(0.10f),
          discountScale(1.0f),
          closeLevelGap(15),
          tempMemberMinDays(3),
          tempMemberMaxDays(10)
    {
    }
};

// A temporary squad member once they joined (temp_member_rules.h): what counts as
// mistreatment, how many strikes they take before leaving, and the warnings they
// give. Hours are in-game hours. config.json keys "tempMember*".
struct TempMembers
{
    // Strikes that make them leave before their time is up.
    int maxStrikes;
    // Hungry: their stomach under this 0..1 fraction of full (or the engine says
    // they are starving) ...
    float hungerThreshold;
    // ... for at least this many hours in a row is one strike.
    float hungryHours;
    // Each kind of strike (hunger, attacked by the squad, knocked out by the
    // squad) counts at most once per this many hours.
    float strikeCooldownHours;
    // They say "me ire pronto" this many hours before their time is up.
    float expiryWarningHours;
    // After leaving, they want nothing to do with the player for this many days
    // (the reject cooldown, submission_tracker.h).
    int rejoinCooldownDays;
    // Once per game day they say over their head how many days they have left
    // ("Me quedan 4 dias"); not on the join day, not on the day of "me ire pronto".
    bool dailyNotice;

    TempMembers()
        : maxStrikes(3),
          hungerThreshold(0.25f),
          hungryHours(12.0f),
          strikeCooldownHours(12.0f),
          expiryWarningHours(12.0f),
          rejoinCooldownDays(7),
          dailyNotice(true)
    {
    }
};

// The free helper (helper_rules.h, helper.h): a much stronger Balanced ally of good
// or neutral character may walk up and offer to escort the player for a few days,
// for free, without taking orders. config.json keys "helper*".
struct Helpers
{
    bool enabled;
    // The chance (0..1) that an eligible ally offers, drawn once per in-game day each.
    float offerChancePerDay;
    // They offer only when their level is over the leader's by more than this.
    int minLevelLead;
    // How many in-game days the help lasts, inclusive at both ends.
    int daysMin;
    int daysMax;
    // After a RECHAZAR (or the squad turning on them) they do not offer again for
    // this many days (saved with the character).
    int rejectCooldownDays;
    // They say "me ire pronto" this many hours before their time is up.
    float expiryWarningHours;
    // Debug: a Windows virtual-key code (119 = F8). Pressed, the selected NPC offers
    // at once, past the roll, the level and the personality. 0 = off.
    int debugKey;
    // The escort is the engine's own Bodyguard contract (AI package 5090-gamedata.base,
    // the player squad leader as employer), like a hired mercenary. False, or when
    // the contract cannot be assigned: the code-driven escort (helper.h fallback).
    bool nativeBodyguard;

    Helpers()
        : enabled(true),
          offerChancePerDay(0.15f),
          minLevelLead(10),
          daysMin(2),
          daysMax(5),
          rejectCooldownDays(3),
          expiryWarningHours(12.0f),
          debugKey(0),
          nativeBodyguard(true)
    {
    }
};

// The duel challenge (duel_challenge_rules.h): an NPC of the elite cut level and
// above (Balanced: a tiered ally; Cheat: any person) can be challenged to a
// TotalSparring duel; a win is a join chance in Balanced (duel_join_rules.h) and a
// sure join in Cheat.
// Hours are in-game hours. config.json keys "duelChallenge*".
struct DuelChallenges
{
    // Off, the popup never offers RETAR A DUELO (TotalSparring present or not).
    bool enabled;
    // After a lost (or drawn) duel the NPC will not be challenged again for this
    // many hours (saved with the character).
    float cooldownHours;
    // A challenge whose duel never finished is forgotten after this many hours.
    float expiryHours;
    // Balanced: a won duel is a join chance (duel_join_rules.h), not a sure join:
    // min(100, joinBaseChance + joinChancePerWin * (wins - 1)) percent, wins counted
    // with the new one. Cheat ignores both: a win always joins.
    int joinBaseChance;
    int joinChancePerWin;
    // A win that does not join hides the duel for a seeded number of hours in
    // [winCooldownMinHours, winCooldownMaxHoursUnder100] (level 80-99) or
    // [winCooldownMinHours, winCooldownMaxHours] (level 100+). A max below the min
    // reads as the min.
    float winCooldownMinHours;
    float winCooldownMaxHoursUnder100;
    float winCooldownMaxHours;
    // A free animal (no owner) can be challenged too, in both presets, from this level
    // up (animal-duel-submission D1): no ally tier and no 80+ cut. Off, only people.
    bool animalsEnabled;
    int animalsFromLevel;
    // Cheat: the level a person needs for RETAR A DUELO (duel-any-source U1). Balanced
    // keeps allyTiers.eliteFromLevel with a tiered ally. 1: every level.
    int cheatPeopleFromLevel;

    DuelChallenges()
        : enabled(true),
          cooldownHours(24.0f),
          expiryHours(6.0f),
          joinBaseChance(25),
          joinChancePerWin(20),
          winCooldownMinHours(24.0f),
          winCooldownMaxHoursUnder100(72.0f),
          winCooldownMaxHours(120.0f),
          animalsEnabled(true),
          animalsFromLevel(40),
          cheatPeopleFromLevel(1)
    {
    }
};

// The proactive offer (proactive-offer, offer_requests.h): which gestures of the
// player ask the submission event for an offer -- the NPC or animal walks up and
// opens the window by itself -- and how long a failed roll keeps it calm where it
// stands instead of attacking. Off, a won duel keeps the old behaviour (Cheat joins
// at once, Balanced rolls) and a heal only marks the ledger.
struct ProactiveOffer
{
    bool onDuelWin;
    bool onHeal;
    float failCalmSeconds;

    ProactiveOffer() : onDuelWin(true), onHeal(true), failCalmSeconds(20.0f) {}
};

// Which dialogue records the plugin fires. Empty means "no FCS package
// installed" -- the plugin then falls back to acting directly, so the mod still
// works as a code-only install.
struct DialogueIds
{
    // The DIALOGUE_PACKAGE attached at runtime so coverage reaches NPCs from
    // mods that were never patched.
    std::string package;
    // DIALOGUE records fired per route.
    std::string rescue;
    std::string captive;
    // The FCS reference-list key this plugin looks for in Dialogue::_doActions
    // to learn that the player accepted.
    std::string acceptKey;

    DialogueIds()
        : package(""),
          rescue(""),
          captive(""),
          acceptKey("totalrecruit accept")
    {
    }
};

struct Config
{
    bool enabled;
    bool showRecruitButton;
    bool autoRecruit;
    std::string recruitLabel;
    int maxSpawnedCharacters;
    int panelToggleKey; // OIS keycode; 0 disables the hotkey.
    // Diagnostic: log personality, archetype and medical state for the clicked
    // character. Off by default; turn it on when something behaves oddly.
    bool probeOnClick;
    // Swap the cursor to the talk icon over anyone recruitable, the way the
    // game already signals a conversation. Without it the mod is invisible
    // until the player right-clicks on spec.
    bool showTalkCursor;
    // Right-clicking a cage resolves to the person inside it. This reads engine
    // memory at a fixed offset, so it is a switch: if a game update moves the
    // field, turning this off restores plain behaviour without a new build.
    bool resolveCageOccupant;
    // Captives call out when they are ready, so the player learns it without
    // right-clicking on spec.
    bool hailWhenReady;
    // A single shout is easy to miss, so a captive who wants to talk keeps
    // asking. In-game hours before the first call, then between each repeat,
    // and how many calls in total. Clamped on load to the ranges below.
    float hailFirstHours;
    float hailRepeatHours;
    int hailRepeats;

    // The readout card over an NPC the mod can talk to: how worn down they are, the
    // chance, the route and what stands in the way (readout.h). Pressing Hablar always
    // shows it beside the offer window. readoutOnSelect (on by default) also shows it
    // over the NPC the player selects with a left click, worked out when the selection
    // changes and every 2 s after that. The old readoutOnHover key is ignored if a
    // config file still has it.
    bool readoutEnabled;
    bool readoutCompact;
    bool readoutOnSelect;

    // How long a notice that asks the player to act (the "wants to talk" card over
    // the head of an NPC who called out, a helper's offer, the submission event's
    // "approaching"; head_notice.h) stays, in real seconds frozen on pause. Default
    // 90. Clamped on load to kHeadNoticeSecondsMin..kHeadNoticeSecondsMax
    // (head_notice_rules.h).
    float headNoticeSeconds;
    // The same for a one-line informational notice (a temporary member's join /
    // strike / farewell, a helper's start / farewell). Default 25. Clamped on load to
    // kHeadNoticeInfoSecondsMin..kHeadNoticeInfoSecondsMax (head_notice_rules.h).
    float headNoticeInfoSeconds;

    int preset; // Preset enum
    // Route A: recruit someone you saved from dying. The one route no existing
    // mod has, and the only one where the player is not exercising power.
    bool rescueEnabled;
    // Only the player's own medics count as saviours.
    bool rescuePlayerOnly;
    // Route B: recruit prisoners in your own cages.
    bool captivityEnabled;
    // Route C: a beaten NPC about to die can yield instead. No cage needed.
    bool surrenderEnabled;
    // Animals use the same engine with faction weight switched off.
    bool animalsEnabled;

    // R1.4 -- faction standing at or above this counts as friendly. Was a
    // hardcoded 25; the spec asks for 20 and for the player to be able to move it.
    float friendlyRelation;
    // Below the first of these an honourable or kind NPC has nothing to discuss
    // with you (their objection is who you are, and no money answers that); below
    // the second even the greedy stop listening, because money works only while
    // the seller expects to live to spend it. Faction standing, -100..100.
    float relationRefuseGoodNatured;
    float relationRefuseAll;
    // R1.1 / R1.2 -- Cheat keeps the ritual (a cage and a moment) and drops
    // every condition. Turn the cage off to get the old instant behaviour.
    bool cheatNeedsCage;
    // cheat-any-cage -- with the cage required, any cage counts in Cheat: another
    // faction's cage, shackles or a kidnap (SafeInPrison), not only the player's.
    bool cheatAnyCage;
    // Game hours in the cage before a Cheat prisoner calls out. In Cheat every
    // wait is at most one game minute (cheat_waits.h), so this only shortens it.
    float cheatHoursCaged;
    // R4.4 -- how much of the healing the player did counts towards the debt,
    // as a 0..1 fraction of health restored. Same shape as feeding an animal:
    // patch someone halfway and it is worth half.
    // How much a saved life argues on its own, before counting how much of
    // the healing the player actually did.
    int rescueBaseBonus;
    float rescueHealWeight;

    Prices prices;
    Chance chance;
    Insistence insistence;
    Submission submission;
    Animals animals;
    AllyTiers allyTiers;
    TempMembers tempMembers;
    Helpers helpers;
    DuelChallenges duelChallenges;
    ProactiveOffer offer;
    DialogueIds dialogue;
    Exclusions exclude;

    Config()
        : enabled(true),
          showRecruitButton(true),
          autoRecruit(true),
          recruitLabel("Hablar"),
          maxSpawnedCharacters(150),
          panelToggleKey(37), // KC_K
          probeOnClick(false),
          showTalkCursor(true),
          resolveCageOccupant(true),
          hailWhenReady(true),
          hailFirstHours(24.0f),
          hailRepeatHours(6.0f),
          hailRepeats(5),
          readoutEnabled(true),
          readoutCompact(false),
          readoutOnSelect(true),
          headNoticeSeconds(90.0f),
          headNoticeInfoSeconds(25.0f),
          friendlyRelation(20.0f),
          relationRefuseGoodNatured(-15.0f),
          relationRefuseAll(-30.0f),
          cheatNeedsCage(true),
          cheatAnyCage(true),
          cheatHoursCaged(0.02f),
          rescueBaseBonus(25),
          rescueHealWeight(30.0f),
          preset(Preset_Balanced),
          rescueEnabled(true),
          rescuePlayerOnly(true),
          captivityEnabled(true),
          surrenderEnabled(true),
          animalsEnabled(true)
    {
    }
};

// Bounds for the hailing schedule, applied when config.json is read so a hand
// edited file cannot turn a prisoner into a siren or silence them entirely.
const float kHailFirstHoursMin = 8.0f;
const float kHailFirstHoursMax = 72.0f;
const float kHailRepeatHoursMin = 1.0f;
const float kHailRepeatHoursMax = 24.0f;
const int kHailRepeatsMin = 1;
const int kHailRepeatsMax = 10;

// Loads config.json from the mod directory. A missing or malformed file leaves
// the permissive defaults in place and is reported, never fatal.
void LoadConfig(const std::string& modDirectory);

// Saves the active config back to config.json.
bool SaveConfig(const std::string& modDirectory = "");

std::string GetModDirectory();

Config& GetConfig();
} // namespace totalrecruit
