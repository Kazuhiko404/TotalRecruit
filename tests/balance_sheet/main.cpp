// Balance sheet: the real numbers of the Balanced preset, printed as Markdown.
//
// It calls the ACTUAL pure rules from src/ (the same files tests/rules_tests
// compiles) with the default Config, so every number in odd/balance/balance-sheet.md
// is what the mod computes, not a re-derivation. Nothing here changes a rule.
//
// Build and run (Release|x64, v100 like the plugin):
//   MSBuild tests/balance_sheet/balance_sheet.vcxproj -p:Configuration=Release -p:Platform=x64
//   tests/balance_sheet/x64/Release/balance_sheet.exe "$(git rev-parse --short HEAD)" > odd/balance/balance-sheet.md

#include "ally_tier_rules.h"
#include "animal_beating_rules.h"
#include "animal_rules.h"
#include "beating_stress_rules.h"
#include "cage_command_rules.h"
#include "cared_for_rules.h"
#include "chance_rules.h"
#include "cheat_waits.h"
#include "config.h"
#include "cooldown_minutes.h"
#include "duel_challenge_rules.h"
#include "duel_join_rules.h"
#include "personality_groups.h"
#include "reason_catalog.h"
#include "route_decisions.h"
#include "submission_rules.h"
#include "wait_format.h"

#include <cmath>
#include <cstdio>
#include <string>

using namespace totalrecruit;

namespace
{
// The representative squad leader every table compares against.
const int kLeader = 40;

// Raw engine readings for the representative states.
const float kHungerFull = 3.0f;      // fed (engine span 0..3)
const float kHungerEmpty = 0.0f;     // starving
const float kWoundHigh = 0.8f;       // worstDamage of a badly hurt part
const float kRested = 1.0f;
const float kExhausted = 0.0f;

const char* TempName(Temperament t)
{
    switch (t)
    {
    case Temperament_Normal:     return "normal";
    case Temperament_Fearful:    return "fearful";
    case Temperament_Traitorous: return "traitorous";
    case Temperament_Dumb:       return "dumb";
    case Temperament_Smart:      return "smart";
    case Temperament_Brave:      return "brave";
    case Temperament_Honorable:  return "honourable";
    case Temperament_Maniacal:   return "maniacal";
    case Temperament_ColdCruel:  return "cold/cruel";
    case Temperament_WarmKind:   return "warm/kind";
    default:                     return "?";
    }
}

const char* ArchName(Archetype a)
{
    switch (a)
    {
    case Archetype_Other:          return "other";
    case Archetype_Bandit:         return "bandit";
    case Archetype_Trader:         return "trader";
    case Archetype_Slaver:         return "slaver";
    case Archetype_Civilian:       return "civilian";
    case Archetype_Adventurer:     return "adventurer";
    case Archetype_Diplomat:       return "diplomat";
    case Archetype_Military:       return "military";
    case Archetype_LawEnforcement: return "law";
    case Archetype_Slave:          return "slave";
    default:                       return "?";
    }
}

const char* CurrencyName(Currency c)
{
    switch (c)
    {
    case Currency_None:       return "none";
    case Currency_Cats:       return "cats";
    case Currency_Care:       return "care";
    case Currency_Freedom:    return "freedom";
    case Currency_Submission: return "submission";
    case Currency_Refused:    return "refused";
    default:                  return "?";
    }
}

const char* ModeName(AllyTierMode m)
{
    switch (m)
    {
    case AllyTierMode_Free:      return "free";
    case AllyTierMode_Paid:      return "paid";
    case AllyTierMode_Temporary: return "temporary";
    case AllyTierMode_Refuse:    return "refuse";
    default:                     return "?";
    }
}

const char* GroupName(PersonalityGroup g)
{
    switch (g)
    {
    case PersonalityGroup_Good:    return "good";
    case PersonalityGroup_Neutral: return "neutral";
    case PersonalityGroup_Greedy:  return "greedy";
    case PersonalityGroup_Bad:     return "bad";
    default:                       return "?";
    }
}

// Days until a daily roll at `percent` has joined with 90% probability:
// ceil(ln 0.1 / ln(1 - p)). The join roll is re-drawn once per game day.
int DaysTo90(int percent)
{
    if (percent >= 100) return 1;
    if (percent <= 0) return -1;
    const double p = (double)percent / 100.0;
    return (int)std::ceil(std::log(0.1) / std::log(1.0 - p));
}

// The cage's JoinChance, composed exactly as DecideCage composes it
// (route_cage.cpp): no relation, intimidation as the situation, the stress bar
// from the state, defeats and the cared-for draw as given.
int CageChance(const Config& c, Temperament t, bool unique, int npcLevel, float hunger,
               float worst, float rested, float beaten, int caredFor, int defeats)
{
    ChanceInputs in;
    in.npcLevel = npcLevel;
    in.playerLevel = kLeader;
    in.relation = 0.0f;
    in.haveRelation = false;
    in.personality = t;
    in.defeats = defeats;
    in.stress = StressFromState(hunger, worst, rested, beaten, c.submission);
    in.grudge = false;
    in.unique = unique;
    in.situation = c.chance.intimidationBonus;
    in.caredFor = caredFor;
    return JoinChance(in, c.chance, c.submission);
}

// A persisted id whose day-0 join roll fails at the ceiling, so DecideCage falls
// through to the currency table (the rules_tests KeyRolling idea).
CharKey FailingKey(const Config& c)
{
    char buf[32];
    for (int i = 0; i < 100000; ++i)
    {
        sprintf_s(buf, sizeof(buf), "balance-%d", i);
        const CharKey key = CharKey::FromId(buf);
        if (!DailyRoll(key, 0, kJoinRollSalt, c.chance.ceiling))
        {
            return key;
        }
    }
    return CharKey::FromId("balance-none");
}

void PrintHeader(const char* commit)
{
    std::printf("# Hoja de balance (Balanced, config por defecto)\n\n");
    std::printf("> **Generado, no editar a mano.** Programa: `tests/balance_sheet/main.cpp`, que llama a las "
                "funciones puras REALES de `src/` con `Config()` por defecto.\n");
    std::printf("> Reglas de `src/` en el commit `%s`.\n", commit);
    std::printf("> Regenerar:\n>\n");
    std::printf("> ```bash\n");
    std::printf("> \"$MSBUILD\" tests/balance_sheet/balance_sheet.vcxproj -p:Configuration=Release -p:Platform=x64 -v:minimal -nologo\n");
    std::printf("> tests/balance_sheet/x64/Release/balance_sheet.exe \"$(git rev-parse --short HEAD)\" > odd/balance/balance-sheet.md\n");
    std::printf("> ```\n\n");
    std::printf("Supuestos comunes: lider del grupo nivel %d; la tirada de unirse se sortea una vez por dia de juego "
                "(`DailyRoll`), asi que \"dias 90%%\" = dias hasta un 90%% de probabilidad acumulada de que se una "
                "manteniendo ese estado; sin relacion (la jaula no la usa), sin rencor, 0 derrotas salvo donde se dice.\n\n",
                kLeader);
}

void PrintTermCatalogue(const Config& c)
{
    const Chance& ch = c.chance;
    const Submission& s = c.submission;
    std::printf("## 1. Catalogo de terminos\n\n");
    std::printf("`JoinChance` = nivel + relacion + personalidad + derrotas + estres + situacion + cuidado "
                "- rencor - unico, acotado a **[%d, %d]** (`chance.floor`/`ceiling`).\n\n", ch.floor, ch.ceiling);
    std::printf("| Termino | Funcion | Formula (defaults) | Min | Max |\n|---|---|---|---|---|\n");
    std::printf("| Nivel | `ChanceFromLevelGap` | base %d; NPC arriba: -%.1f/nivel (piso %d); NPC abajo: +%.1f/nivel (tope +%d) | %d | %d |\n",
                ch.levelBase, ch.levelGapPenalty, ch.levelGapFloor, ch.levelEdgeBonus, ch.levelEdgeCap,
                ChanceFromLevelGap(100, 1, ch), ChanceFromLevelGap(1, 100, ch));
    std::printf("| Relacion | `ChanceFromRelation` | relacion x %.2f (no en la jaula ni en field-yield) | %d | %d |\n",
                ch.relationWeight, ChanceFromRelation(-100.0f, true, ch), ChanceFromRelation(100.0f, true, ch));
    int pMin = 0, pMax = 0;
    for (int i = 0; i < Temperament_Count; ++i)
    {
        const int v = ChanceFromPersonality((Temperament)i, ch);
        if (v < pMin) pMin = v;
        if (v > pMax) pMax = v;
    }
    std::printf("| Personalidad | `ChanceFromPersonality` | `chance.personality[]` (tabla abajo) | %d | %d |\n", pMin, pMax);
    std::printf("| Derrotas | `ChanceFromDefeats` | %d/derrota / resolve, tope %d | 0 | %d |\n",
                s.chancePerDefeat, s.maxDefeatChance, ChanceFromDefeats(100, Temperament_Fearful, s));
    std::printf("| Estres | `ChanceFromStress` | estres(0..1) x %d / resolve, tope %d | 0 | %d |\n",
                s.maxStressChance, s.maxStressChance, ChanceFromStress(1.0f, Temperament_Fearful, s));
    std::printf("| Situacion: jaula | `intimidationBonus` | fijo | %d | %d |\n", ch.intimidationBonus, ch.intimidationBonus);
    std::printf("| Situacion: field-yield | `fieldYieldBonus` - lealtad | %d (militar/ley: -%d) | %d | %d |\n",
                s.fieldYieldBonus, s.fieldYieldLoyaltyPenalty, s.fieldYieldBonus - s.fieldYieldLoyaltyPenalty, s.fieldYieldBonus);
    std::printf("| Situacion: rescate | `rescueBaseBonus` + curado x `rescueHealWeight` | %d + %.0f x curado(0..1) | %d | %d |\n",
                c.rescueBaseBonus, c.rescueHealWeight, c.rescueBaseBonus, c.rescueBaseBonus + (int)c.rescueHealWeight);
    std::printf("| Cuidado (jaula) | `CaredForBonus` | sorteo diario estable, solo sano+alimentado+vida plena | %d | %d |\n",
                CaredForBonusFromSeed(0u, s.caredForBonusMin, s.caredForBonusMax),
                CaredForBonusFromSeed((unsigned int)(s.caredForBonusMax - s.caredForBonusMin), s.caredForBonusMin, s.caredForBonusMax));
    std::printf("| Rencor | `grudgePenalty` | fijo (honorable/valiente/amable: rechazo absoluto) | -%d | 0 |\n", ch.grudgePenalty);
    std::printf("| Unico | `uniquePenalty` | fijo | -%d | 0 |\n\n", ch.uniquePenalty);

    std::printf("### Personalidad y resolve\n\n| Temperamento | Personalidad | Resolve (divide estres, derrotas y golpes) | Estres pleno vale | Grupo |\n|---|---|---|---|---|\n");
    for (int i = 0; i < Temperament_Count; ++i)
    {
        const Temperament t = (Temperament)i;
        std::printf("| %s | %+d | %.1f | +%d | %s |\n", TempName(t), ChanceFromPersonality(t, ch),
                    ResolveMultiplier(t, s), ChanceFromStress(1.0f, t, s), GroupName(GroupOf(t)));
    }

    std::printf("\n### Barra de estres (`StressTermsFromState`)\n\n");
    std::printf("estres = clamp01(hambre x %.2f + heridas x %.2f + cansancio x %.2f + golpes x 1.00). "
                "El termino de golpes entra **sin peso**: una barra de golpes llena satura sola la barra entera.\n\n",
                s.stressFromHunger, s.stressFromWounds, s.stressFromExhaustion);
    std::printf("| Estado | hambre | heridas | cansancio | golpes | estres |\n|---|---|---|---|---|---|\n");
    struct Row { const char* label; float h, w, r, b; };
    const Row rows[] = {
        { "fresco", kHungerFull, 0.0f, kRested, 0.0f },
        { "hambriento (lleno 0)", kHungerEmpty, 0.0f, kRested, 0.0f },
        { "herido (worstDamage 0.8)", kHungerFull, kWoundHigh, kRested, 0.0f },
        { "agotado", kHungerFull, 0.0f, kExhausted, 0.0f },
        { "hambre+herido+agotado", kHungerEmpty, kWoundHigh, kExhausted, 0.0f },
        { "golpes 0.5", kHungerFull, 0.0f, kRested, 0.5f },
        { "golpes 1.0", kHungerFull, 0.0f, kRested, 1.0f },
    };
    for (int i = 0; i < (int)(sizeof(rows) / sizeof(rows[0])); ++i)
    {
        const StressTerms t = StressTermsFromState(rows[i].h, rows[i].w, rows[i].r, rows[i].b, s);
        std::printf("| %s | %.2f | %.2f | %.2f | %.2f | %.2f |\n", rows[i].label, t.hunger, t.wounds, t.tired, t.beaten,
                    StressFromState(rows[i].h, rows[i].w, rows[i].r, rows[i].b, s));
    }

    std::printf("\n### Termino de nivel por diferencia (NPC - lider)\n\n| Diferencia | ");
    const int gaps[] = { -60, -40, -27, -20, -10, 0, 10, 20, 30, 40, 50, 60 };
    const int nGaps = (int)(sizeof(gaps) / sizeof(gaps[0]));
    for (int i = 0; i < nGaps; ++i) std::printf("%+d | ", gaps[i]);
    std::printf("\n|---|");
    for (int i = 0; i < nGaps; ++i) std::printf("---|");
    std::printf("\n| Nivel | ");
    for (int i = 0; i < nGaps; ++i) std::printf("%d | ", ChanceFromLevelGap(kLeader + gaps[i], kLeader, ch));
    std::printf("\n\n");
}

void PrintCageMatrix(const Config& c)
{
    const Submission& s = c.submission;
    std::printf("## 2. Jaula: probabilidad diaria de unirse (Balanced)\n\n");
    std::printf("Lider %d. Intimidacion +%d siempre. Columnas: **sano** = estres 0 sin bono (referencia; en el juego un preso "
                "sano, alimentado y a vida plena SIEMPRE tiene el bono de cuidado); **cuid. min/max** = sano + bono %d / %d; "
                "**hambre** = lleno 0; **herido** = worstDamage 0.8; **H+H+C** = hambre + herido + agotado (estres %.2f); "
                "**golpes** = barra de golpes 1.0, por lo demas sano (sin bono: los golpes hieren); **quebrado** = todo junto (estres 1.0). "
                "Entre parentesis: dias hasta 90%% acumulado.\n\n",
                kLeader, c.chance.intimidationBonus, s.caredForBonusMin, s.caredForBonusMax,
                StressFromState(kHungerEmpty, kWoundHigh, kExhausted, 0.0f, s));
    std::printf("| Temperamento | Unico | Nivel NPC | sano | cuid. min | cuid. max | hambre | herido | H+H+C | golpes | quebrado |\n");
    std::printf("|---|---|---|---|---|---|---|---|---|---|---|\n");
    const Temperament temps[] = { Temperament_Fearful, Temperament_Normal, Temperament_Brave, Temperament_Honorable };
    const int levels[] = { kLeader - 20, kLeader, kLeader + 20, kLeader + 50 };
    for (int u = 0; u < 2; ++u)
    {
        for (int ti = 0; ti < 4; ++ti)
        {
            for (int li = 0; li < 4; ++li)
            {
                const Temperament t = temps[ti];
                const bool unique = u == 1;
                const int lv = levels[li];
                const int cells[] = {
                    CageChance(c, t, unique, lv, kHungerFull, 0.0f, kRested, 0.0f, 0, 0),
                    CageChance(c, t, unique, lv, kHungerFull, 0.0f, kRested, 0.0f, s.caredForBonusMin, 0),
                    CageChance(c, t, unique, lv, kHungerFull, 0.0f, kRested, 0.0f, s.caredForBonusMax, 0),
                    CageChance(c, t, unique, lv, kHungerEmpty, 0.0f, kRested, 0.0f, 0, 0),
                    CageChance(c, t, unique, lv, kHungerFull, kWoundHigh, kRested, 0.0f, 0, 0),
                    CageChance(c, t, unique, lv, kHungerEmpty, kWoundHigh, kExhausted, 0.0f, 0, 0),
                    CageChance(c, t, unique, lv, kHungerFull, 0.0f, kRested, 1.0f, 0, 0),
                    CageChance(c, t, unique, lv, kHungerEmpty, kWoundHigh, kExhausted, 1.0f, 0, 0),
                };
                std::printf("| %s | %s | %d (%+d) |", TempName(t), unique ? "si" : "no", lv, lv - kLeader);
                for (int k = 0; k < 8; ++k)
                {
                    std::printf(" %d (%d) |", cells[k], DaysTo90(cells[k]));
                }
                std::printf("\n");
            }
        }
    }

    std::printf("\n### Cuidar contra quebrar (mismo NPC, nivel igual al lider)\n\n");
    std::printf("| Temperamento | Unico | cuidado medio (+%d) | quebrado | ventaja de quebrar | dias 90%% cuidando | dias 90%% quebrando |\n|---|---|---|---|---|---|---|\n",
                (s.caredForBonusMin + s.caredForBonusMax) / 2);
    for (int u = 0; u < 2; ++u)
    {
        for (int ti = 0; ti < 4; ++ti)
        {
            const Temperament t = temps[ti];
            const int care = CageChance(c, t, u == 1, kLeader, kHungerFull, 0.0f, kRested, 0.0f,
                                        (s.caredForBonusMin + s.caredForBonusMax) / 2, 0);
            const int broken = CageChance(c, t, u == 1, kLeader, kHungerEmpty, kWoundHigh, kExhausted, 1.0f, 0, 0);
            std::printf("| %s | %s | %d | %d | %+d | %d | %d |\n", TempName(t), u == 1 ? "si" : "no", care, broken,
                        broken - care, DaysTo90(care), DaysTo90(broken));
        }
    }
    std::printf("\nEl bono de cuidado maximo (+%d) vale %d%% de lo que vale el estres pleno de un normal (+%d).\n\n",
                s.caredForBonusMax, s.caredForBonusMax * 100 / ChanceFromStress(1.0f, Temperament_Normal, s),
                ChanceFromStress(1.0f, Temperament_Normal, s));
}

void PrintCageVerdicts(const Config& c)
{
    std::printf("### Que pasa cuando la tirada de la jaula FALLA (`DecideCage`, tabla de monedas)\n\n");
    std::printf("Preso sano, alimentado, vida plena, nivel igual al lider, bolsillo conocido en 0 cats, clave cuya tirada falla. "
                "`se une` = se une igual aunque la tirada fallo.\n\n");
    std::printf("| Temperamento | Arquetipo | Unico | Moneda | Se une | Motivo |\n|---|---|---|---|---|---|\n");
    const CharKey key = FailingKey(c);
    const Temperament temps[] = { Temperament_Fearful, Temperament_Normal, Temperament_Brave,
                                  Temperament_Honorable, Temperament_WarmKind, Temperament_ColdCruel };
    const Archetype archs[] = { Archetype_Other, Archetype_Bandit, Archetype_Military, Archetype_Slave };
    for (int u = 0; u < 2; ++u)
    {
        for (int ti = 0; ti < 6; ++ti)
        {
            for (int ai = 0; ai < 4; ++ai)
            {
                CoreFacts core;
                core.temperament = temps[ti];
                core.archetype = archs[ai];
                core.enslaved = archs[ai] == Archetype_Slave;
                core.unique = u == 1;
                core.npcLevel = kLeader;
                core.playerLevel = kLeader;
                core.inMyCage = true;
                CageExtra extra;
                extra.roll.haveKey = true;
                extra.roll.key = key;
                extra.roll.dayIndex = 0;
                extra.care = extra.roll;
                extra.purse.known = true;
                extra.purse.cats = 0;
                const Verdict v = DecideCage(core, extra, c);
                std::printf("| %s | %s | %s | %s | %s | %s |\n", TempName(temps[ti]), ArchName(archs[ai]),
                            u == 1 ? "si" : "no", CurrencyName(v.currency), v.willJoin ? "**si**" : "no",
                            ReasonDefaultText(v.reasonKey));
            }
        }
    }
    std::printf("\n");
}

struct BeatSim
{
    int sessionsToFull;   // -1 when it never reaches 1.0 in 72 h
    float plateau;        // value right after the last session of 72 h
    float at24;           // value at hour 24 after sessions at hours 0..23
};

BeatSim SimulateBeating(float hitGain, float koGain, bool useHit, bool useKo, const Submission& s)
{
    BeatingStressLedger ledger;
    const CharKey key = CharKey::FromId("sim");
    BeatSim r;
    r.sessionsToFull = -1;
    r.plateau = 0.0f;
    r.at24 = 0.0f;
    for (int h = 0; h < 72; ++h)
    {
        float v = 0.0f;
        if (useHit) v = ledger.Add(key, hitGain, (double)h, s.beatingStressFadeHours);
        if (useKo) v = ledger.Add(key, koGain, (double)h, s.beatingStressFadeHours);
        if (r.sessionsToFull < 0 && v >= 0.999f) r.sessionsToFull = h + 1;
        if (h == 23) r.at24 = ledger.Get(key, 24.0, s.beatingStressFadeHours);
        r.plateau = v;
    }
    return r;
}

void PrintBeating(const Config& c)
{
    const Submission& s = c.submission;
    std::printf("## 3. Estres por golpes (Golpear / Noquear)\n\n");
    std::printf("ganancia = clamp(base x factor de nivel / resolve, %.2f, %.2f); base Golpear %.2f, Noquear %.2f; "
                "factor = lider/NPC acotado a [%.1f, %.1f]; descanso Golpear %.0f h, Noquear %.0f h; "
                "una barra llena se va en %.0f h (-%.4f/h). Simulacion: un golpe (y/o un noqueo) cada vez que el descanso "
                "termina, 72 h seguidas, lider %d.\n\n",
                s.beatingStressMin, s.beatingStressMax, s.beatStressBase, s.knockoutStressBase,
                s.beatingStressLevelMin, s.beatingStressLevelMax, s.beatingCooldownHours, s.knockoutCooldownHours,
                s.beatingStressFadeHours, 1.0f / s.beatingStressFadeHours, kLeader);
    std::printf("| Temperamento | Nivel NPC | factor | +Golpear | +Noquear | sesiones a 1.0 (solo Golpear) | meseta solo Golpear | "
                "a las 24 h solo Golpear | sesiones a 1.0 (Golpear+Noquear) | estres pleno vale |\n");
    std::printf("|---|---|---|---|---|---|---|---|---|---|\n");
    const Temperament temps[] = { Temperament_Fearful, Temperament_Normal, Temperament_Brave, Temperament_Honorable };
    const int levels[] = { 20, 27, 40, 60, 80 };
    for (int ti = 0; ti < 4; ++ti)
    {
        for (int li = 0; li < 5; ++li)
        {
            const Temperament t = temps[ti];
            const int lv = levels[li];
            const float hit = BeatingStressGain(BeatingBlow_Hit, kLeader, lv, t, s);
            const float ko = BeatingStressGain(BeatingBlow_Knockout, kLeader, lv, t, s);
            const BeatSim onlyHit = SimulateBeating(hit, ko, true, false, s);
            const BeatSim both = SimulateBeating(hit, ko, true, true, s);
            char toFull[32];
            char toFullBoth[32];
            if (onlyHit.sessionsToFull < 0) sprintf_s(toFull, sizeof(toFull), "nunca"); else sprintf_s(toFull, sizeof(toFull), "%d", onlyHit.sessionsToFull);
            if (both.sessionsToFull < 0) sprintf_s(toFullBoth, sizeof(toFullBoth), "nunca"); else sprintf_s(toFullBoth, sizeof(toFullBoth), "%d", both.sessionsToFull);
            std::printf("| %s | %d | %.2f | %.3f | %.3f | %s | %.2f (+%d) | %.2f (+%d) | %s | +%d |\n",
                        TempName(t), lv, BeatingLevelFactor(kLeader, lv, s), hit, ko, toFull,
                        onlyHit.plateau, ChanceFromStress(StressFromState(kHungerFull, 0.0f, kRested, onlyHit.plateau, s), t, s),
                        onlyHit.at24, ChanceFromStress(StressFromState(kHungerFull, 0.0f, kRested, onlyHit.at24, s), t, s),
                        toFullBoth, ChanceFromStress(1.0f, t, s));
        }
    }
    std::printf("\nEl umbral para saturar a 1 golpe por hora es una ganancia > %.4f (lo que se desvanece en 1 h). "
                "Una barra llena tarda %.0f h en irse si se deja de golpear. Golpear ademas hiere, asi que el preso golpeado "
                "pierde el bono de cuidado y suma el termino de heridas.\n\n",
                1.0f / s.beatingStressFadeHours, s.beatingStressFadeHours);
}

int OpenChance(const Config& c, Temperament t, int npcLevel, int defeats, float worst, int situation,
               bool haveRelation, float relation)
{
    ChanceInputs in;
    in.npcLevel = npcLevel;
    in.playerLevel = kLeader;
    in.relation = relation;
    in.haveRelation = haveRelation;
    in.personality = t;
    in.defeats = defeats;
    in.stress = StressFromState(kHungerFull, worst, kRested, 0.0f, c.submission);
    in.situation = situation;
    return JoinChance(in, c.chance, c.submission);
}

void PrintOtherRoutes(const Config& c)
{
    const Submission& s = c.submission;
    std::printf("## 4. Otras rutas\n\n");
    std::printf("### Field-yield (vencido en el campo, sin jaula; se abre con %d derrotas)\n\n", s.defeatsToBreak);
    std::printf("Situacion +%d (militar/ley: %+d). Nivel igual al lider. Recien vencido = worstDamage 0.8 (estres %.2f).\n\n",
                s.fieldYieldBonus, s.fieldYieldBonus - s.fieldYieldLoyaltyPenalty,
                StressFromState(kHungerFull, kWoundHigh, kRested, 0.0f, s));
    std::printf("| Temperamento | 2 derrotas | 3 derrotas | 4 derrotas | 4 derr. +20 niveles | 2 derr. leal | 4 derr. leal |\n|---|---|---|---|---|---|---|\n");
    const Temperament temps[] = { Temperament_Fearful, Temperament_Normal, Temperament_Brave,
                                  Temperament_Honorable, Temperament_Maniacal };
    const int open = s.fieldYieldBonus;
    const int loyal = s.fieldYieldBonus - s.fieldYieldLoyaltyPenalty;
    for (int ti = 0; ti < 5; ++ti)
    {
        const Temperament t = temps[ti];
        std::printf("| %s | %d | %d | %d | %d | %d | %d |\n", TempName(t),
                    OpenChance(c, t, kLeader, 2, kWoundHigh, open, false, 0.0f),
                    OpenChance(c, t, kLeader, 3, kWoundHigh, open, false, 0.0f),
                    OpenChance(c, t, kLeader, 4, kWoundHigh, open, false, 0.0f),
                    OpenChance(c, t, kLeader + 20, 4, kWoundHigh, open, false, 0.0f),
                    OpenChance(c, t, kLeader, 2, kWoundHigh, loyal, false, 0.0f),
                    OpenChance(c, t, kLeader, 4, kWoundHigh, loyal, false, 0.0f));
    }

    std::printf("\n### Rescate (situacion %d + %.0f x curado)\n\n", c.rescueBaseBonus, c.rescueHealWeight);
    std::printf("Nivel igual al lider, sin derrotas, sano al despertar (estres 0), relacion desconocida salvo la ultima columna.\n\n");
    std::printf("| Temperamento | curado 0 (+%d) | curado 0.5 (+%d) | curado 1 (+%d) | curado 1, NPC +20 | curado 1, relacion -30 |\n|---|---|---|---|---|---|\n",
                c.rescueBaseBonus, c.rescueBaseBonus + (int)(0.5f * c.rescueHealWeight), c.rescueBaseBonus + (int)c.rescueHealWeight);
    for (int ti = 0; ti < 5; ++ti)
    {
        const Temperament t = temps[ti];
        const int s0 = c.rescueBaseBonus;
        const int s5 = c.rescueBaseBonus + (int)(0.5f * c.rescueHealWeight);
        const int s1 = c.rescueBaseBonus + (int)(1.0f * c.rescueHealWeight);
        std::printf("| %s | %d | %d | %d | %d | %d |\n", TempName(t),
                    OpenChance(c, t, kLeader, 0, 0.0f, s0, false, 0.0f),
                    OpenChance(c, t, kLeader, 0, 0.0f, s5, false, 0.0f),
                    OpenChance(c, t, kLeader, 0, 0.0f, s1, false, 0.0f),
                    OpenChance(c, t, kLeader + 20, 0, 0.0f, s1, false, 0.0f),
                    OpenChance(c, t, kLeader, 0, 0.0f, s1, true, -30.0f));
    }

    const DuelChallenges& d = c.duelChallenges;
    std::printf("\n### Duelo ganado (80+, TotalSparring)\n\n");
    std::printf("chance = min(100, %d + %d x (victorias - 1)); si no se une, enfriamiento %.0f-%.0f h (80-99) o %.0f-%.0f h (100+).\n\n",
                d.joinBaseChance, d.joinChancePerWin, d.winCooldownMinHours, d.winCooldownMaxHoursUnder100,
                d.winCooldownMinHours, d.winCooldownMaxHours);
    std::printf("| Victoria n | chance | unido para la n-esima (acumulado) |\n|---|---|---|\n");
    double notYet = 1.0;
    for (int w = 1; w <= 6; ++w)
    {
        const int p = DuelJoinChance(w, d);
        notYet *= 1.0 - (double)p / 100.0;
        std::printf("| %d | %d | %.0f%% |\n", w, p, (1.0 - notYet) * 100.0);
    }

    const AllyTiers& a = c.allyTiers;
    std::printf("\n### Aliados por nivel (relacion >= %.0f)\n\n", c.friendlyRelation);
    std::printf("Cortes: medio %d, alto %d, elite %d, leyenda %d; descuento = min(tope, relacion/100 x %.1f); "
                "temporal si NPC - lider <= %d; temporal %d-%d dias.\n\n",
                a.midFromLevel, a.highFromLevel, a.eliteFromLevel, a.legendFromLevel, a.discountScale,
                a.closeLevelGap, a.tempMemberMinDays, a.tempMemberMaxDays);
    std::printf("| Tramo | tope | desc. relacion 20 | desc. relacion 50 | desc. relacion 100 |\n|---|---|---|---|---|\n");
    const char* tierNames[] = { "bajo (<20)", "medio (20-49)", "alto (50-79)", "elite (80-99)", "leyenda (100+)" };
    for (int tr = 0; tr <= (int)AllyTier_Legend; ++tr)
    {
        const AllyTier tier = (AllyTier)tr;
        std::printf("| %s | %d%% | %d%% | %d%% | %d%% |\n", tierNames[tr], DiscountPercent(AllyTierMaxDiscount(tier, a)),
                    DiscountPercent(FriendshipDiscount(tier, 20.0f, a)), DiscountPercent(FriendshipDiscount(tier, 50.0f, a)),
                    DiscountPercent(FriendshipDiscount(tier, 100.0f, a)));
    }
    const int leaders[] = { kLeader, 80 };
    const int lvls[] = { 10, 30, 60, 85, 105 };
    for (int ld = 0; ld < 2; ++ld)
    {
        std::printf("\n| Nivel NPC (lider %d, relacion 50) | good | neutral | greedy | bad |\n|---|---|---|---|---|\n",
                    leaders[ld]);
        for (int li = 0; li < 5; ++li)
        {
            std::printf("| %d |", lvls[li]);
            for (int g = 0; g < 4; ++g)
            {
                const AllyTierDecision dec = DecideAllyTier(lvls[li], leaders[ld], (PersonalityGroup)g, 50.0f, a);
                std::printf(" %s", ModeName(dec.mode));
                if (dec.mode == AllyTierMode_Paid) std::printf(" -%d%%", DiscountPercent(dec.discountFraction));
                std::printf(" |");
            }
            std::printf("\n");
        }
    }
    std::printf("\n");
}

void PrintCheatLeftovers(const Config& c)
{
    Config cheat = c;
    cheat.preset = Preset_Cheat;
    std::printf("## 5. Restos de Cheat (esperas de 1 minuto)\n\n");
    std::printf("`kCheatWaitHours` = %.4f h. El rechazo y el duelo se guardan como el MINUTO de juego en que terminan "
                "(`CooldownUntilMinute`, priority-one B): una espera de 1 h o mas termina en la hora entera redondeada "
                "hacia arriba (como antes), una menor de 1 h en el minuto redondeado hacia arriba. Las etiquetas muestran "
                "minutos bajo 1 h (`ShowWait`) y horas desde 1 h.\n\n", kCheatWaitHours);
    std::printf("| Espera | Cheat (h) | empieza 10.25 h, termina | etiqueta |\n|---|---|---|---|\n");
    const double now = 10.25;
    const WaitTexts texts;
    const int rejectUntil = CooldownUntilMinute(now, RejectCooldownHours(cheat));
    const int duelUntil = DuelCooldownUntilMinute(now, DuelLossCooldownHours(cheat));
    std::printf("| rechazo del sometido | %.4f | minuto %d (%.0f min despues) | \"%s\" |\n", RejectCooldownHours(cheat),
                rejectUntil, rejectUntil - now * 60.0, FillWait(texts, ShowWait(RejectCooldownHours(cheat))).c_str());
    std::printf("| duelo perdido | %.4f | minuto %d (%.0f min despues) | \"%s\" |\n", DuelLossCooldownHours(cheat),
                duelUntil, duelUntil - now * 60.0, FillWait(texts, ShowWait(DuelLossCooldownHours(cheat))).c_str());
    std::printf("| descanso Golpear | %.4f | (ledger en horas de juego) | \"%s\" |\n", BeatingCooldownHours(cheat),
                RestLabel("Golpear", BeatingCooldownHours(cheat), texts).c_str());
    std::printf("| descanso Noquear | %.4f | (ledger en horas de juego) | \"%s\" |\n\n", KnockoutCooldownHours(cheat),
                RestLabel("Noquear", KnockoutCooldownHours(cheat), texts).c_str());
    const int balancedReject = CooldownUntilMinute(now, RejectCooldownHours(c));
    std::printf("Comparacion Balanced: el rechazo (%.0f h) empezado a las 10.25 h termina en el minuto %d (hora %.2f); "
                "Golpear en descanso pleno se lee \"%s\".\n\n", RejectCooldownHours(c), balancedReject,
                balancedReject / 60.0, RestLabel("Golpear", BeatingCooldownHours(c), texts).c_str());
}

void PrintCagedAnimalBeating(const Config& c)
{
    const Animals& a = c.animals;
    std::printf("## 6. Animal enjaulado: los golpes bajan la barra de comida (Balanced, priority-one D)\n\n");
    std::printf("`DecideCagedAnimal` (`route_animal.cpp`): resolve = max(%.1f, %.2f + nivel x %.3f); "
                "descuento = clamp(%.2f x estres / resolve, 0, %.2f); barra = barra x (1 - descuento). "
                "La primera comida de mano del jugador sigue siendo obligatoria. Barra base = "
                "`TamingFullnessNeeded` de un adulto (madurez 1.0) frente al lider %d, sin temperamento. "
                "Estres por Golpear con temperamento normal (seccion 3).\n\n",
                kAnimalBeatingResolveFloor, a.cageBeatingResolveBase, a.cageBeatingResolvePerLevel,
                a.cageBeatingWeight, kAnimalBeatingMaxDiscount, kLeader);
    std::printf("| Nivel animal | resolve | +Golpear | barra base | estres 0.25 | estres 0.5 | estres 1.0 |\n"
                "|---|---|---|---|---|---|---|\n");
    const int levels[] = { 5, 20, 40, 60, 90 };
    const float stresses[] = { 0.25f, 0.5f, 1.0f };
    for (int li = 0; li < 5; ++li)
    {
        const int lv = levels[li];
        const float bar = TamingFullnessNeeded(1.0f, lv, kLeader, a);
        std::printf("| %d | %.2f | %.3f | %.0f%% |", lv, AnimalBeatingResolve(lv, a),
                    BeatingStressGain(BeatingBlow_Hit, kLeader, lv, Temperament_Normal, c.submission), bar * 100.0f);
        for (int si = 0; si < 3; ++si)
        {
            std::printf(" %.0f%% (-%.0f%%) |", ApplyAnimalBeating(bar, stresses[si], lv, a) * 100.0f,
                        AnimalBeatingDiscount(stresses[si], lv, a) * 100.0f);
        }
        std::printf("\n");
    }
    std::printf("\nCeldas: barra de estomago que debe alcanzar (descuento aplicado). Con `animalCageBeatingCounts` "
                "apagado el descuento es 0.\n\n");
}
} // namespace

int main(int argc, char** argv)
{
    const char* commit = argc > 1 ? argv[1] : "desconocido";
    const Config config;   // the defaults: Balanced
    PrintHeader(commit);
    PrintTermCatalogue(config);
    PrintCageMatrix(config);
    PrintCageVerdicts(config);
    PrintBeating(config);
    PrintOtherRoutes(config);
    PrintCheatLeftovers(config);
    PrintCagedAnimalBeating(config);
    return 0;
}
