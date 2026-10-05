#include "reason_catalog.h"

namespace totalrecruit
{
namespace
{
// One row per ReasonKey, in enum order (the tests check it). `messageKey` is the
// messages.json path; "{band}" in it is the faction band of the NPC. `text` is the
// built-in line used when messages.json has no entry, so a missing or hand-broken
// file costs flavour and nothing else.
//
// Non-ASCII is written as UTF-8 hex escapes because the v100 toolset reads this
// source as Latin-1; an escape swallows a following hex digit, so a literal is
// split there (see DEVELOPING.md, C2022).
struct Row
{
    ReasonKey key;
    const char* messageKey;
    const char* text;
    RefusalTone tone;
};

const Row kRows[] = {
    { Reason_None,                "", "", RefusalTone_Default },
    { Reason_Nobody,              "reason.nobody",                     "no hay nadie ah\xc3\xad", RefusalTone_Default },
    { Reason_AnimalsDisabled,     "reason.animalsDisabled",            "los animales est\xc3\xa1n desactivados", RefusalTone_Default },
    { Reason_UnreadableTraits,    "reason.unreadable",                 "se une a ti", RefusalTone_Default },
    { Reason_Pretending,          "reason.pretending",                 "se est\xc3\xa1 haciendo el muerto", RefusalTone_Default },
    { Reason_Dead,                "reason.dead",                       "est\xc3\xa1 muerto", RefusalTone_Default },
    { Reason_CheatJoinsFreely,    "cheat.joins",                       "se une a ti", RefusalTone_Default },
    { Reason_CheatNeedsCage,      "cheat.needsCage",                   "metelo en una de tus jaulas y hablara enseguida", RefusalTone_Default },
    { Reason_CheatWaiting,        "cheat.waiting",                     "esta dentro: dale un momento y te llamara", RefusalTone_Default },
    { Reason_CheatJoins,          "cheat.joins",                       "quiere hablar contigo: se une", RefusalTone_Default },
    { Reason_UnconsciousOwesYou,  "reason.unconscious.owesYou",        "inconsciente: te lo deber\xc3\xa1 al despertar", RefusalTone_Default },
    { Reason_UnconsciousDying,    "reason.unconscious.dying",          "se muere y no reacciona: c\xc3\xbaralo y espera", RefusalTone_Default },
    { Reason_UnconsciousWait,     "reason.unconscious.wait",           "inconsciente: espera a que despierte", RefusalTone_Default },
    { Reason_Dying,               "reason.dying",                      "se est\xc3\xa1 muriendo: c\xc3\xbaralo primero", RefusalTone_Default },
    { Reason_RescueJoins,         "rescue.joins",                      "te debe la vida", RefusalTone_Default },
    { Reason_RescueRepays,        "rescue.repays",                     "no puede seguirte, pero te deja algo en pago", RefusalTone_Default },
    { Reason_FieldYieldJoins,     "submission.broken",                 "le ganaste demasiadas veces: se rinde y te sigue", RefusalTone_Default },
    { Reason_FieldYieldNotYet,    "submission.notYet",                 "todavia no se quiebra: vencelo otra vez, o dejalo sin comer", RefusalTone_Default },
    { Reason_Surrendered,         "reason.surrendered",                "se rinde antes que morir aqu\xc3\xad", RefusalTone_Default },
    { Reason_GrudgeNever,         "grudge.never",                      "mataste a alguien suyo, y eso no se compra", RefusalTone_Grudge },
    { Reason_InOthersCage,        "reason.inOthersCage",               "est\xc3\xa1 en la jaula de otro", RefusalTone_Default },
    { Reason_Insistence,          "reason.insistence",                 "est\xc3\xa1 harto de tu insistencia: d\xc3\xa9jalo en paz un rato", RefusalTone_Insistence },
    { Reason_AllyJoins,           "faction.{band}.reason",             "os llev\xc3\xa1is bien: se une a ti", RefusalTone_Default },
    { Reason_AllyMaybe,           "faction.{band}.reasonMaybe",        "te aprecia, pero te mide: hazte m\xc3\xa1s fuerte", RefusalTone_TooWeak },
    { Reason_FactionHonour,       "faction.{band}.reasonHonourable",   "su honor no le deja tratar con alguien de tu reputaci\xc3\xb3n", RefusalTone_Honour },
    { Reason_FactionGreedy,       "faction.enemigo.reasonGreedy",      "ni por dinero: te odia demasiado", RefusalTone_Default },
    { Reason_HonourStarving,      "reason.honour.starving",            "tiene hambre y aceptaria comida", RefusalTone_Default },
    { Reason_HonourRefusesBribe,  "reason.honour.noBribes",            "su honor le impide aceptar sobornos: gana su respeto, o enci\xc3\xa9rralo y som\xc3\xa9telo", RefusalTone_Honour },
    { Reason_MilitaryLoyalty,     "reason.military",                   "sirve con lealtad a su facci\xc3\xb3n: gana reputaci\xc3\xb3n, o enci\xc3\xa9rralo y som\xc3\xa9telo", RefusalTone_Loyalty },
    { Reason_CatsAsk,             "reason.cats.ask",                   "pide {cats} cats y tienes {purse}", RefusalTone_Default },
    { Reason_CatsJoin,            "reason.cats.join",                  "se une por {cats} cats", RefusalTone_Default },
    { Reason_CaptivityDisabled,   "reason.captivityDisabled",          "el reclutamiento de prisioneros est\xc3\xa1 desactivado", RefusalTone_Default },
    { Reason_Intimidated,         "captive.intimidated",               "intimidado por tu poder, se somete", RefusalTone_Default },
    { Reason_FreedFollows,        "reason.freed",                      "seguir\xc3\xa1 a quien lo liber\xc3\xb3", RefusalTone_Default },
    { Reason_CaredFor,            "reason.cared.person",               "alimentado y curado: se unir\xc3\xa1", RefusalTone_Default },
    { Reason_Starving,            "reason.starving",                   "hambriento: dale de comer", RefusalTone_Default },
    { Reason_Wounded,             "reason.wounded",                    "herido: c\xc3\xbaralo", RefusalTone_Default },
    { Reason_StarvingAndWounded,  "reason.starvingAndWounded",         "hambriento y herido: dale comida y c\xc3\xbaralo", RefusalTone_Default },
    { Reason_BrokenNoPride,       "reason.broken",                     "quebrado y sin orgullo: te seguir\xc3\xa1", RefusalTone_Default },
    { Reason_UniqueZealot,        "unique.zealot",                     "no hay dinero que compre lo que cree: el hambre y las heridas har\xc3\xa1n el trabajo", RefusalTone_Default },
    { Reason_Defiant,             "reason.defiant",                    "sigue desafiante: d\xc3\xa9jalo con hambre o h\xc3\xa9relo", RefusalTone_Default },
    { Reason_AnimalOwned,         "animal.owned",                      "ya tiene due\xc3\xb1o: no lo vas a convencer con comida, m\xc3\xa9telo en una jaula", RefusalTone_Default },
    { Reason_TameWounded,         "animal.tame.wounded",               "est\xc3\xa1 herido: c\xc3\xbaralo antes de que conf\xc3\xad" "e en ti", RefusalTone_Default },
    { Reason_TameReady,           "animal.tame.ready",                 "comi\xc3\xb3 de tu mano y te sigue", RefusalTone_Default },
    { Reason_TameHungry,          "animal.tame.hungry",                "todav\xc3\xad" "a tiene hambre: le falta {cats}% de barriga", RefusalTone_Default },
    { Reason_TameNotFed,          "animal.tame.notFed",                "todav\xc3\xad" "a no ha comido de tu mano", RefusalTone_Default },
    { Reason_RejectCooldown,      "reason.rejectCooldown",             "no quiere saber nada de ti", RefusalTone_Grudge },
    { Reason_AllyTemporary,       "reason.ally.temporary",             "te acompa\xc3\xb1" "ar\xc3\xa1 unos d\xc3\xad" "as como miembro temporal", RefusalTone_Default },
    { Reason_AllyLeaderTooWeak,   "reason.ally.leaderTooWeak",         "eres demasiado d\xc3\xa9" "bil para que te acompa\xc3\xb1" "e", RefusalTone_TooWeak },
    { Reason_AllyCruel,           "reason.ally.cruel",                 "no camina al lado de nadie: su crueldad no se pone a tus \xc3\xb3rdenes", RefusalTone_Default },
    { Reason_AllyGreedy,          "reason.ally.greedy",                "vale m\xc3\xa1s de lo que podr\xc3\xad" "as pagarle nunca", RefusalTone_Default },
    { Reason_AnimalYieldJoins,    "animal.yield.joins",                "lo venciste demasiadas veces: agacha la cabeza y te sigue", RefusalTone_Default },
    { Reason_AnimalYieldNotYet,   "animal.yield.notYet",               "todav\xc3\xad" "a no se somete: vencelo otra vez, o dale de comer", RefusalTone_Default },
    { Reason_CheatAnimalHealed,   "cheat.animal.healed",               "lo curaste: conf\xc3\xad" "a en ti y te sigue", RefusalTone_Default },
    { Reason_CheatAnimalDefeated, "cheat.animal.defeated",             "lo venciste: te reconoce como el m\xc3\xa1s fuerte y te sigue", RefusalTone_Default },
};

const int kRowCount = (int)(sizeof(kRows) / sizeof(kRows[0]));

const Row* Find(ReasonKey key)
{
    if ((int)key < 0 || (int)key >= kRowCount)
    {
        return &kRows[0];
    }
    return &kRows[key];
}
} // namespace

std::string ReasonMessageKey(ReasonKey key, const std::string& band)
{
    std::string path = Find(key)->messageKey;
    const std::string token = "{band}";
    const std::string::size_type at = path.find(token);
    if (at != std::string::npos)
    {
        path.replace(at, token.size(), band);
    }
    return path;
}

const char* ReasonDefaultText(ReasonKey key)
{
    return Find(key)->text;
}

RefusalTone RefusalToneFor(ReasonKey key)
{
    return Find(key)->tone;
}

int ReasonCatalogSize()
{
    return kRowCount;
}

ReasonKey ReasonCatalogKeyAt(int index)
{
    return (index >= 0 && index < kRowCount) ? kRows[index].key : Reason_None;
}
} // namespace totalrecruit
