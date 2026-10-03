#include "common.h"
#include "rules.h"
#include "config.h"
#include "log.h"
#include "engine_reads.h"
#include "char_key_reader.h"
#include "character_facts.h"
#include "route_rules.h"
#include "route_decisions.h"
#include "reason_text.h"

namespace totalrecruit
{
namespace
{
// Every term, printed. A single percentage tells you the answer and nothing about
// why, so when a verdict looks wrong the log has to say which of the four
// contributions produced it -- that is the whole point of keeping them apart.
void LogChanceBreakdown(Character* character, const ChanceInputs& in, int total)
{
    const Chance& cc = GetConfig().chance;
    const Submission& cs = GetConfig().submission;
    char buf[256] = { 0 };
    _snprintf_s(buf, sizeof(buf), _TRUNCATE,
                "[chance] level=%d relation=%d personality=%d defeats=%d(x%d) "
                "situation=%d stress=%d(%.0f%%) grudge=-%d unique=-%d -> %d%%  (npc %d vs player %d, rel %.0f)",
                ChanceFromLevelGap(in.npcLevel, in.playerLevel, cc),
                ChanceFromRelation(in.relation, in.haveRelation, cc),
                ChanceFromPersonality(in.personality, cc),
                ChanceFromDefeats(in.defeats, in.personality, cs), in.defeats,
                in.situation, ChanceFromStress(in.stress, in.personality, cs),
                in.stress * 100.0f,
                in.grudge ? cc.grudgePenalty : 0,
                in.unique ? cc.uniquePenalty : 0,
                total, in.npcLevel, in.playerLevel,
                in.haveRelation ? in.relation : 0.0f);
    LogInfo(CharacterName(character) + " " + buf);
}

// Every factor of a quote, printed, for the same reason as the chance breakdown: a
// price that looks wrong has to say which multiplier made it so.
void LogPriceBreakdown(Character* character, const PriceInputs& in, int cats)
{
    const Prices& prices = GetConfig().prices;
    char buf[256] = { 0 };
    _snprintf_s(buf, sizeof(buf), _TRUNCATE,
                "[price] level=%d base=%.0f talent=x%.2f unique=x%.2f personality=x%.2f "
                "premium=x%.2f faction=x%.2f -> %d cats (floor %d, cap %d)",
                in.level, BasePriceInCats(in.level, prices), in.talentMultiplier,
                in.unique ? prices.uniqueMultiplier : 1.0f,
                PersonalityPriceMultiplier(in.personality, prices),
                PremiumMultiplier(in, prices),
                in.factionMultiplier, cats, prices.catsMinimum, prices.catsHardCap);
    LogInfo(CharacterName(character) + " " + buf);
}

// What the player is told, and what the log keeps (probeOnClick). The reason is
// turned into text here, once, whichever route produced it.
Verdict Finish(Verdict verdict, Character* character, const PriceInputs* price,
               GatherMode mode)
{
    // A peek is silent: the readout logs its own line, and the probe breakdown
    // would repeat every few hundred milliseconds.
    if (mode == Gather_Decide && GetConfig().probeOnClick)
    {
        if (verdict.rolled)
        {
            LogChanceBreakdown(character, verdict.chanceInputs, verdict.chancePercent);
        }
        if (price != NULL && verdict.currency == Currency_Cats)
        {
            LogPriceBreakdown(character, *price, verdict.cats);
        }
    }

    verdict.reason = RenderReason(verdict, character);
    return verdict;
}

void LogRoute(Character* character, Route route)
{
    // The key is what the roll is seeded from: "id:<uuid>" survives a game
    // relaunch, "handle:..." (the fallback) does not.
    CharKey probeKey;
    const bool haveKey = SafeCharKey(character, &probeKey);
    LogInfo(CharacterName(character) + " [route] " + RouteName(route) +
            " [key] " + (haveKey ? probeKey.ToString() : std::string("unreadable")));
}

void LogHold(Character* character, float held)
{
    char note[96] = { 0 };
    _snprintf_s(note, sizeof(note), _TRUNCATE,
                " [hold] held=%.1fh easy-mode needs %.1fh", held,
                GetConfig().prices.easyHoursCaged);
    LogInfo(CharacterName(character) + note);
}
} // namespace

const char* CurrencyName(Currency currency)
{
    switch (currency)
    {
    case Currency_None:       return "nada";
    case Currency_Cats:       return "cats";
    case Currency_Care:       return "cuidados";
    case Currency_Freedom:    return "libertad";
    case Currency_Submission: return "sumision";
    case Currency_Refused:    return "rechazo";
    default:                  return "?";
    }
}

namespace
{
Verdict Run(Character* character, GatherMode mode, EvaluationDetail* detail)
{
    const bool deciding = (mode == Gather_Decide);

    if (character == NULL)
    {
        Verdict nobody;
        nobody.reasonKey = Reason_Nobody;
        return Finish(nobody, character, NULL, mode);
    }

    const Config& config = GetConfig();

    // Cheat keeps the gesture and drops the price: it never reads who they are.
    if (config.preset == Preset_Cheat)
    {
        const CheatFacts facts = GatherCheatFacts(character, mode);
        if (detail != NULL)
        {
            detail->route = facts.inMyCage ? Route_Cage : Route_FreeWorld;
            detail->holdKnown = facts.holdKnown;
            detail->hoursHeld = facts.hoursHeld;
        }
        return Finish(DecideCheat(facts, config), character, NULL, mode);
    }

    CoreFacts core;
    GatherCoreFacts(character, &core);
    if (detail != NULL)
    {
        detail->haveCore = true;
        detail->core = core;
    }

    Verdict gate;
    if (DecideGate(core, config, &gate))
    {
        return Finish(gate, character, NULL, mode);
    }

    // Which route answers first is decided in one place, over plain facts:
    // route_rules.cpp. Order is not implied by the order of the cases below.
    const Route route = ChooseEntryRoute(MakeRouteFacts(core, config));
    if (detail != NULL)
    {
        detail->route = route;
    }
    if (deciding && config.probeOnClick)
    {
        LogRoute(character, route);
    }

    switch (route)
    {
    case Route_FieldYield:
        return Finish(DecideFieldYield(core, GatherYieldExtra(character, mode), config),
                      character, NULL, mode);

    case Route_Unconscious:
        if (deciding)
        {
            LogUnconsciousCondition(character);
        }
        return Finish(DecideUnconscious(core, config), character, NULL, mode);

    case Route_GrudgeRefusal:
        return Finish(DecideGrudgeRefusal(core), character, NULL, mode);

    case Route_Rescue:
        return Finish(DecideRescue(core, GatherRescueExtra(character, mode), config),
                      character, NULL, mode);

    case Route_Dying:
        return Finish(DecideDying(core), character, NULL, mode);

    case Route_EasyHealthy:
        return Finish(DecideEasyHealthy(core), character, NULL, mode);

    case Route_Surrendered:
        return Finish(DecideSurrendered(core), character, NULL, mode);

    case Route_FreeWorld:
    {
        const FreeExtra extra = GatherFreeExtra(character, core, mode);
        return Finish(DecideFreeWorld(core, extra, config), character, &extra.price, mode);
    }

    case Route_Cage:
    default:
    {
        const CageExtra extra = GatherCageExtra(character, core, mode);
        if (detail != NULL)
        {
            detail->holdKnown = extra.holdKnown;
            detail->hoursHeld = extra.hoursHeld;
        }
        if (deciding && config.probeOnClick && config.captivityEnabled &&
            config.prices.easyHoursCaged > 0.0f)
        {
            LogHold(character, extra.hoursHeld);
        }
        return Finish(DecideCage(core, extra, config), character, &extra.price, mode);
    }
    }
}
} // namespace

Verdict Evaluate(Character* character)
{
    return Run(character, Gather_Decide, NULL);
}

Verdict EvaluatePeek(Character* character, EvaluationDetail* detail)
{
    return Run(character, Gather_Peek, detail);
}
} // namespace totalrecruit
