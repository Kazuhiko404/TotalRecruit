#pragma once

#include <string>
#include <vector>

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

// How strict the rule engine is. The engine is the same in all three; only the
// numbers and which factors are consulted change.
enum Preset
{
    // Everything recruitable, click and done. The original behaviour.
    Preset_Cheat,
    // Each archetype charges in its own currency: cats, food, freeing, submission.
    Preset_Balanced,
    // As Balanced, plus faction relation as a modifier. Faction is the one thing
    // the player cannot change, so it is never a gate -- only a weight, and only
    // for players who want that roleplay.
    Preset_VanillaPlus
};

// Prices, expressed as what the NPC wants before they will join. A price that
// cannot be paid is never a refusal, only a "not yet" with a reason attached.
struct Prices
{
    // Easy mode: hours a captive must simply be held before they give in,
    // regardless of who they are -- flat, no per-personality or unique extra.
    // Counted from when the prisoner was first seen in the player's cage (see
    // hold_clock.h). 0 disables it and leaves the per-archetype prices as the
    // only route.
    float easyHoursCaged;
    // Route B: cats demanded, scaled from the NPC's rough level.
    // Price = (catsPerLevel/10) * level^2 + catsCeiling * logistic(level).
    // The square term keeps low levels affordable; the logistic carries the top
    // and flattens instead of running away. Calibrated in price_rules.cpp against
    // the Recruit-Prisoners-for-Cats tiers; the quote is that curve times talent,
    // uniqueness, personality and faction, rounded to 10 and held between
    // catsMinimum and catsHardCap (FinalPriceInCats).
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
    // Vanilla+ only: how much faction relation shifts the price. Relation runs
    // roughly -100..100; this is the fraction of the base price added at -100.
    float factionWeight;
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
    // still kept inside the int range). The reference mod's strongest uniques sit
    // at 100-150k, which is what the default allows.
    int catsHardCap;
    // The most the talent multiplier may add. 2.0 means a specialist costs at most
    // double what their level alone says.
    float talentMaxMultiplier;

    Prices()
        : easyHoursCaged(6.0f),
          catsPerLevel(90),
          catsCeiling(28000),
          priceCurveSteepness(0.09f),
          priceCurveMidpoint(31.0f),
          catsMinimum(200),
          hungerThreshold(0.0f),
          woundedThreshold(0.5f),
          fearfulMultiplier(0.6f),
          braveMultiplier(1.6f),
          factionWeight(0.75f),
          talentWeight(1.0f),
          uniqueMultiplier(2.0f),
          catsHardCap(150000),
          talentMaxMultiplier(2.0f)
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
          grudgeIsAbsolute(true)
    {
    }
};

// Submission: being beaten, and how much of a beating it takes. Everything here
// is a number the player can move, because how far a mod should let you push a
// prisoner is a taste question, not a design one.
struct Submission
{
    // Count knockouts the player inflicted. Off makes submission depend on
    // present condition alone, as it did before.
    bool countDefeats;
    // Game hours before another knockout of the same character counts again.
    // One beating raises several engine calls as the body settles, and a
    // character left down keeps registering while they lie there.
    float defeatDebounceHours;
    // What a reload leaves behind. The engine's memory can only say "this
    // happened", not how often, so a save restores this much and no more --
    // precision is lost, progress is not.
    int persistedFloorCount;
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
    // R2.3 -- roughing up a prisoner in your own cage. A deliberate act with a
    // cooldown, which is what makes it a choice rather than a grind, and it
    // knocks out rather than wounding: an accidental corpse is not a mechanic.
    // Route C -- yielding in the field, with no cage involved. How many times
    // the player has to beat the same person before they will consider giving
    // in. This is the route the spec asks for and it had become unreachable:
    // everything that read the defeat count lived behind the cage.
    int defeatsToBreak;
    // What a field yield argues on its own. Lower than the cage, because a cage
    // is a sustained condition and a beating in the open is a single moment.
    int fieldYieldBonus;
    // The stress bar: how much each kind of hardship contributes, and how much
    // the whole bar can argue for. Derived from present state, so it falls
    // again when the player feeds or treats them -- keeping someone hungry is
    // a choice that has to be maintained, not a box that stays ticked.
    float stressFromHunger;
    float stressFromWounds;
    float stressFromExhaustion;
    int maxStressChance;
    bool allowBeating;
    float beatingCooldownHours;
    float beatingKnockoutSeconds;

    Submission()
        : countDefeats(true),
          defeatDebounceHours(0.5f),
          persistedFloorCount(1),
          chancePerDefeat(12),
          maxDefeatChance(45),
          begWhenNearDeath(true),
          nearDeathBlood(0.25f),
          fearfulResolve(0.6f),
          braveResolve(1.6f),
          honourableResolve(1.8f),
          defeatsToBreak(2),
          fieldYieldBonus(10),
          stressFromHunger(0.5f),
          stressFromWounds(0.35f),
          stressFromExhaustion(0.15f),
          maxStressChance(45),
          allowBeating(true),
          beatingCooldownHours(6.0f),
          beatingKnockoutSeconds(60.0f)
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
          personalityMatters(true)
    {
    }
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
    // R1.1 / R1.2 -- Cheat keeps the ritual (a cage and a moment) and drops
    // every condition. Turn the cage off to get the old instant behaviour.
    bool cheatNeedsCage;
    // Game hours in the cage before a Cheat prisoner calls out. A minute of
    // game time by default: in Cheat the wait is a gesture, not a price.
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
          friendlyRelation(20.0f),
          cheatNeedsCage(true),
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
