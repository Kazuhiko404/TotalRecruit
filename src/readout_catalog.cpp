#include "readout_catalog.h"

namespace totalrecruit
{
namespace
{
// One row per id, in ReasonKey order and then the card's own lines: the cage wait,
// the open roll, the field-yield beat-again, a Balanced ally's discounted price
// (join / short) and a temporary member's days left (the test checks the count). Text is plain ASCII like the rest of messages.json, and short: the card
// does not wrap. A row with empty text is a verdict the card is hidden for.
struct Row
{
    const char* key;
    const char* text;
};

const Row kRows[] = {
    { "",                                    "" },                                       // None
    { "readout.line.nobody",                 "" },                                       // Nobody
    { "readout.line.animalsDisabled",        "" },                                       // AnimalsDisabled
    { "readout.line.unreadable",             "Se une" },
    { "readout.line.pretending",             "" },                                       // Pretending
    { "readout.line.dead",                   "" },                                       // Dead
    { "readout.line.cheat.joinsFreely",      "Se une" },
    { "readout.line.cheat.needsCage",        "Metelo en una jaula" },
    { "readout.line.cheat.waiting",          "Espera un momento en la jaula" },
    { "readout.line.cheat.joins",            "Quiere hablar: se une" },
    { "readout.line.unconscious.owesYou",    "Inconsciente: te lo debe al despertar" },
    { "readout.line.unconscious.dying",      "Se muere: curalo y espera" },
    { "readout.line.unconscious.wait",       "Inconsciente: espera a que despierte" },
    { "readout.line.dying",                  "Se muere: curalo primero" },
    { "readout.line.rescue.joins",           "Te debe la vida: se une" },
    { "readout.line.rescue.repays",          "No puede seguirte: te paga con lo suyo" },
    { "readout.line.easyHealthy",            "Sano gracias a ti: se une" },
    { "readout.line.fieldYield.joins",       "Se rinde y te sigue" },
    { "readout.line.fieldYield.notYet",      "Aun no cede: dejalo sin comer" },
    { "readout.line.surrendered",            "Se rinde antes que morir" },
    { "readout.line.grudge.never",           "Rencor: no perdona fuera de la jaula" },
    { "readout.line.inOthersCage",           "Esta en la jaula de otro" },
    { "readout.line.insistence",             "Harto de ti: dejalo en paz un rato" },
    { "readout.line.ally.joins",             "Aliado: se une" },
    { "readout.line.ally.maybe",             "Quiza te siga; si no, hazte mas fuerte" },
    { "readout.line.faction.honour",         "Su honor no trata con tu reputacion" },
    { "readout.line.faction.greedy",         "Te odia: ni por dinero" },
    { "readout.line.honour.starving",        "Tiene hambre: dale de comer" },
    { "readout.line.honour.noBribes",        "Honor: no acepta sobornos" },
    { "readout.line.military",               "Leal a su faccion: no se compra" },
    { "readout.line.cats.ask",               "Precio {cats} cats (tienes {purse})" },
    { "readout.line.cats.join",              "Se une por {cats} cats" },
    { "readout.line.captivityDisabled",      "Reclutar prisioneros esta desactivado" },
    { "readout.line.easyWait",               "Lleva bastante encerrado: se rinde" },
    { "readout.line.intimidated",            "Intimidado: se somete" },
    { "readout.line.freed",                  "Seguira a quien lo libero" },
    { "readout.line.cared.person",           "Alimentado y curado: se une" },
    { "readout.line.starving",               "Hambriento: dale de comer" },
    { "readout.line.wounded",                "Herido: curalo" },
    { "readout.line.starvingAndWounded",     "Hambriento y herido: comida y cura" },
    { "readout.line.broken",                 "Quebrado: te seguira" },
    { "readout.line.unique.zealot",          "Solo el hambre y las heridas lo quiebran" },
    { "readout.line.defiant",                "Desafiante: hambre o golpes lo ablandan" },
    { "readout.line.animal.owned",           "Tiene dueno: metelo en una jaula" },
    { "readout.line.animal.tame.wounded",    "Herido: curalo antes de domarlo" },
    { "readout.line.animal.tame.ready",      "Comio de tu mano: te sigue" },
    { "readout.line.animal.tame.hungry",     "Hambre {food}% / necesita {need}%" },
    { "readout.line.animal.tame.notFed",     "No comio de tu mano: dale comida" },
    { "readout.line.rejectCooldown",         "No quiere saber nada de ti" },
    { "readout.line.ally.temporary",         "Aliado: te acompana unos dias" },
    { "readout.line.ally.leaderTooWeak",     "Aliado: eres muy debil para el" },
    { "readout.line.ally.cruel",             "Cruel: no camina a tu lado" },
    { "readout.line.ally.greedy",            "Ambicioso: no hay precio para el" },
    { "readout.line.waitCage",              "Faltan {hours} en la jaula" },
    { "readout.line.rollOpen",               "{chance}%: puede unirse si le hablas" },    // kLineRollOpen
    { "readout.line.fieldYield.beatAgain",   "Aun no cede: vencelo otra vez" },          // kLineFieldYieldBeatAgain
    { "readout.line.ally.paidJoin",          "Aliado: {cats} cats con descuento" },      // kLineAllyPaidJoin
    { "readout.line.ally.paidAsk",           "Aliado: {cats} cats (tienes {purse})" },   // kLineAllyPaidAsk
    { "readout.line.temp.daysLeft",          "Temporal: le quedan {need} dias" },        // kLineTempMemberDaysLeft
};

// One label per ReadoutRoute, in enum order.
const Row kRoutes[] = {
    { "readout.route.free",       "Libre" },
    { "readout.route.captivity",  "Cautiverio" },
    { "readout.route.rescue",     "Rescate" },
    { "readout.route.surrender",  "Rendicion" },
    { "readout.route.mercy",      "Piedad" },
    { "readout.route.animal",     "Animal" },
    { "readout.route.fieldYield", "Sometido" },
};

const int kRouteCount = (int)(sizeof(kRoutes) / sizeof(kRoutes[0]));

// An unknown route reads as the open world, like the card always did.
int RouteRow(ReadoutRoute route)
{
    const int i = (int)route;
    return (i >= 0 && i < kRouteCount) ? i : 0;
}

const int kRowCount = (int)(sizeof(kRows) / sizeof(kRows[0]));
} // namespace

int ReadoutLineCount()
{
    return kRowCount;
}

const char* ReadoutLineMessageKey(int id)
{
    return (id >= 0 && id < kRowCount) ? kRows[id].key : "";
}

const char* ReadoutLineDefaultText(int id)
{
    return (id >= 0 && id < kRowCount) ? kRows[id].text : "";
}

const char* ReadoutRouteMessageKey(ReadoutRoute route)
{
    return kRoutes[RouteRow(route)].key;
}

const char* ReadoutRouteDefaultText(ReadoutRoute route)
{
    return kRoutes[RouteRow(route)].text;
}
} // namespace totalrecruit
