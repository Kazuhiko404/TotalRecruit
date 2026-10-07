#include "common.h"
#include "npc_speech.h"
#include "engine_reads.h"
#include "character_facts.h"
#include "reason_catalog.h"
#include "messages.h"
#include "opening_line_rules.h"

#include <sstream>

// Player-facing text is Spanish to match the game it is built against.
// Non-ASCII is written as UTF-8 hex escapes because the v100 toolset reads this
// source as Latin-1, so an accented character typed directly arrives mangled.

namespace totalrecruit
{
namespace
{
// Who asks for cats, from the engine's npc type (opening_line_rules.h).
OpeningSpeaker SpeakerFor(int npcType)
{
    switch (npcType)
    {
    case OT_BANDIT: return OpeningSpeaker_Bandit;
    case OT_TRADER: return OpeningSpeaker_Trader;
    case OT_SLAVER: return OpeningSpeaker_Slaver;
    default:        return OpeningSpeaker_Other;
    }
}
} // namespace

// The NPC's own voice. Archetype decides what they care about, personality
// decides how they say it -- so a maniacal bandit and a fearful farmer never
// open the same way, without the mod author writing a branch for each pairing.
std::string SpeakOpeningLine(Character* character, const Verdict& verdict)
{
    // A beak thing does not negotiate in Spanish. It gets a sound rather than a
    // sentence, and blanking these keys in messages.json silences it entirely --
    // which is the other reasonable answer.
    if (verdict.isAnimal)
    {
        if (verdict.viaRescue)
        {
            return Say("animal.rescued", "*gime bajo y no te quita los ojos de encima*",
                       character);
        }
        if (verdict.currency == Currency_Care)
        {
            return Say("animal.hungry", "*olfatea tus manos, buscando comida*", character);
        }
        if (verdict.currency == Currency_Refused)
        {
            return Say("animal.refused", "*gru\xc3\xb1" "e y retrocede*", character);
        }
        return Say("animal.calm", "*te observa, quieto*", character);
    }

    Traits traits;
    OpeningFacts facts;
    facts.traitsRead = character != NULL && SafeReadTraits(character, &traits);
    facts.refused = verdict.currency == Currency_Refused;
    facts.tone = RefusalToneFor(verdict.reasonKey);
    facts.viaRescue = verdict.viaRescue;
    facts.currency = verdict.currency;
    facts.willJoin = verdict.willJoin;
    facts.temperament = traits.temperament;
    facts.speaker = SpeakerFor(traits.npcType);

    if (facts.traitsRead && facts.refused && !verdict.band.empty())
    {
        // A refusal that comes from the relationship should sound like the
        // relationship. An ally who thinks you are too green and a zealot who
        // hates your banner are not saying the same thing.
        const std::string keyed = Say("faction." + verdict.band + ".greeting", "", character);
        if (!keyed.empty())
        {
            return keyed;
        }
    }

    // The tone of a refusal comes from WHY they refused (the reason's key), not
    // from words found in the finished text (reason_catalog.cpp). Every line is a
    // messages.json key (refusal.*, currency.*) with the line it always said as
    // the fallback (opening_line_rules.h).
    const OpeningLine line = OpeningLineFor(facts);
    if (line.key[0] == '\0')
    {
        return std::string();
    }
    return Say(line.key, line.fallback, character);
}

namespace
{
// One popup key, falling back to the built-in text (offer_view.cpp) when
// messages.json does not have it.
void Word(std::string* slot, const char* key)
{
    *slot = Say(key, *slot, NULL);
}
} // namespace

// The popup's words: the messages.json "popup" block (route tags reuse the
// readout's), each defaulting to DefaultOfferTexts. Read on every open, so an
// edited file is picked up with the next reload of messages.json.
OfferTexts OfferWording()
{
    OfferTexts t = DefaultOfferTexts();
    Word(&t.title, "popup.title");

    Word(&t.accept, "popup.button.accept");
    Word(&t.cancel, "popup.button.cancel");
    Word(&t.close, "popup.button.close");
    Word(&t.notNow, "popup.button.notNow");
    Word(&t.feed, "popup.button.feed");
    Word(&t.feedMore, "popup.button.feedMore");
    Word(&t.beat, "popup.button.cageBeat");
    Word(&t.knockout, "popup.button.cageKnockout");
    Word(&t.restWait.hours, "cage.button.restHours");
    Word(&t.restWait.minutes, "cage.button.restMinutes");
    Word(&t.ignore, "popup.button.ignore");
    Word(&t.decline, "popup.button.decline");
    Word(&t.routeHelper, "popup.route.helper");

    Word(&t.routeFree, "readout.route.free");
    Word(&t.routeCaptivity, "readout.route.captivity");
    Word(&t.routeRescue, "readout.route.rescue");
    Word(&t.routeSurrender, "readout.route.surrender");
    Word(&t.routeFieldYield, "readout.route.fieldYield");
    Word(&t.routeMercy, "readout.route.mercy");
    Word(&t.routeAnimal, "readout.route.animal");

    Word(&t.priceLabel, "popup.info.price");
    Word(&t.purseLabel, "popup.info.purse");
    Word(&t.chanceLabel, "popup.info.chance");
    Word(&t.caredForLabel, "popup.info.caredFor");
    Word(&t.free, "popup.info.free");
    Word(&t.catsValue, "popup.info.cats");
    Word(&t.shortPurse, "popup.shortPurse");

    Word(&t.bodyRefused, "popup.body.refused");
    Word(&t.bodyCats, "popup.body.cats");
    Word(&t.bodyFreedom, "popup.body.freedom");
    Word(&t.bodySubmission, "popup.body.submission");
    Word(&t.bodyCare, "popup.body.care");
    Word(&t.bodyRescue, "popup.body.rescue");
    Word(&t.bodyJoins, "popup.body.joins");
    Word(&t.bodySubmitted, "popup.body.submitted");
    Word(&t.bodyAnimalSubmitted, "popup.body.animalSubmitted");
    Word(&t.bodyHelper, "popup.body.helper");
    Word(&t.reasonDefiantNoBeat, "popup.reason.defiantNoBeat");

    Word(&t.allyFreeBody, "popup.ally.free");
    Word(&t.allyPaidBody, "popup.ally.paid");
    Word(&t.allyPaidFullBody, "popup.ally.paidFull");
    Word(&t.allyTemporaryBody, "popup.ally.temporary");
    Word(&t.discountTag, "popup.ally.discountTag");

    Word(&t.duelChallenge, "popup.button.duel");
    Word(&t.duelRecordLabel, "popup.info.duels");
    Word(&t.duelJoinLabel, "popup.info.duelJoin");
    Word(&t.duelJoinChance, "popup.info.duelJoinChance");
    Word(&t.duelJoinSure, "popup.info.duelJoinSure");
    Word(&t.duelJoinNeedWins, "popup.info.duelJoinNeedWins");
    Word(&t.duelJoinNeedWin, "popup.info.duelJoinNeedWin");

    Word(&t.haggleLabel, "popup.haggle.label");
    Word(&t.haggleOdds, "popup.haggle.odds");
    Word(&t.haggleOffer, "popup.haggle.offer");
    Word(&t.haggleOffended, "popup.haggle.offended");
    Word(&t.haggleShort, "popup.haggle.short");
    return t;
}

// Several lines per personality so a prisoner shouting twice does not repeat
// himself word for word. Kept short: this is a bubble the player reads in
// passing, not a speech.
std::string HailLine(Character* character)
{
    Traits traits;
    if (character == NULL || !SafeReadTraits(character, &traits))
    {
        return "\xc2\xa1" "Eh! Hablemos.";
    }

    // Cheap rotation off the pointer, so the same NPC keeps his own flavour of
    // line while different ones in a row do not all say the same thing.
    const unsigned int pick =
        (unsigned int)((reinterpret_cast<size_t>(character) >> 4) % 3);

    switch (traits.personality)
    {
    case PT_FEARFUL:
        if (pick == 0) return "\xc2\xa1" "Espera! Podemos hablar, \xc2\xbfs\xc3\xad?";
        if (pick == 1) return "No me dejes aqu\xc3\xad... escucha un momento.";
        return "Por favor. Solo escucha lo que tengo que decir.";

    case PT_HONORABLE:
        if (pick == 0) return "Est\xc3\xa1 bien. Te escucho.";
        if (pick == 1) return "Has ganado. Habla y te oir\xc3\xa9.";
        return "Ya no tengo nada que probar. Di lo tuyo.";

    case PT_MANIACAL:
        if (pick == 0) return "\xc2\xa1" "Ja! Ven aqu\xc3\xad, quiero hablar contigo.";
        if (pick == 1) return "\xc2\xa1" "Eh, t\xc3\xba! S\xc3\xad, t\xc3\xba. Acerca" "te.";
        return "Esto se puso interesante. Hablemos.";

    case PT_COLD_CRUEL:
        if (pick == 0) return "Tenemos algo que discutir. Ven.";
        if (pick == 1) return "Acerca" "te. No voy a gritarlo.";
        return "Hablemos, antes de que cambie de idea.";

    case PT_BRAVE:
        if (pick == 0) return "\xc2\xa1" "Eh! Ven, quiero hablar.";
        if (pick == 1) return "No me voy a quedar callado. Hablemos.";
        return "Ven aqu\xc3\xad. Tengo algo que proponerte.";

    default:
        if (pick == 0) return "\xc2\xa1" "Eh! Hablemos.";
        if (pick == 1) return "Oye... ven un momento.";
        return "\xc2\xbf" "Tienes un momento? Quiero hablar.";
    }
}

std::string SpeakAcceptLine(const Verdict& verdict)
{
    // Short enough to fit the button. What the player is agreeing to is spelled
    // out in the window's body, so the button only has to say yes.
    return "ACEPTAR";
}

} // namespace totalrecruit
