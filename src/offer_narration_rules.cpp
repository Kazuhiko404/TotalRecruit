#include "offer_narration_rules.h"

namespace totalrecruit
{
namespace
{
// One row per OfferNarrationId, in enum order. The person's approach and joined texts
// are the ones the event already shipped with (messages.json submissionEvent.*); the
// animal's are a narrator speaking about it. Plain ASCII, like messages.json.
const OfferNarration kPerson[OfferNarration_Count] = {
    { "submissionEvent.approachNotice", "{name} se acerca para hablar contigo." },
    { "submissionEvent.calmNotice",     "{name} no quiere unirse, pero deja de pelear por {seconds} s." },
    { "submissionEvent.ignoredNotice",  "{name}: lo ignoraste y se aleja." },
    { "submissionEvent.joinedNotice",   "{name}: se une a ti." },
};

const OfferNarration kAnimal[OfferNarration_Count] = {
    { "submissionEvent.animal.approachNotice", "{name} se acerca con la cabeza gacha." },
    { "submissionEvent.animal.calmNotice",     "{name} se calma y deja de atacarte por {seconds} s." },
    { "submissionEvent.animal.ignoredNotice",  "{name} se aleja trotando." },
    { "submissionEvent.animal.joinedNotice",   "{name} te sigue: ahora es parte de tu grupo." },
};
} // namespace

OfferNarration OfferNarrationFor(OfferNarrationId id, bool animal)
{
    const int index = ((int)id >= 0 && (int)id < OfferNarration_Count)
                          ? (int)id
                          : (int)OfferNarration_ApproachNotice;
    return animal ? kAnimal[index] : kPerson[index];
}

bool OfferSpeaks(bool animal)
{
    return !animal;
}
} // namespace totalrecruit
