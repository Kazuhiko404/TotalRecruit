#include "common.h"
#include "recruit_core.h"
#include "config.h"
#include "log.h"
#include "rules.h"
#include "insistence.h"
#include "insistence_rules.h"
#include "rescue_memory.h"
#include "pacify.h"
#include "npc_speech.h"
#include "char_key_reader.h"
#include "engine_reads.h"
#include "dialogue_bridge.h"
#include "dialogue_authoring.h"
#include "offer_popup.h"
#include "feeding.h"
#include "messages.h"
#include "defeats.h"
#include "cage_commands.h"
#include "cats_purse.h"
#include "money_rules.h"
#include "character_facts.h"
#include "haggle_rules.h"
#include "haggle_ledger.h"
#include "temp_member.h"
#include "duel_challenge_rules.h"
#include "duel_join_rules.h"
#include "duel_narration_rules.h"
#include "cheat_waits.h"
#include "npc_level.h"
#include "ts_bridge.h"
#include "head_notice.h"
#include "npc_motion.h"
#include "approach_rules.h"
#include "offer_requests.h"

#include <algorithm>

// Building/UseableStuff.h cannot be included here: it redefines
// BuildingDesignation against Platoon.h. Only one method is needed, so it is
// declared at global scope -- putting it inside a namespace would mangle the
// symbol differently and fail to link against KenshiLib.lib.
class UseableStuff
{
public:
    const hand& getOccupant() const;
};

// UseableStuff::getOccupant is non-virtual and exported, so it can be declared
// and called directly. Whether the pointer really IS a UseableStuff is decided
// by reading Building::specialFunction rather than by a virtual call.
//
// An earlier version declared Building::getUseableStuff as virtual here. That
// built a vtable of this plugin's own and placed the method at slot 0, while
// the engine keeps it at offset 0x300 -- so the call jumped to whatever
// function actually lived at slot 0, which destroyed the cage and crashed the
// game. Nothing in this file may declare a virtual belonging to an engine type.

namespace totalrecruit
{
namespace
{
std::string ToLower(const std::string& text)
{
    std::string out(text);
    std::transform(out.begin(), out.end(), out.begin(), ::tolower);
    return out;
}

bool ListContains(const std::vector<std::string>& list, const std::string& value)
{
    if (list.empty() || value.empty())
    {
        return false;
    }

    const std::string needle = ToLower(value);
    for (size_t i = 0; i < list.size(); ++i)
    {
        if (ToLower(list[i]) == needle)
        {
            return true;
        }
    }
    return false;
}

// Faction::getName returns a reference, so only a pointer crosses the guard.
bool SafeFactionPtr(Character* character, Faction** out)
{
    TR_TRY
    {
        *out = character->getFaction();
        return (*out != NULL);
    }
    TR_CATCH { return false; }
}

bool SafeFactionNamePtr(Faction* faction, const char** out)
{
    TR_TRY
    {
        *out = faction->getName().c_str();
        return true;
    }
    TR_CATCH { return false; }
}

// Both guards below sit in their own POD-only frames, because C2712 counts
// every object in the function, not only those inside the __try block.
bool SafeReadPlayerState(Faction** outFaction, ActivePlatoon** outSquad)
{
    TR_TRY
    {
        if (ou == NULL || ou->player == NULL)
        {
            return false;
        }

        *outFaction = ou->player->participant;
        Platoon* platoon = ou->player->currentPlatoon;
        *outSquad = (platoon != NULL) ? platoon->activePlatoon : NULL;
        return true;
    }
    TR_CATCH { return false; }
}

bool SafeSetFaction(Character* character, Faction* faction, ActivePlatoon* squad)
{
    TR_TRY
    {
        character->setFaction(faction, squad);
        return true;
    }
    TR_CATCH { return false; }
}

// The engine's own recruit path, as used by vanilla dialogue.
bool SafeEngineRecruit(Character* character)
{
    TR_TRY
    {
        if (ou == NULL || ou->player == NULL)
        {
            return false;
        }
        return ou->player->recruit(character, false);
    }
    TR_CATCH { return false; }
}

// getHandle returns a reference, so the call sits in its own POD-only frame.
void RefreshPortraits(Character* character)
{
    ou->addPortraitUpdate(character->getHandle());
}

void SafeRefreshPortraits(Character* character)
{
    TR_TRY
    {
        if (ou != NULL)
        {
            RefreshPortraits(character);
        }
    }
    TR_CATCH { }
}

// showPlayerAMessage takes a std::string, so the temporary is built in a plain
// function and only the call itself is guarded.
void PostMessage(const char* text, bool queued)
{
    ou->showPlayerAMessage(std::string(text), queued);
    ou->playNotification(NULL);
}

void ShowMessage(const char* text, bool isProblem)
{
    TR_TRY { PostMessage(text, isProblem); }
    TR_CATCH { }
}

std::string IntToString(int value)
{
    char buffer[32] = { 0 };
    _snprintf_s(buffer, sizeof(buffer), _TRUNCATE, "%d", value);
    return std::string(buffer);
}

// What this verdict actually costs in cats. Verdict::cats is also used by the
// animal route to carry "percent of stomach missing" for its message, so it is
// only money when the currency says it is.
int CatsDue(const Verdict& verdict)
{
    return (verdict.currency == Currency_Cats && verdict.cats > 0) ? verdict.cats : 0;
}

// The player's purse is the squad leader's. All the arithmetic -- whether the
// funds are there, all-or-nothing, the refund -- is money_rules.cpp.
ChargeResult ChargePlayer(int amount)
{
    Character* leader = PlayerPurseOwner();
    if (leader == NULL)
    {
        return Charge_Failed;
    }
    CharacterPurse purse(leader);
    return ChargePrice(purse, amount);
}

// Gives back a charge whose purchase did not happen.
void RefundPlayer(int amount)
{
    Character* leader = PlayerPurseOwner();
    if (leader == NULL)
    {
        LogError("Could not refund " + IntToString(amount) + " cats: no player purse.");
        return;
    }
    CharacterPurse purse(leader);
    if (!RefundCharge(purse, amount))
    {
        LogError("Refund of " + IntToString(amount) + " cats was refused.");
    }
}

// Building::specialFunction, read at its documented offset rather than through
// getSpecialFunction() -- that is virtual, and this plugin cannot safely
// declare an engine virtual (see the note above the class declarations).
const size_t kSpecialFunctionOffset = 0x158;

bool LooksLikeCage(RootObject* target)
{
    const int function = *reinterpret_cast<const int*>(
        reinterpret_cast<const char*>(target) + kSpecialFunctionOffset);
    return function == BF_CAGE;
}

// getOccupant returns a hand reference, and the compiler counts that as an
// object in whatever frame touches it, so the read lives in a plain function
// and only the call crosses the guard.
void ReadOccupant(RootObject* target, Character** out)
{
    if (!LooksLikeCage(target))
    {
        return;
    }

    UseableStuff* stuff = reinterpret_cast<UseableStuff*>(target);
    *out = stuff->getOccupant().getCharacter();
}

Character* SafeReadOccupant(RootObject* target)
{
    Character* occupant = NULL;
    TR_TRY { ReadOccupant(target, &occupant); }
    TR_CATCH { return NULL; }
    return occupant;
}

bool SafeDataType(RootObject* target, itemType* out)
{
    TR_TRY
    {
        *out = target->getDataType();
        return true;
    }
    TR_CATCH { return false; }
}

// Cages are tagged BUILDING; UseableStuff derives from Building and has no
// itemType of its own.
Character* OccupantOf(RootObject* target, itemType type)
{
    if (type != BUILDING || !GetConfig().resolveCageOccupant)
    {
        return NULL;
    }
    return SafeReadOccupant(target);
}

// The remedy button: food, when they are hungry and you are carrying some.
// HelpCaptive dispatches on the same test, so the label and the action cannot
// disagree. The beating is not a remedy any more: the popup offers the cage
// commands by their own rule (offer_view.cpp ApplyCageCommands, fed by
// CageInputFor below).
OfferRemedy RemedyFor(const Verdict& verdict, bool again)
{
    if (verdict.willJoin)
    {
        return OfferRemedy_None;
    }
    if (verdict.currency == Currency_Care && verdict.wantsFood && PlayerHasFood())
    {
        return again ? OfferRemedy_FeedMore : OfferRemedy_Feed;
    }
    return OfferRemedy_None;
}

// What the popup needs to decide GOLPEAR / NOQUEAR: the context menu's facts and,
// only when they apply, the engine's knockout odds for NOQUEAR's label.
void CageInputFor(Character* character, OfferInput* in)
{
    in->cage = ReadCageCommandFacts(character);
    if (CageCommandsAllowed(in->cage))
    {
        in->knockoutOddsKnown = ReadKnockoutOdds(character, &in->knockoutOdds);
    }
}

// A cats quote: the only kind of verdict a haggle can be about (a yes with a
// price, or the "not yet" that is only the purse).
bool IsCatsQuote(const Verdict& verdict)
{
    return verdict.currency == Currency_Cats && verdict.cats > 0 &&
           (verdict.willJoin || verdict.reasonKey == Reason_CatsAsk);
}

// What the popup's haggle section needs (offer_view.h OfferHaggleInput): off
// unless haggling is on, the verdict is a cats quote and the engine gave a roll
// key and a clock to seed the attempt with. `outTerms` (optional) receives the
// engine reads, so the attempt draws on the very key and day shown.
OfferHaggleInput HaggleInputFor(Character* character, const Verdict& verdict, HaggleTerms* outTerms)
{
    const Prices& prices = GetConfig().prices;
    OfferHaggleInput haggle;
    haggle.prices = prices;
    if (!prices.haggleEnabled || !IsCatsQuote(verdict))
    {
        return haggle;
    }
    const HaggleTerms terms = ReadHaggleTerms(character);
    if (outTerms != NULL)
    {
        *outTerms = terms;
    }
    if (!terms.roll.haveKey)
    {
        return haggle;
    }
    haggle.enabled = true;
    haggle.locked = terms.locked;
    haggle.leverage = terms.speakerKnown
                          ? HaggleLeverage(terms.speakerBestStat, terms.npcLevel, prices)
                          : 0;
    haggle.personality = terms.temperament;
    return haggle;
}

// RETAR A DUELO, the record row and SI LE GANAS (offer_view.h OfferDuelInput): offered
// by DuelOfferedFor; the record is read once, now, as the window opens. What the next
// win does needs the record in Balanced (the chance grows with the wins); Cheat is sure.
OfferDuelInput DuelInputFor(Character* character, const Verdict& verdict)
{
    OfferDuelInput duel;
    duel.offered = DuelOfferedFor(character, verdict);
    if (!duel.offered)
    {
        return duel;
    }
    Character* challenger = SelectedSquadActor(character);
    if (challenger != NULL)
    {
        duel.recordKnown = ReadDuelRecord(challenger, character, &duel.wins, &duel.losses,
                                          &duel.draws);
    }
    const DuelJoinOdds odds = DuelNextWinOddsNow(duel.recordKnown ? duel.wins : 0);
    duel.joinKnown = odds.sure || odds.winsNeeded > 0 || duel.recordKnown;
    duel.joinSure = odds.sure;
    duel.joinChance = odds.chance;
    duel.joinWinsNeeded = odds.winsNeeded;
    return duel;
}

// What the player is carrying: the squad leader's purse (cats_purse.h).
bool ReadPlayerPurse(int* out)
{
    Character* leader = PlayerPurseOwner();
    if (leader == NULL)
    {
        return false;
    }
    CharacterPurse purse(leader);
    return purse.Balance(out);
}

// Opens the popup for a verdict, whatever it says. A yes carries its price and
// the purse it is checked against; a price the purse cannot meet and every
// refusal carry their reason in red with ACEPTAR hidden; a refusal the player
// can act on carries its remedy (offer_view.cpp decides all of that). Nothing
// ends in a notification: the popup is where the player reads the answer.
bool PresentOffer(Character* character, const Verdict& verdict, bool afterRemedy)
{
    OfferInput in;
    in.name = CharacterName(character);
    in.verdict = verdict;
    in.purseKnown = ReadPlayerPurse(&in.purse);
    in.remedy = RemedyFor(verdict, afterRemedy);
    in.afterRemedy = afterRemedy;
    in.haggle = HaggleInputFor(character, verdict, NULL);
    CageInputFor(character, &in);
    in.duel = DuelInputFor(character, verdict);
    return ShowPricedOffer(character, in, OfferWording());
}

// Pays `cats` and recruits, putting the money back when the engine refuses the
// recruit. `joined` is the line the player reads on success. True when they joined.
bool PayAndRecruit(Character* character, int cats, const std::string& name,
                   const std::string& joined)
{
    // Money only moves here. Declining, or walking away, costs nothing.
    if (!SettlePrice(character, cats, name))
    {
        return false;
    }

    std::string message;
    if (Recruit(character, &message))
    {
        TellPlayer(joined, false);
        return true;
    }

    // Paid but not delivered: the engine refused the recruit. Put the money back.
    RefundPrice(character, cats, name);
    TellPlayer(name + ": " + message, true);
    return false;
}
} // namespace

Character* CharacterFromTarget(RootObject* target)
{
    if (target == NULL)
    {
        return NULL;
    }

    // The type read is the only thing guarded here; the logging below builds
    // std::strings, and C2712 forbids those in any frame containing a __try.
    itemType type = NULL_ITEM;
    if (!SafeDataType(target, &type))
    {
        return NULL;
    }

    if (type == CHARACTER || type == HUMAN_CHARACTER ||
        type == ANIMAL_CHARACTER || type == WORLDMAP_CHARACTER)
    {
        return static_cast<Character*>(target);
    }

    // Right-clicking a cage resolves to the cage, not the person inside it --
    // the two overlap on screen and the building usually wins. Asking the cage
    // who is in it turns that click into the one the player meant.
    Character* occupant = OccupantOf(target, type);
    if (occupant != NULL)
    {
        LogInfo("Target is a container; using its occupant instead.");
        return occupant;
    }

    if (GetConfig().probeOnClick)
    {
        LogInfo("Right-click target is not a character (itemType " +
                IntToString((int)type) + ") and holds nobody.");
    }
    return NULL;
}

bool IsRecruitable(Character* character, std::string* outReason)
{
    if (character == NULL)
    {
        if (outReason) *outReason = "no character";
        return false;
    }

    const Config& config = GetConfig();
    const Exclusions& exclude = config.exclude;

    bool flag = false;
    if (exclude.sameFaction && SafeIsPlayerCharacter(character, &flag) && flag)
    {
        if (outReason) *outReason = "already in your faction";
        return false;
    }
    if (exclude.dead && SafeIsDead(character, &flag) && flag)
    {
        if (outReason) *outReason = "excluded: dead";
        return false;
    }
    if (exclude.unconscious && SafeIsDown(character, &flag) && flag)
    {
        if (outReason) *outReason = "excluded: unconscious";
        return false;
    }
    if (exclude.animals && SafeIsAnimal(character, &flag) && flag)
    {
        if (outReason) *outReason = "excluded: animal";
        return false;
    }

    if (!exclude.characterNames.empty() &&
        ListContains(exclude.characterNames, CharacterName(character)))
    {
        if (outReason) *outReason = "excluded: character name";
        return false;
    }

    if (!exclude.factions.empty())
    {
        Faction* faction = NULL;
        const char* factionName = NULL;
        if (SafeFactionPtr(character, &faction) &&
            SafeFactionNamePtr(faction, &factionName) &&
            factionName != NULL &&
            ListContains(exclude.factions, factionName))
        {
            if (outReason) *outReason = "excluded: faction";
            return false;
        }
    }

    return true;
}

bool Recruit(Character* character, std::string* outMessage)
{
    const Config& config = GetConfig();
    if (!config.enabled)
    {
        if (outMessage) *outMessage = "mod disabled";
        return false;
    }

    std::string reason;
    if (!IsRecruitable(character, &reason))
    {
        if (outMessage) *outMessage = reason;
        return false;
    }

    const std::string name = CharacterName(character);

    Faction* playerFaction = NULL;
    ActivePlatoon* squad = NULL;
    if (!SafeReadPlayerState(&playerFaction, &squad))
    {
        if (outMessage) *outMessage = "could not read player state";
        return false;
    }

    if (playerFaction == NULL)
    {
        if (outMessage) *outMessage = "no player faction";
        return false;
    }

    // PlayerInterface::recruit is the same function the vanilla "join your
    // squad" dialogue calls, so it does the platoon bookkeeping the engine
    // expects rather than just flipping a faction pointer. setFaction stays as
    // the fallback for the cases it refuses.
    if (!SafeEngineRecruit(character))
    {
        if (!SafeSetFaction(character, playerFaction, squad))
        {
            LogError("Both recruit paths failed for " + name);
            if (outMessage) *outMessage = "engine refused the recruit";
            return false;
        }
        LogInfo("Recruited " + name + " via setFaction fallback.");
    }

    // SafePacify resets combat mode, clears temporary enemy status with the player,
    // and forces the character's AI package to rethink its action so they do not
    // re-attack immediately.
    Character* leader = NULL;
    if (ou != NULL && ou->player != NULL && ou->player->currentPlatoon != NULL)
    {
        leader = ou->player->currentPlatoon->getSquadLeader();
    }
    SafePacify(character, leader);

    // Without this the new member can be missing from the portrait bar until
    // something else forces a refresh.
    SafeRefreshPortraits(character);

    LogInfo("Recruited " + name + ".");
    if (outMessage) *outMessage = name;
    return true;
}

void TellPlayer(const std::string& message, bool isProblem)
{
    ShowMessage(message.c_str(), isProblem);
}

void AttemptRecruit(Character* character)
{
    if (character == NULL)
    {
        return;
    }

    const Config& config = GetConfig();
    if (!config.enabled)
    {
        return;
    }

    const std::string name = CharacterName(character);

    // proactive-offer: the player is talking by hand, so a pending offer request is
    // used up (no second window walking up afterwards).
    ConsumeRequestedOffer(character, "the player talked by hand");

    // Config exclusions come first: they are the player's own opt-outs, and
    // they outrank anything the rules would say.
    std::string excluded;
    if (!IsRecruitable(character, &excluded))
    {
        TellPlayer(name + ": " + excluded, true);
        return;
    }

    const Verdict verdict = Evaluate(character);

    // R2.4 -- insistence. Only a real no counts (a failed roll, a personality
    // refusal), and never from an animal: a state ("heal them first", out cold)
    // or an instruction is what the player is busy acting on, not pestering
    // (insistence_rules.h). Recorded after the verdict, so the talk that fills the
    // counter still hears its own reason and the next one hears "harto".
    if (ou != NULL && CountsAsInsistence(verdict, verdict.isAnimal))
    {
        double nowHours = ou->getTimeStamp_inGameHours().getTotalHours();
        RecordInteraction(character, nowHours);
    }

    // A refusal is not the end of the interaction -- it is the moment the
    // player most needs somewhere to act. So it goes through the same popup as
    // an acceptance, with its reason and whatever remedy applies.
    if (!verdict.willJoin)
    {
        LogInfo("Refused " + name + " (" + CurrencyName(verdict.currency) + "): " +
                verdict.reason);
    }

    // A rescued NPC who cannot follow pays what they carry. The verdict only
    // said so in words until now: owesGift was set and nothing ever read it,
    // so the mod promised a gift and handed over nothing. Settled once and
    // remembered in the save, so reopening the window is not a second payday.
    if (verdict.owesGift)
    {
        const int gift = PayPartingGift(character);
        if (gift > 0)
        {
            TellPlayer(Fill(Say("rescue.gift", "{name}: te deja {cats} cats", character),
                            name, gift, 0),
                       false);
        }
    }

    // The NPC states their terms out loud, where they stand. The window that
    // follows only carries the decision.
    const std::string spoken = SpeakOpeningLine(character, verdict);
    if (!spoken.empty())
    {
        SpeakAloud(character, spoken);
    }

    // An FCS dialogue is used when the author supplied one, because the native
    // conversation window is richer than anything built here. Otherwise the
    // plugin's own offer window does the job, which is what keeps this mod
    // working on NPCs no FCS file has ever heard of.
    const std::string& dialogueId = verdict.viaRescue ? config.dialogue.rescue
                                                      : config.dialogue.captive;
    if (OpenConversation(character, dialogueId))
    {
        RememberPendingPrice(character, CatsDue(verdict));
        return;
    }

    // Never recruit without the popup: a yes nobody confirmed is a squad
    // change the player never agreed to.
    if (!PresentOffer(character, verdict, false))
    {
        LogError("Offer window would not open for " + name + "; nothing was decided.");
    }
}

void HelpCaptive(Character* character)
{
    if (character == NULL)
    {
        return;
    }

    const std::string name = CharacterName(character);

    // The remedy button is only ever food; the dispatch re-checks the same test
    // PresentOffer labelled it with, so a stale window cannot feed for nothing.
    const Verdict before = Evaluate(character);
    if (RemedyFor(before, false) != OfferRemedy_Feed)
    {
        return;
    }

    std::string message;
    if (!FeedCaptive(character, &message))
    {
        TellPlayer(name + ": " + message, true);
        return;
    }
    TellPlayer(name + ": " + message, false);

    // Show where that leaves them, so the player learns whether one meal was
    // enough or there is more to do.
    const Verdict after = Evaluate(character);
    const std::string spoken = SpeakOpeningLine(character, after);
    if (!spoken.empty())
    {
        SpeakAloud(character, spoken);
    }

    PresentOffer(character, after, true);
}

void CompleteOffer(Character* character, bool accepted)
{
    if (character == NULL)
    {
        return;
    }

    const std::string name = CharacterName(character);

    // A declined offer says nothing: the offer popup already shows the outcome.
    if (!accepted)
    {
        return;
    }

    // Re-evaluated rather than trusting the verdict from when the window
    // opened: the world can change while it is on screen, and the price should
    // reflect the moment the player actually agreed.
    const Verdict verdict = Evaluate(character);
    if (!verdict.willJoin)
    {
        TellPlayer(name + ": " + verdict.reason, true);
        return;
    }

    // An 80+ ally comes for a few days only, and goes back to the faction they came
    // from: read now, while the recruit has not changed it yet.
    const std::string homeFaction = verdict.temporary ? FactionStringId(character) : std::string();
    const bool joined = PayAndRecruit(character, CatsDue(verdict), name,
                                      name + ": " + verdict.reason);
    if (joined && verdict.temporary)
    {
        OnRecruitedTemporarily(character, verdict.temporaryDays, homeFaction);
    }
}

void CompleteHaggle(Character* character, int offer)
{
    if (character == NULL)
    {
        return;
    }

    const std::string name = CharacterName(character);

    // Re-evaluated, like CompleteOffer: the attempt is about the price asked now.
    const Verdict verdict = Evaluate(character);
    if (!IsCatsQuote(verdict))
    {
        TellPlayer(name + ": " + verdict.reason, true);
        return;
    }

    // The same key and day the join roll uses (MintRollKey), read at this moment.
    HaggleTerms terms;
    const OfferHaggleInput haggle = HaggleInputFor(character, verdict, &terms);
    if (!haggle.enabled || haggle.locked)
    {
        // Nothing to haggle any more (offended meanwhile, or no seed): show the
        // window as it stands now instead of drawing on nothing.
        PresentOffer(character, verdict, false);
        return;
    }

    const int ask = verdict.cats;
    const int held = ClampOffer(offer, HaggleSliderBounds(ask, haggle.prices));
    if (held >= ask)
    {
        // The ask itself is not a haggle.
        CompleteOffer(character, true);
        return;
    }

    const int odds = AcceptOdds(ask, held, haggle.leverage, haggle.personality, haggle.prices);
    const HaggleOutcome outcome = ResolveHaggle(odds, terms.roll.key, terms.roll.dayIndex);
    if (GetConfig().probeOnClick)
    {
        LogInfo("[haggle] " + name + " ask=" + IntToString(ask) + " offer=" + IntToString(held) +
                " odds=" + IntToString(odds) + "% -> " +
                (outcome == Haggle_Accepted ? "accepted" : "offended"));
    }

    if (outcome == Haggle_Accepted)
    {
        // The offer is the price: SettlePrice re-checks the purse against it and
        // money.paid names it.
        PayAndRecruit(character, held, name,
                      Fill(Say("haggle.accepted", "{name}: acepta {cats} cats y se une a tu grupo",
                               character),
                           name, held, 0));
        return;
    }

    // Offended: no more haggling today, and the ask goes up (ApplyCatsOffer reads
    // the ledger), so the window reopens locked at the raised price.
    SessionHaggleLedger().RecordOffence(terms.roll.key, terms.roll.dayIndex);
    TellPlayer(Fill(Say("haggle.offended", "{name}: se ofende con tu oferta de {cats} cats",
                        character),
                    name, held, 0),
               true);
    PresentOffer(character, Evaluate(character), false);
}

bool SettlePrice(Character* character, int cats, const std::string& name)
{
    switch (ChargePlayer(cats))
    {
    case Charge_Free:
        return true;

    case Charge_Paid:
        LogInfo("Charged " + IntToString(cats) + " cats for " + name + ".");
        TellPlayer(Fill(Say("money.paid", "{name}: pagaste {cats} cats", character),
                        name, cats, 0),
                   false);
        return true;

    case Charge_Short:
    case Charge_Failed:
    default:
        TellPlayer(Fill(Say("money.short", "{name}: no tienes suficientes fondos ({cats} cats)",
                            character),
                        name, cats, 0),
                   true);
        return false;
    }
}

void RefundPrice(Character* character, int cats, const std::string& name)
{
    if (cats <= 0)
    {
        return;
    }
    RefundPlayer(cats);
    LogInfo("Refunded " + IntToString(cats) + " cats: " + name + " could not be recruited.");
    TellPlayer(Fill(Say("money.refunded", "{name}: te devolvieron {cats} cats", character),
                    name, cats, 0),
               false);
}

// ------------------------------------------------------------ Duel challenge
namespace
{
// How far from any squad member the challenged NPC is looked for once the duel is
// over (the bout's own pointers died with TotalSparring's callback).
const float kDuelSearchMeters = 80.0f;
const int kDuelSearchMembers = 64;
// How often stale challenges are swept out.
const DWORD kDuelExpirySweepMs = 5000;
DWORD g_lastDuelSweep = 0;

// The NPC carrying `key`, near any of the player's characters. NULL when not found.
Character* FindNearSquad(const CharKey& key)
{
    Character* members[kDuelSearchMembers] = { NULL };
    const int count = SafePlayerCharacters(members, kDuelSearchMembers);
    for (int m = 0; m < count; ++m)
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        std::vector<Character*> nearby;
        if (members[m] == NULL || !NpcMotion::Position(members[m], &x, &y, &z) ||
            !CharactersNear(Ogre::Vector3(x, y, z), MetersToWorldUnits(kDuelSearchMeters), &nearby))
        {
            continue;
        }
        for (unsigned int i = 0; i < nearby.size(); ++i)
        {
            CharKey found;
            if (nearby[i] != NULL && SafeFindCharKey(nearby[i], &found) && found == key)
            {
                return nearby[i];
            }
        }
    }
    return NULL;
}

// The line for a window TotalSparring would not open (messages.json duel.*).
std::string DuelOpenProblem(TsOpenResult result, Character* npc, const std::string& name)
{
    switch (result)
    {
    case TsOpen_Busy:
        return Say("duel.busy", "Ya hay un duelo abierto en TotalSparring.", npc);
    case TsOpen_NotReady:
    case TsOpen_Absent:
        return Say("duel.notReady", "TotalSparring todavia no esta listo para duelos.", npc);
    case TsOpen_Refused:
        return Fill(Say("duel.refused", "TotalSparring no pudo abrir el duelo con {name}.", npc),
                    name, 0, 0);
    case TsOpen_Failed:
    case TsOpen_Opened:
    default:
        return Say("duel.failed", "No se pudo abrir el duelo de TotalSparring.", npc);
    }
}

const char* ResolutionWord(DuelResolution resolution)
{
    switch (resolution)
    {
    case DuelResolution_Won:    return "won";
    case DuelResolution_Lost:   return "lost";
    default:                    return "ignored";
    }
}

// The player's character carrying `key`, or NULL.
Character* FindSquadMember(const CharKey& key)
{
    Character* members[kDuelSearchMembers] = { NULL };
    const int count = SafePlayerCharacters(members, kDuelSearchMembers);
    for (int m = 0; m < count; ++m)
    {
        CharKey found;
        if (members[m] != NULL && SafeFindCharKey(members[m], &found) && found == key)
        {
            return members[m];
        }
    }
    return NULL;
}

// The winner's head-to-head wins against `npc` as TotalSparring has them, the bout
// that just ended included (it writes its history before it tells its listeners).
// The winner is side A; the challenger is the fallback. An unreadable record counts
// as the first win, so the chance is never higher than the player earned.
int WinsAgainst(const DuelResolveResult& result, Character* npc, std::string* winnerName)
{
    Character* winner = FindSquadMember(result.winner);
    if (winner == NULL)
    {
        winner = FindSquadMember(result.challenger);
    }
    if (winner != NULL)
    {
        *winnerName = CharacterName(winner);
    }
    int wins = 0;
    int losses = 0;
    int draws = 0;
    if (winner == NULL || !TsDuelStats(winner, npc, &wins, &losses, &draws))
    {
        LogError("[duel] the record against " + result.npc.ToString() +
                 " could not be read: counting this as the first win.");
        return 1;
    }
    return wins < 1 ? 1 : wins;
}

// An animal is told about by a narrator and raises no notice over its head
// (duel_narration_rules.h); an unreadable flag reads as a person, the old behaviour.
bool DuelNpcIsAnimal(Character* npc)
{
    bool animal = false;
    return npc != NULL && SafeIsAnimal(npc, &animal) && animal;
}

// The duel line for `id` in this NPC's form, from messages.json or its built-in text.
std::string DuelLine(DuelNarrationId id, Character* npc, bool animal)
{
    const DuelNarration line = DuelNarrationFor(id, animal);
    return Say(line.key, line.fallback, npc);
}

std::string WithNumber(const std::string& text, const std::string& key, int number)
{
    std::string out = text;
    const std::string value = IntToString(number);
    std::string::size_type at = 0;
    while ((at = out.find(key, at)) != std::string::npos)
    {
        out.replace(at, key.size(), value);
        at += value.size();
    }
    return out;
}

std::string WithChance(const std::string& text, int chance)
{
    return WithNumber(text, "{chance}", chance);
}

// duel-any-source U4: a win under the minimum wins (duel_join_rules.h) joins nobody and
// starts no cooldown; the rival says it was a good fight and the line counts the wins
// still missing. With the proactive offer on and the rival in reach, the offer event
// calms it like a win that stays.
void OnDuelWonNeedsMoreWins(Character* npc, const std::string& name, const DuelResolveResult& result,
                            const DuelWinDecision& decision, const std::string& head)
{
    LogInfo(head + "needs " + IntToString(decision.winsStillNeeded) + " more win(s) vs " + name +
            " [key " + result.npc.ToString() + "]");
    const bool animal = DuelNpcIsAnimal(npc);
    if (DuelRaisesHeadNotice(animal))
    {
        RaiseHeadNotice(npc, Say("duel.notice.needMoreWins", "Buena pelea. Ganame otra vez", npc),
                        HeadNoticeKind_Info);
    }
    TellPlayer(WithNumber(Fill(DuelLine(DuelNarration_NeedMoreWins, npc, animal), name, 0, 0), "{wins}",
                          decision.winsStillNeeded),
               false);
    if (GetConfig().offer.onDuelWin && WithinOfferReach(npc))
    {
        RequestOffer(npc, OfferTrigger_DuelWin, OfferVerdict_Calm);
    }
}

// A won duel that did not roll the join (Balanced): a good fight, come back later.
// The win cooldown goes into the duel cooldown book, saved with the NPC like a lost
// duel's.
void OnDuelWonStays(Character* npc, const std::string& name, const DuelResolveResult& result,
                    const DuelWinDecision& decision, double nowHours)
{
    SetDuelCooldownUntil(result.npc, decision.cooldownUntilHour);
    const double left = (double)decision.cooldownUntilHour - nowHours;
    const int hours = left > 0.0 ? (int)(left + 0.999) : 0;
    const bool animal = DuelNpcIsAnimal(npc);
    if (DuelRaisesHeadNotice(animal))
    {
        RaiseHeadNotice(npc, Say("duel.notice.stays", "Buena pelea. Retame otra vez", npc),
                        HeadNoticeKind_Info);
    }
    TellPlayer(WithChance(Fill(DuelLine(DuelNarration_Stays, npc, animal), name, 0, hours),
                          decision.chance),
               false);
}

// A won duel: Cheat joins for good; Balanced rolls the seeded join chance
// (duel_join_rules.h) and either joins for good -- a plain Recruit, never a temporary
// stay -- or starts the win cooldown.
// proactive-offer: a won duel asks the submission event for an offer instead of
// recruiting on the spot (offer_requests.h). True when it did: the rival walks up and
// opens the window (a passed roll), or stops attacking for a while (a failed one, after
// the usual win cooldown and line). Off (offerOnDuelWin) or out of the event's reach, the
// old behaviour runs.
bool RequestDuelOffer(Character* npc, const std::string& name, const DuelResolveResult& result,
                      const DuelWinDecision& decision, double nowHours, const std::string& head)
{
    if (!GetConfig().offer.onDuelWin)
    {
        return false;
    }
    if (!WithinOfferReach(npc))
    {
        LogInfo("[offer] " + name + " is out of the event's reach: the duel win resolves as before");
        return false;
    }
    if (!decision.joins)
    {
        LogInfo(head + "stays; the offer event calms it [key " + result.npc.ToString() + "]");
        OnDuelWonStays(npc, name, result, decision, nowHours);
        RequestOffer(npc, OfferTrigger_DuelWin, OfferVerdict_Calm);
        return true;
    }
    LogInfo(head + "the offer event brings " + name + " over [key " + result.npc.ToString() + "]");
    RequestOffer(npc, OfferTrigger_DuelWin, OfferVerdict_Join);
    return true;
}

void OnDuelWon(const DuelResolveResult& result, double nowHours)
{
    Character* npc = FindNearSquad(result.npc);
    if (npc == NULL)
    {
        LogError("[duel] won against " + result.npc.ToString() + ", who is not near the squad.");
        TellPlayer(Say("duel.missing", "Ganaste el duelo, pero tu rival ya no esta cerca para unirse.",
                       NULL),
                   true);
        return;
    }
    const std::string name = CharacterName(npc);
    bool dead = false;
    if (SafeIsDead(npc, &dead) && dead)
    {
        LogInfo("[duel] " + name + " died in the duel: nobody to recruit.");
        TellPlayer(Fill(Say("duel.died", "{name} murio en el duelo.", npc), name, 0, 0), false);
        return;
    }

    const Config& config = GetConfig();
    std::string winnerName = "?";
    DuelWinFacts facts;
    facts.cheat = config.preset == Preset_Cheat;
    facts.npc = result.npc;
    facts.wins = WinsAgainst(result, npc, &winnerName);
    facts.npcLevel = config.allyTiers.eliteFromLevel;
    SafeNpcLevel(npc, &facts.npcLevel);
    facts.nowHours = nowHours;
    const DuelWinDecision decision = DecideDuelWin(facts, config.duelChallenges);
    const std::string head = "[duel] " + winnerName + " won (wins " + IntToString(facts.wins) +
                             "): join chance " + IntToString(decision.chance) + "%, roll " +
                             IntToString(decision.roll) + " -> ";
    if (decision.winsStillNeeded > 0)
    {
        OnDuelWonNeedsMoreWins(npc, name, result, decision, head);
        return;
    }
    if (RequestDuelOffer(npc, name, result, decision, nowHours, head))
    {
        return;
    }
    if (!decision.joins)
    {
        char hours[32] = { 0 };
        _snprintf_s(hours, sizeof(hours), _TRUNCATE, "%.1f", decision.cooldownHours);
        LogInfo(head + "stays (cooldown " + hours + " h until hour " +
                IntToString(decision.cooldownUntilHour) + ") vs " + name + " level " +
                IntToString(facts.npcLevel) + " [key " + result.npc.ToString() + "]");
        OnDuelWonStays(npc, name, result, decision, nowHours);
        return;
    }
    LogInfo(head + "joined vs " + name + " [key " + result.npc.ToString() + "]");

    std::string message;
    if (!Recruit(npc, &message))
    {
        LogError("[duel] " + name + " lost the duel but the recruit failed: " + message);
        TellPlayer(name + ": " + message, true);
        return;
    }
    const bool animal = DuelNpcIsAnimal(npc);
    if (DuelRaisesHeadNotice(animal))
    {
        RaiseSquadHeadNotice(npc, Say("duel.notice.joined", "Me ganaste. Me uno a ti", npc),
                             HeadNoticeKind_Info);
    }
    TellPlayer(Fill(DuelLine(DuelNarration_Joined, npc, animal), name, 0, 0), false);
    LogInfo("[duel] " + name + " lost the duel and joins for good" + (animal ? " (animal)" : "") +
            " [key " + result.npc.ToString() + "]");
}

// A lost (or drawn) duel: they tell the player to come back stronger, and the
// cooldown (already in the book, saved with the NPC) keeps the button away.
void OnDuelLost(const DuelResolveResult& result, double nowHours)
{
    Character* npc = FindNearSquad(result.npc);
    const std::string name = npc != NULL ? CharacterName(npc) : std::string("Tu rival");
    const double left = (double)result.cooldownUntilHour - nowHours;
    const int hours = left > 0.0 ? (int)(left + 0.999) : 0;
    const bool animal = DuelNpcIsAnimal(npc);
    if (npc != NULL && DuelRaisesHeadNotice(animal))
    {
        RaiseHeadNotice(npc, Say("duel.notice.lost", "Vuelve cuando seas mas fuerte", npc),
                        HeadNoticeKind_Info);
    }
    TellPlayer(Fill(DuelLine(DuelNarration_Lost, npc, animal), name, 0, hours), false);
    LogInfo("[duel] " + name + " won the duel: no challenge until hour " +
            IntToString(result.cooldownUntilHour) + " [key " + result.npc.ToString() + "]");
}

void LogBout(const DuelBoutRecord& bout, DuelResolution resolution)
{
    if (!bout.isDuel && !GetConfig().probeOnClick)
    {
        return;
    }
    LogInfo(std::string("[duel] bout ") + (bout.isDuel ? "duel" : "spar") + " outcome=" +
            IntToString((int)bout.outcome) + " a=" + (bout.aKnown ? bout.a.ToString() : "?") +
            (bout.aIsSquadMember ? "(squad)" : "") + " b=" + (bout.bKnown ? bout.b.ToString() : "?") +
            " -> " + ResolutionWord(resolution));
}
} // namespace

namespace
{
// CanOfferDuel over the engine facts, given the two a verdict supplies: whether the
// Balanced ally tiers answered, whether it is an animal (a hint: the Cheat verdict
// never reads who they are), and the level to fall back on when SafeNpcLevel fails.
bool DuelOfferedWith(Character* npc, bool allyTiered, bool animalHint, int levelFallback)
{
    if (npc == NULL)
    {
        return false;
    }
    const Config& config = GetConfig();
    DuelOfferFacts facts;
    facts.enabled = config.duelChallenges.enabled;
    if (!facts.enabled)
    {
        return false;
    }
    facts.allyTiered = allyTiered;
    // Cheat has no ally tiers: the level alone decides, for any person.
    facts.cheatPreset = config.preset == Preset_Cheat;
    // The Cheat verdict never reads who they are, so it may not know an animal.
    facts.isAnimal = animalHint;
    bool animal = false;
    if (!facts.isAnimal && SafeIsAnimal(npc, &animal))
    {
        facts.isAnimal = animal;
    }
    if (facts.isAnimal)
    {
        // A free animal of the animal level, in both presets (animal-duel-submission
        // D1). exclude.animals always wins; Balanced also needs animals switched on.
        facts.animalAllowed = config.duelChallenges.animalsEnabled && !config.exclude.animals &&
                              (facts.cheatPreset || config.animalsEnabled);
        if (!facts.animalAllowed)
        {
            return false;
        }
        facts.animalOwned = IsOwnedAnimal(npc);
        facts.animalFromLevel = config.duelChallenges.animalsFromLevel;
    }
    else if (!(facts.allyTiered || facts.cheatPreset))
    {
        return false;
    }
    facts.tsAvailable = TsAvailable();
    // The level TotalSparring shows (npc_level.h); the tier's own read otherwise.
    facts.npcLevel = levelFallback;
    SafeNpcLevel(npc, &facts.npcLevel);
    facts.eliteFromLevel = config.allyTiers.eliteFromLevel;
    facts.cheatPeopleFromLevel = config.duelChallenges.cheatPeopleFromLevel;
    CharKey key;
    double now = 0.0;
    facts.onCooldown = SafeFindCharKey(npc, &key) && SafeGameHours(&now) && DuelOnCooldown(key, now);
    return CanOfferDuel(facts);
}
} // namespace

bool DuelOfferedFor(Character* npc, const Verdict& verdict)
{
    return DuelOfferedWith(npc, verdict.allyTiered, verdict.isAnimal, verdict.allyLevel);
}

namespace
{
// duel-any-source U2: the offer rule for side B of a TotalSparring duel nobody
// challenged through TotalRecruit. The verdict of the moment cannot be used (a rival
// just knocked out takes the unconscious route), so the ally tier is read past the
// route (IsTieredAllyAtRest); the rest is the button's own rule.
bool ExternalRivalOfferedUnguarded(Character* npc)
{
    return DuelOfferedWith(npc, IsTieredAllyAtRest(npc), false, 0);
}

// POD frame: the reads above are guarded one by one, this catches anything else.
bool ExternalRivalOffered(Character* npc)
{
    TR_TRY
    {
        return ExternalRivalOfferedUnguarded(npc);
    }
    TR_CATCH
    {
        return false;
    }
}

// Whether a bout with no pending challenge counts as one (JudgeExternalBout), with
// the log line for a duel. Spars and duels a challenge answers say nothing here.
bool ExternalBoutEligible(const DuelBoutRecord& bout, const DuelChallenges& config)
{
    ExternalBoutFacts facts;
    facts.countExternal = config.countExternalBouts;
    facts.hasPending = bout.bKnown && DuelChallengePending(bout.b);
    ExternalBoutReason reason = JudgeExternalBout(bout, facts);
    if (reason == ExternalBout_NotDuel || reason == ExternalBout_Pending)
    {
        return false;
    }
    Character* rival = NULL;
    if (reason == ExternalBout_CheckOffer)
    {
        rival = FindNearSquad(bout.b);
        facts.offerChecked = true;
        facts.offered = rival != NULL && ExternalRivalOffered(rival);
        reason = JudgeExternalBout(bout, facts);
    }
    const std::string name = rival != NULL ? CharacterName(rival)
                                           : (bout.bKnown ? bout.b.ToString() : std::string("?"));
    if (reason == ExternalBout_Counts)
    {
        LogInfo(std::string("[duel] external bout: ") + name + " counts (" +
                (bout.outcome == DuelBout_AWon ? "won" : "lost") + ")");
        return true;
    }
    LogInfo(std::string("[duel] external bout: ") + name + " ignored (" +
            (reason == ExternalBout_NotOffered && rival == NULL ? "not near the squad"
                                                                : ExternalBoutReasonWord(reason)) +
            ")");
    return false;
}
} // namespace

DuelJoinOdds DuelNextWinOddsNow(int winsSoFar)
{
    const Config& config = GetConfig();
    return DuelNextWinOdds(config.preset == Preset_Cheat, winsSoFar, config.duelChallenges);
}

bool ReadDuelRecord(Character* challenger, Character* npc, int* wins, int* losses, int* draws)
{
    return TsDuelStats(challenger, npc, wins, losses, draws);
}

void ChallengeToDuel(Character* npc)
{
    if (npc == NULL)
    {
        return;
    }
    const std::string name = CharacterName(npc);
    if (!TsAvailable())
    {
        TellPlayer(DuelOpenProblem(TsOpen_NotReady, npc, name), true);
        return;
    }
    // The cage commands' actor rule: the selected squad member, awake, not the NPC.
    Character* challenger = SelectedSquadActor(npc);
    if (challenger == NULL)
    {
        TellPlayer(CageCommandProblem(CageCommand_NoActor, npc), true);
        return;
    }

    CharKey npcKey;
    CharKey challengerKey;
    double now = 0.0;
    if (!SafeCharKey(npc, &npcKey) || !SafeCharKey(challenger, &challengerKey) ||
        !SafeGameHours(&now))
    {
        LogError("[duel] could not read the keys or the clock to challenge " + name + ".");
        TellPlayer(DuelOpenProblem(TsOpen_Failed, npc, name), true);
        return;
    }

    // Recorded before the window opens, so even an instant result finds it.
    RecordDuelChallenge(npcKey, challengerKey, now);
    const TsOpenResult opened = TsOpenDuel(npc, challenger);
    if (opened == TsOpen_Opened)
    {
        TellPlayer(Fill(DuelLine(DuelNarration_Challenged, npc, DuelNpcIsAnimal(npc)), name, 0, 0),
                   false);
        LogInfo("[duel] " + CharacterName(challenger) + " challenges " + name + " [key " +
                npcKey.ToString() + "]");
        return;
    }
    WithdrawDuelChallenge(npcKey);
    TellPlayer(DuelOpenProblem(opened, npc, name), true);
    LogInfo("[duel] TotalSparring did not open the duel with " + name + " (" +
            IntToString((int)opened) + "): challenge withdrawn.");
}

void DuelChallengeTick()
{
    TsBridgeTick();

    std::vector<DuelBoutRecord> bouts;
    TsTakeBoutEvents(&bouts);
    const DWORD tick = GetTickCount();
    const bool sweep = tick - g_lastDuelSweep >= kDuelExpirySweepMs;
    if (bouts.empty() && !sweep)
    {
        return;
    }
    double now = 0.0;
    if (!SafeGameHours(&now))
    {
        if (!bouts.empty())
        {
            LogError("[duel] the game clock could not be read: " + IntToString((int)bouts.size()) +
                     " bout(s) not resolved.");
        }
        return;
    }

    // The lost-duel cooldown is Cheat's minute in Cheat (cheat_waits.h); the rest of
    // the duel settings are read as configured.
    DuelChallenges config = GetConfig().duelChallenges;
    config.cooldownHours = DuelLossCooldownHours(GetConfig());
    if (sweep)
    {
        g_lastDuelSweep = tick;
        const int expired = ExpireDuelChallenges(now, config.expiryHours);
        if (expired > 0)
        {
            LogInfo("[duel] " + IntToString(expired) + " challenge(s) expired unanswered.");
        }
    }

    for (size_t i = 0; i < bouts.size(); ++i)
    {
        const bool external = ExternalBoutEligible(bouts[i], config);
        const DuelResolveResult result = ResolveDuelBout(bouts[i], now, config, external);
        LogBout(bouts[i], result.resolution);
        if (result.resolution == DuelResolution_Won)
        {
            OnDuelWon(result, now);
        }
        else if (result.resolution == DuelResolution_Lost)
        {
            OnDuelLost(result, now);
        }
    }
}
} // namespace totalrecruit
