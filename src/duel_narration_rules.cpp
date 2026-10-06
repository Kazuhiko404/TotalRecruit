#include "duel_narration_rules.h"

namespace totalrecruit
{
namespace
{
// One row per DuelNarrationId, in enum order. The person's texts are the ones the duel
// already shipped with (messages.json duel.*); the animal's are a narrator speaking
// about it. Plain ASCII, like messages.json.
const DuelNarration kPerson[DuelNarration_Count] = {
    { "duel.challenged", "{name} acepta el reto: elige el duelo en la ventana de TotalSparring." },
    { "duel.joined",     "{name} perdio el duelo y se une a tu grupo para siempre." },
    { "duel.stays",      "{name} perdio el duelo, pero no se une ({chance}% de que se uniera). "
                         "Podras retarlo otra vez en {hours} h." },
    { "duel.lost",       "{name} gano el duelo. Vuelve cuando seas mas fuerte (podras retarlo en {hours} h)." },
    { "duel.needMoreWins", "{name} perdio el duelo, pero aun no se une. Victorias que faltan: {wins}." },
};

const DuelNarration kAnimal[DuelNarration_Count] = {
    { "duel.animal.challenged", "{name} grune y se planta frente a ti: elige el duelo en la ventana de TotalSparring." },
    { "duel.animal.joined",     "{name} agacha la cabeza y te sigue: ahora es parte de tu grupo." },
    { "duel.animal.stays",      "{name} se sacude y se aleja sin seguirte ({chance}% de que se uniera). "
                                "Podras retarlo otra vez en {hours} h." },
    { "duel.animal.lost",       "{name} gana el duelo y se aleja sin volverse. Podras retarlo otra vez en {hours} h." },
    { "duel.animal.needMoreWins", "{name} se sacude y se aleja sin seguirte. Victorias que faltan: {wins}." },
};
} // namespace

DuelNarration DuelNarrationFor(DuelNarrationId id, bool animal)
{
    const int index = ((int)id >= 0 && (int)id < DuelNarration_Count) ? (int)id
                                                                        : (int)DuelNarration_Challenged;
    return animal ? kAnimal[index] : kPerson[index];
}

bool DuelRaisesHeadNotice(bool animal)
{
    return !animal;
}
} // namespace totalrecruit
