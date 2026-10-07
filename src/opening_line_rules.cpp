#include "opening_line_rules.h"

namespace totalrecruit
{
namespace
{
// One row per line, in OpeningLineId order. The texts are the lines
// npc_speech.cpp always said, byte for byte (UTF-8 written as hex escapes: the
// v100 toolset reads source as Latin-1).
enum OpeningLineId
{
    Line_Unread,
    Line_RefusalDefault,
    Line_RefusalInsistence,
    Line_RefusalTooWeak,
    Line_RefusalHonour,
    Line_RefusalLoyalty,
    Line_RefusalGrudge,
    Line_RescueHonourable,
    Line_RescueCold,
    Line_RescueFearful,
    Line_Rescue,
    Line_Freedom,
    Line_CareKind,
    Line_CareFearful,
    Line_Care,
    Line_SubmissionHonourable,
    Line_SubmissionManiacal,
    Line_SubmissionFearful,
    Line_Submission,
    Line_Ready,
    Line_CatsBandit,
    Line_CatsTrader,
    Line_CatsSlaver,
    Line_CatsFearful,
    Line_Cats,
    Line_Count
};

const OpeningLine kLines[Line_Count] = {
    { "currency.unread", "\xc2\xbfY bien? Habla de una vez." },   // Line_Unread
    { "refusal.fallback", "\xc2\xbfQui\xc3\xa9n eres t\xc3\xba? No tengo nada que hablar contigo." },   // Line_RefusalDefault
    { "refusal.insistence", "\xc2\xa1Me est\xc3\xa1s hartando! D\xc3\xa9jame en paz." },   // Line_RefusalInsistence
    { "refusal.tooWeak", "\xc2\xbfT\xc3\xba pretender darme \xc3\xb3rdenes? Eres demasiado d\xc3\xa9" "bil. L\xc3\xa1rgate." },   // Line_RefusalTooWeak
    { "refusal.honour", "El honor no se compra con gatos. No me rebajar\xc3\xa9 a esto." },   // Line_RefusalHonour
    { "refusal.loyalty", "\xc2\xbfQui\xc3\xa9n eres t\xc3\xba? \xc2\xbfQu\xc3\xa9 quieres de m\xc3\xad?" },   // Line_RefusalLoyalty
    { "refusal.grudge", "S\xc3\xa9 lo que hiciste. Fuera de mi vista." },   // Line_RefusalGrudge
    { "currency.rescueHonourable", "Estaba muerto y no me abandonaste. Eso es una deuda." },   // Line_RescueHonourable
    { "currency.rescueCold", "Me curaste. Qu\xc3\xa9 raro. Nadie hace eso por aqu\xc3\xad." },   // Line_RescueCold
    { "currency.rescueFearful", "Cre\xc3\xad que era mi fin. Y te quedaste." },   // Line_RescueFearful
    { "currency.rescue", "Pudiste seguir de largo. No lo hiciste." },   // Line_Rescue
    { "currency.freedom", "Qu\xc3\xadtame estas cadenas y ser\xc3\xa9 tuyo." },   // Line_Freedom
    { "currency.careKind", "No me interesan tus monedas. Solo valoro a quien ayuda de coraz\xc3\xb3n." },   // Line_CareKind
    { "currency.careFearful", "Por favor... dame algo de comer y te seguir\xc3\xa9." },   // Line_CareFearful
    { "currency.care", "No he comido nada. Dame comida y hablamos." },   // Line_Care
    { "currency.submissionHonourable", "Me venciste con honor, pero eso no me hace tuyo. Todav\xc3\xad" "a no." },   // Line_SubmissionHonourable
    { "currency.submissionManiacal", "\xc2\xa1" "Adelante! Haz lo peor que puedas, solo me da risa." },   // Line_SubmissionManiacal
    { "currency.submissionFearful", "No pienso pelear contigo. Solo... no me dejes aqu\xc3\xad." },   // Line_SubmissionFearful
    { "currency.submission", "Me tienes enjaulado. Eso es todo lo que tienes." },   // Line_Submission
    { "currency.ready", "Estoy listo para unirme a ti." },   // Line_Ready
    { "currency.catsBandit", "Todos tenemos un precio. El m\xc3\xado se paga en cats." },   // Line_CatsBandit
    { "currency.catsTrader", "Hablemos de negocios. Yo no trabajo gratis." },   // Line_CatsTrader
    { "currency.catsSlaver", "\xc2\xbfMe quieres? C\xc3\xb3mprame, igual que yo te comprar\xc3\xad" "a." },   // Line_CatsSlaver
    { "currency.catsFearful", "Solo quiero salir de aqu\xc3\xad. \xc2\xbfMe haces que valga la pena?" },   // Line_CatsFearful
    { "currency.cats", "Sigo a quien pague. \xc2\xbf" "Est\xc3\xa1s pagando?" },   // Line_Cats
};

const OpeningLine kNoLine = { "", "" };

OpeningLineId RefusalLine(RefusalTone tone)
{
    switch (tone)
    {
    case RefusalTone_Insistence: return Line_RefusalInsistence;
    case RefusalTone_TooWeak:    return Line_RefusalTooWeak;
    case RefusalTone_Honour:     return Line_RefusalHonour;
    // A loyal soldier has no idea who the player is; the grudge line was accusing
    // every one of them of something they never did.
    case RefusalTone_Loyalty:    return Line_RefusalLoyalty;
    // The one refusal that really is about something the player did.
    case RefusalTone_Grudge:     return Line_RefusalGrudge;
    case RefusalTone_Default:
    default:                     return Line_RefusalDefault;
    }
}

OpeningLineId RescueLine(Temperament temperament)
{
    switch (temperament)
    {
    case Temperament_Honorable: return Line_RescueHonourable;
    case Temperament_ColdCruel:
    case Temperament_Maniacal:  return Line_RescueCold;
    case Temperament_Fearful:   return Line_RescueFearful;
    default:                    return Line_Rescue;
    }
}

OpeningLineId CareLine(Temperament temperament)
{
    if (temperament == Temperament_Honorable || temperament == Temperament_WarmKind)
    {
        return Line_CareKind;
    }
    return temperament == Temperament_Fearful ? Line_CareFearful : Line_Care;
}

OpeningLineId SubmissionLine(Temperament temperament)
{
    switch (temperament)
    {
    case Temperament_Honorable: return Line_SubmissionHonourable;
    case Temperament_Maniacal:  return Line_SubmissionManiacal;
    case Temperament_Fearful:   return Line_SubmissionFearful;
    default:                    return Line_Submission;
    }
}

OpeningLineId CatsLine(OpeningSpeaker speaker, Temperament temperament)
{
    switch (speaker)
    {
    case OpeningSpeaker_Bandit: return Line_CatsBandit;
    case OpeningSpeaker_Trader: return Line_CatsTrader;
    case OpeningSpeaker_Slaver: return Line_CatsSlaver;
    case OpeningSpeaker_Other:
    default:
        return temperament == Temperament_Fearful ? Line_CatsFearful : Line_Cats;
    }
}
} // namespace

OpeningLine OpeningLineFor(const OpeningFacts& facts)
{
    if (!facts.traitsRead)
    {
        return kLines[Line_Unread];
    }
    if (facts.refused)
    {
        return kLines[RefusalLine(facts.tone)];
    }
    if (facts.viaRescue)
    {
        return kLines[RescueLine(facts.temperament)];
    }
    switch (facts.currency)
    {
    case Currency_Freedom:
        return kLines[Line_Freedom];
    case Currency_Care:
        return kLines[CareLine(facts.temperament)];
    case Currency_Submission:
        return kLines[SubmissionLine(facts.temperament)];
    case Currency_None:
        return facts.willJoin ? kLines[Line_Ready] : kNoLine;
    case Currency_Cats:
    default:
        return kLines[CatsLine(facts.speaker, facts.temperament)];
    }
}
} // namespace totalrecruit
