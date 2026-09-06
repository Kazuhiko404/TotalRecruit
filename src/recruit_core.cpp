#include "common.h"
#include "recruit_core.h"
#include "config.h"
#include "log.h"
#include "rules.h"
#include "dialogue_bridge.h"
#include "dialogue_authoring.h"
#include "offer_popup.h"
#include "feeding.h"

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

// Reads through a possibly-dangling pointer, so each accessor is guarded.
bool SafeIsDead(Character* character, bool* out)
{
    TR_TRY { *out = character->isDead(); return true; }
    TR_CATCH { return false; }
}

bool SafeIsDown(Character* character, bool* out)
{
    TR_TRY { *out = character->isDown(); return true; }
    TR_CATCH { return false; }
}

bool SafeIsAnimal(Character* character, bool* out)
{
    TR_TRY { *out = (character->isAnimal() != NULL); return true; }
    TR_CATCH { return false; }
}

bool SafeIsPlayerCharacter(Character* character, bool* out)
{
    TR_TRY { *out = character->isPlayerCharacter(); return true; }
    TR_CATCH { return false; }
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

// getName returns std::string by value, so the call and the copy out of it
// both live in a helper that the guard only jumps through.
void CopyName(Character* character, char* buffer, size_t size)
{
    strncpy_s(buffer, size, character->getName().c_str(), _TRUNCATE);
}

// showPlayerAMessage takes a std::string, so the temporary is built in a plain
// function and only the call itself is guarded.
void PostMessage(const char* text, bool queued)
{
    ou->showPlayerAMessage(std::string(text), queued);
    ou->playNotification(NULL);
}

// Takes cats out of the player's purse. Money lives on the squad leader, and
// takeMoney reports whether the funds were actually there.
bool DeductMoney(int amount, bool* paid)
{
    if (ou == NULL || ou->player == NULL)
    {
        return false;
    }
    Platoon* platoon = ou->player->currentPlatoon;
    if (platoon == NULL)
    {
        return false;
    }
    Character* leader = platoon->getSquadLeader();
    if (leader == NULL)
    {
        return false;
    }
    *paid = leader->takeMoney(amount);
    return true;
}

bool SafeCopyName(Character* character, char* buffer, size_t size)
{
    TR_TRY
    {
        CopyName(character, buffer, size);
        return true;
    }
    TR_CATCH { return false; }
}

void ShowMessage(const char* text, bool isProblem)
{
    TR_TRY { PostMessage(text, isProblem); }
    TR_CATCH { }
}

bool ChargePlayer(int amount)
{
    bool paid = false;
    TR_TRY
    {
        if (!DeductMoney(amount, &paid))
        {
            return false;
        }
    }
    TR_CATCH { return false; }
    return paid;
}

std::string IntToString(int value)
{
    char buffer[32] = { 0 };
    _snprintf_s(buffer, sizeof(buffer), _TRUNCATE, "%d", value);
    return std::string(buffer);
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

// Opens the window for a verdict, whether it is a yes or a no. A refusal still
// gets a window because that is where the player can act on it -- feeding a
// starving captive turns "no" into "not yet", which is the whole point.
bool PresentOffer(Character* character, const Verdict& verdict, const std::string& spoken)
{
    // The remedy button appears only when the plugin can actually deliver it.
    std::string extraLabel;
    if (!verdict.willJoin && verdict.currency == Currency_Care && verdict.wantsFood && PlayerHasFood())
    {
        extraLabel = "DARLE COMIDA";
    }

    // A window only earns its place when there is a choice inside it. If they
    // will not join and there is no remedy to offer, the speech bubble has
    // already said everything -- repeating it in a box with a single dismiss
    // button is noise the player has to click away.
    // An animal's bubble is a noise, not an explanation, so the window is the
    // only place a refusal can actually say why. People keep the old rule:
    // their bubble already said it, and a box with one dismiss button is noise.
    if (!verdict.willJoin && extraLabel.empty() && !verdict.isAnimal)
    {
        return false;
    }

    // The window narrates the situation; the NPC's own words are already in a
    // bubble over their head, so repeating them here would waste the one line
    // the window has.
    const std::string body = DescribeOffer(character, verdict);

    if (!verdict.willJoin)
    {
        return ShowOffer(character, body, "", "CERRAR", extraLabel);
    }

    return ShowOffer(character, body, SpeakAcceptLine(verdict), "RECHAZAR", extraLabel);
}
} // namespace

std::string CharacterName(Character* character)
{
    if (character == NULL)
    {
        return "<unnamed>";
    }

    char buffer[128] = { 0 };
    if (!SafeCopyName(character, buffer, sizeof(buffer)))
    {
        return "<unnamed>";
    }
    return std::string(buffer);
}

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
    // player most needs somewhere to act. So it goes through the same window as
    // an acceptance, carrying whatever remedy applies, rather than ending in
    // the message roller with nothing to click.
    if (!verdict.willJoin)
    {
        LogInfo("Refused " + name + " (" + CurrencyName(verdict.currency) + "): " +
                verdict.reason);
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
        RememberPendingPrice(character, verdict.cats);
        return;
    }

    if (PresentOffer(character, verdict, spoken))
    {
        return;
    }

    // No window. This used to recruit outright when the verdict was yes, which
    // is the same defect the rescue route had: the squad changed with nobody
    // agreeing to it, and from the player's side it looks like the popup simply
    // never appeared. Say what happened instead, and log why the window failed
    // -- silence here is what made this hard to see.
    if (verdict.willJoin)
    {
        LogError("Offer window would not open for " + name +
                 "; not recruiting without a confirmation.");
        TellPlayer(name + ": " + verdict.reason, false);
    }
    else
    {
        TellPlayer(name + ": " + verdict.reason, true);
    }
}

void HelpCaptive(Character* character)
{
    if (character == NULL)
    {
        return;
    }

    const std::string name = CharacterName(character);

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

    std::string extraLabel;
    if (!after.willJoin && after.currency == Currency_Care && after.wantsFood && PlayerHasFood())
    {
        extraLabel = "Darle mas comida";
    }

    ShowOffer(character, spoken, SpeakAcceptLine(after), "Ahora no.", extraLabel);
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
    if (!SettlePrice(character, verdict.cats, name))
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
        TellPlayer(name + ": " + message, true);
    }
}

bool SettlePrice(Character* character, int cats, const std::string& name)
{
    if (cats <= 0)
    {
        return true;
    }

    if (!ChargePlayer(cats))
    {
        TellPlayer(name + ": no tienes suficientes fondos (" + IntToString(cats) + " cats)", true);
        return false;
    }

    TellPlayer(name + ": pagaste " + IntToString(cats) + " cats", false);
    return true;
}
} // namespace totalrecruit
