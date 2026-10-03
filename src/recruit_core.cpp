#include "common.h"
#include "recruit_core.h"
#include "config.h"
#include "log.h"
#include "rules.h"
#include "insistence.h"
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
#include "cats_purse.h"
#include "money_rules.h"

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

// The remedy the third button offers, when the plugin can actually deliver it.
// Only one fits, so it is the one that applies: food when they are hungry and
// you are carrying some, otherwise the beating -- the lever R2.3 gives the
// player against someone whose price is submission. HelpCaptive dispatches on
// the same test, so the label and the action cannot disagree.
OfferRemedy RemedyFor(Character* character, const Verdict& verdict, bool again)
{
    if (verdict.willJoin)
    {
        return OfferRemedy_None;
    }
    if (verdict.currency == Currency_Care && verdict.wantsFood && PlayerHasFood())
    {
        return again ? OfferRemedy_FeedMore : OfferRemedy_Feed;
    }
    if (CanBeatPrisoner(character, NULL))
    {
        return OfferRemedy_Beat;
    }
    return OfferRemedy_None;
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
    in.remedy = RemedyFor(character, verdict, afterRemedy);
    in.afterRemedy = afterRemedy;
    return ShowOffer(character, BuildOfferView(in, OfferWording()));
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

    // Config exclusions come first: they are the player's own opt-outs, and
    // they outrank anything the rules would say.
    std::string excluded;
    if (!IsRecruitable(character, &excluded))
    {
        TellPlayer(name + ": " + excluded, true);
        return;
    }

    // Registra la interacción para evitar spam/insistencia (ignorar temporalmente tras 3 intentos seguidos)
    if (ou != NULL)
    {
        double nowHours = ou->getTimeStamp_inGameHours().getTotalHours();
        RecordInteraction(character, nowHours);
    }

    const Verdict verdict = Evaluate(character);

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

    // The same button carries whichever remedy the window offered, so the
    // dispatch has to agree with the label PresentOffer chose.
    const Verdict before = Evaluate(character);
    const OfferRemedy remedy = RemedyFor(character, before, false);
    const bool feeding = (remedy == OfferRemedy_Feed);

    std::string message;
    if (feeding)
    {
        if (!FeedCaptive(character, &message))
        {
            TellPlayer(name + ": " + message, true);
            return;
        }
        TellPlayer(name + ": " + message, false);
    }
    else
    {
        if (!BeatPrisoner(character, &message))
        {
            TellPlayer(message, true);
            return;
        }
        TellPlayer(message, false);
        // They are unconscious now, so there is no conversation to reopen.
        return;
    }

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

    if (!accepted)
    {
        TellPlayer(name + ": lo dejaste donde estaba.", false);
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

    // Money only moves here. Declining, or walking away, costs nothing.
    const int due = CatsDue(verdict);
    if (!SettlePrice(character, due, name))
    {
        return;
    }

    std::string message;
    if (Recruit(character, &message))
    {
        TellPlayer(name + ": " + verdict.reason, false);
    }
    else
    {
        // Paid but not delivered: the engine refused the recruit. Put the money back.
        RefundPrice(character, due, name);
        TellPlayer(name + ": " + message, true);
    }
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
} // namespace totalrecruit
