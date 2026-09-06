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
    // regardless of who they are. 0 disables it and leaves the per-archetype
    // prices as the only route.
    float easyHoursCaged;
    // Route B: cats demanded, scaled from the NPC's rough level.
    // Price = (catsPerLevel/10) * level^2 + catsCeiling * logistic(level).
    // The square term keeps low levels affordable; the logistic carries the top
    // and flattens instead of running away. See PriceInCats for the calibration.
    int catsPerLevel;
    // What the strongest characters approach. Not a hard cap: talent, faction
    // and personality still multiply on top.
    int catsCeiling;
    // How sharply the price accelerates around the midpoint.
    float priceCurveSteepness;
    // The level at which the curve is steepest -- roughly "a serious fighter".
    float priceCurveMidpoint;
    int catsMinimum;
    // Optional extra trigger for "starving". The engine's own isReallyHungry
    // and isHungerKO already decide this, so 0 (off) is the sane default: the
    // raw hunger field is not a 0..1 fraction -- a healthy NPC reads about 3 --
    // so any low value here would call everyone starving.
    float hungerThreshold;
    // Overall health at or below this counts as "hurt enough to listen".
    float woundedThreshold;
    // Hours in your cage before a proud NPC will consider it.
    float hoursCagedForProud;
    // Multipliers applied to the cats price by personality.
    float fearfulMultiplier;
    float braveMultiplier;
    // Unique characters (Fenix and the like) demand submission plus this much
    // extra time. They are harder, never impossible.
    float uniqueExtraHours;
    // Vanilla+ only: how much faction relation shifts the price. Relation runs
    // roughly -100..100; this is the fraction of the base price added at -100.
    float factionWeight;
    // How hard an unusual talent moves the price. 0 disables it and prices by
    // level alone. A stat far above the character's own level is the only thing
    // that reads as "they know something you cannot see from here".
    float talentWeight;

    Prices()
        : easyHoursCaged(12.0f),
          catsPerLevel(40),
          catsCeiling(72000),
          priceCurveSteepness(0.11f),
          priceCurveMidpoint(45.0f),
          catsMinimum(200),
          hungerThreshold(0.0f),
          woundedThreshold(0.5f),
          hoursCagedForProud(12.0f),
          fearfulMultiplier(0.6f),
          braveMultiplier(1.6f),
          uniqueExtraHours(24.0f),
          factionWeight(0.75f),
          talentWeight(1.0f)
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

    Prices prices;
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
