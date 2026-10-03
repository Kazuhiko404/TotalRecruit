#include "common.h"
#include "readout.h"
#include "card_dismiss_rules.h"
#include "config.h"
#include "defeats.h"
#include "engine_reads.h"
#include "log.h"
#include "mercy.h"
#include "mouse_input.h"
#include "offer_popup.h"
#include "readout_model.h"
#include "readout_target.h"
#include "readout_text.h"
#include "readout_ui.h"
#include "recruit_core.h"
#include "recruit_overlay.h"
#include "rules.h"

#include <cstdio>

namespace totalrecruit
{
namespace
{
// A card that follows the selection is worked out again this often, so the price
// and the stress it shows stay roughly current. Slow on purpose: the evaluation is
// the costly part, and every frame in between only moves the card.
const unsigned int kSelectionRefreshMs = 2000;
// A card pinned by Hablar with no offer window to close it goes after this long.
const unsigned int kPinTimeoutMs = 8000;

enum CardMode
{
    Card_None,
    Card_Pinned,  // the Hablar click
    Card_Selected // readoutOnSelect
};

CardMode g_mode = Card_None;
// The character the card is about. Raw: only ever read behind a guard, and cleared
// at a load. Never dereferenced here.
Character* g_shown = NULL;
// What the card shows, as last worked out. Kept so a card that went off the screen
// comes back without another evaluation.
ReadoutView g_view;
ReadoutTexts g_texts;
// The selection as last seen: comparing it is the whole per-frame cost of the
// selection mode. Raw, never dereferenced, cleared at a load.
Character* g_lastSelection = NULL;
DWORD g_nextRefresh = 0;
DWORD g_pinnedAt = 0;
// The offer window has been seen open since the pin: its closing ends the pin.
bool g_offerSeen = false;
std::string g_lastLogged;

// A click elsewhere put the card away (card_dismiss_rules.h). Kept apart from the
// mode: the card is still worked out for its NPC, only not drawn.
bool g_dismissed = false;
PressEdgeState g_pressEdges;
// The selection as the dismissal last saw it. Separate from g_lastSelection, which
// the end of a pin clears to re-decide the card: that is not a new selection. Raw,
// never dereferenced, cleared at a load.
Character* g_dismissSelection = NULL;

// What Present did with the card, for the probeOnClick log: each outcome is
// logged once per target (and again once the card is about someone else), so a
// log states where the card went without a line per frame.
enum PresentOutcome
{
    Present_DeadReadFailed,
    Present_Dead,
    Present_MenuOpen,
    Present_AnchorFailed,
    Present_OverHead,
    Present_BesideOffer,
    Present_NoRoom,
    Present_OutcomeCount
};
// The target the outcomes below were logged for. Raw, never dereferenced.
Character* g_anchorLogTarget = NULL;
// One bit per PresentOutcome, plus one per AnchorResult for the failures.
unsigned int g_anchorLogged = 0;

const char* RouteWord(ReadoutRoute route)
{
    switch (route)
    {
    case ReadoutRoute_Captivity: return "captivity";
    case ReadoutRoute_Rescue:    return "rescue";
    case ReadoutRoute_Surrender: return "surrender";
    case ReadoutRoute_FieldYield: return "field-yield";
    case ReadoutRoute_Mercy:     return "mercy";
    case ReadoutRoute_Animal:    return "animal";
    case ReadoutRoute_Free:
    default:                     return "free";
    }
}

bool TimeReached(DWORD now, DWORD when)
{
    return (LONG)(now - when) >= 0;
}

void HideCard()
{
    g_mode = Card_None;
    g_shown = NULL;
    g_offerSeen = false;
    if (ReadoutUiVisible())
    {
        ReadoutUiHide();
    }
}

// What is drawn, in one line. Only logged when it changes, so the slow refresh of
// an unchanged card writes nothing.
void LogReadout(Character* character, const ReadoutView& view, const ReadoutTexts& texts,
                const char* source)
{
    char buf[320] = { 0 };
    _snprintf_s(buf, sizeof(buf), _TRUNCATE,
                " route=%s chance=%s stress=%s (hunger %d wounds %d tired %d) defeats=%s "
                "food=%s line=\"%s\"%s%s [%s]",
                RouteWord(view.route),
                view.showChance ? texts.chanceValue.c_str() : "-",
                view.showStress ? texts.stressValue.c_str() : "-",
                view.hungerPts, view.woundsPts, view.tiredPts,
                view.showDefeats ? texts.defeatsValue.c_str() : "-",
                view.showFood ? texts.foodValue.c_str() : "-",
                texts.primary.c_str(),
                texts.secondary.empty() ? "" : " + ",
                texts.secondary.c_str(), source);
    const std::string line = "[readout] " + CharacterName(character) + buf;
    if (line != g_lastLogged)
    {
        g_lastLogged = line;
        LogInfo(line);
    }
}

// Works the card out for this character: the one full evaluation. False when there
// is nothing to show (not someone the mod talks to, or a verdict with no card).
// On success the card is about `character` and holds the new content.
bool WorkOut(Character* character, const char* source)
{
    const Config& config = GetConfig();
    if (!IsRecruitable(character, NULL))
    {
        return false;
    }

    // The card is looking at them anyway: standing again re-arms the next
    // knockout (defeat_episode_rules). Never counts, never mints an id.
    NoteSeenStanding(character);

    // The same decision a click would run, asked as a bystander.
    EvaluationDetail detail;
    const Verdict verdict = EvaluatePeek(character, &detail);

    ReadoutInput input;
    input.route = detail.route;
    input.haveCore = detail.haveCore;
    input.core = detail.core;
    input.verdict = verdict;
    input.holdKnown = detail.holdKnown;
    input.hoursHeld = detail.hoursHeld;
    input.mercyPending = IsMercyPending(character);
    // The peek mints no id, so it has no roll key: a roll it could not make is
    // shown as odds (readout_model.h).
    input.rollKeyed = false;

    const ReadoutView view = BuildReadout(input, config);
    if (!view.visible)
    {
        return false;
    }

    g_view = view;
    g_texts = BuildReadoutTexts(view, CharacterName(character), character);
    g_shown = character;
    // A changed card is laid out again on the next show (readout_ui's LayoutKey).
    if (ReadoutUiVisible())
    {
        ReadoutUiHide();
    }

    if (config.probeOnClick)
    {
        LogReadout(character, g_view, g_texts, source);
    }
    return true;
}

// True the first time this outcome is seen for the card's current target.
bool FirstTimeFor(PresentOutcome outcome, AnchorResult anchor)
{
    if (g_anchorLogTarget != g_shown)
    {
        g_anchorLogTarget = g_shown;
        g_anchorLogged = 0;
    }
    unsigned int bit = 1u << (unsigned int)outcome;
    if (outcome == Present_AnchorFailed)
    {
        bit = 1u << ((unsigned int)Present_OutcomeCount + (unsigned int)anchor);
    }
    if ((g_anchorLogged & bit) != 0)
    {
        return false;
    }
    g_anchorLogged |= bit;
    return true;
}

// One probeOnClick line per outcome per target. The format:
//   [readout-anchor] <name> [select|hablar] <outcome> head=(x,y) view=WxH
//       world=(x,y,z) origin=(x,y,z) [card=(l,t wxh)] [offer=(l,t wxh)]
// head is the projected point over the head (the card's bottom hangs above it),
// world the game-space point that was projected, origin the scene's relative
// origin taken out first.
void LogPresent(PresentOutcome outcome, const AnchorReading* anchor,
                const ReadoutPlacement* placed, const ScreenRect* offer, const char* source)
{
    if (!GetConfig().probeOnClick ||
        !FirstTimeFor(outcome, anchor != NULL ? anchor->result : Anchor_Ok))
    {
        return;
    }

    const char* what = "";
    switch (outcome)
    {
    case Present_DeadReadFailed: what = "over: the dead-read failed"; break;
    case Present_Dead:           what = "over: dead"; break;
    case Present_MenuOpen:       what = "hidden: right-click menu open"; break;
    case Present_AnchorFailed:   what = "hidden: anchor failed"; break;
    case Present_OverHead:       what = "shown over the head"; break;
    case Present_BesideOffer:    what = "shown beside the offer"; break;
    case Present_NoRoom:         what = "hidden: no side of the offer has room"; break;
    default:                     break;
    }

    char buf[512] = { 0 };
    int used = _snprintf_s(buf, sizeof(buf), _TRUNCATE, " [%s] %s", source, what);
    if (anchor != NULL && used >= 0)
    {
        const int more = _snprintf_s(
            buf + used, sizeof(buf) - used, _TRUNCATE,
            "%s%s%s head=(%d,%d) view=%dx%d world=(%.1f,%.1f,%.1f) origin=(%.1f,%.1f,%.1f)",
            anchor->result == Anchor_Ok ? "" : " (",
            anchor->result == Anchor_Ok ? "" : AnchorResultWord(anchor->result),
            anchor->result == Anchor_Ok ? "" : ")", anchor->x, anchor->y, anchor->viewW,
            anchor->viewH, anchor->head.x, anchor->head.y, anchor->head.z, anchor->origin.x,
            anchor->origin.y, anchor->origin.z);
        used = more >= 0 ? used + more : used;
    }
    if (placed != NULL && used >= 0)
    {
        const int more = _snprintf_s(buf + used, sizeof(buf) - used, _TRUNCATE,
                                     " card=(%d,%d %dx%d)", placed->card.left, placed->card.top,
                                     placed->card.width, placed->card.height);
        used = more >= 0 ? used + more : used;
    }
    if (offer != NULL && used >= 0)
    {
        _snprintf_s(buf + used, sizeof(buf) - used, _TRUNCATE, " offer=(%d,%d %dx%d)",
                    offer->left, offer->top, offer->width, offer->height);
    }
    LogInfo("[readout-anchor] " + CharacterName(g_shown) + buf);
}

void HideUi()
{
    if (ReadoutUiVisible())
    {
        ReadoutUiHide();
    }
}

// Draws the card that was worked out, over its NPC: no evaluation. Hidden (but kept)
// while the NPC is off the screen or, unless it stands beside a window, while a
// right-click menu is open. `avoid` is the window the card steps beside (the offer):
// sideways only, at the head's height, and hidden when neither side has room.
// False when the NPC died: the card is over.
bool Present(const ScreenRect* avoid, const char* source)
{
    bool dead = false;
    if (!SafeIsDead(g_shown, &dead))
    {
        LogPresent(Present_DeadReadFailed, NULL, NULL, NULL, source);
        return false;
    }
    if (dead)
    {
        LogPresent(Present_Dead, NULL, NULL, NULL, source);
        return false;
    }

    // Put away by a click elsewhere: kept, not drawn.
    if (g_dismissed)
    {
        HideUi();
        return true;
    }

    if (avoid == NULL && IsContextMenuOpen())
    {
        LogPresent(Present_MenuOpen, NULL, NULL, NULL, source);
        HideUi();
        return true;
    }

    const AnchorReading anchor = ReadoutAnchorFor(g_shown);
    if (anchor.result != Anchor_Ok)
    {
        LogPresent(Present_AnchorFailed, &anchor, NULL, avoid, source);
        HideUi();
        return true;
    }

    // Re-placing an unchanged card does not lay it out again (LayoutKey).
    ReadoutPlacement placed;
    if (ReadoutUiVisible())
    {
        ReadoutUiMove(anchor.x, anchor.y, avoid, &placed);
    }
    else if (!ReadoutUiShow(g_view, g_texts, GetConfig().readoutCompact, anchor.x, anchor.y,
                            avoid, &placed))
    {
        return true;
    }

    switch (placed.placement)
    {
    case Placement_BesideWindow:
        LogPresent(Present_BesideOffer, &anchor, &placed, avoid, source);
        break;
    case Placement_NoRoom:
        LogPresent(Present_NoRoom, &anchor, &placed, avoid, source);
        break;
    case Placement_OverHead:
    default:
        LogPresent(Present_OverHead, &anchor, &placed, avoid, source);
        break;
    }
    return true;
}

// The pinned card, beside the offer window while it is open. False when the pin is
// over: the offer closed, no offer opened in time, or (no offer open) the player
// selected someone else.
bool TickPinned(Character* selected)
{
    const bool offerOpen = IsOfferOpen();
    if (offerOpen)
    {
        g_offerSeen = true;
    }
    else if (g_offerSeen || TimeReached(GetTickCount(), g_pinnedAt + kPinTimeoutMs) ||
             selected != g_lastSelection)
    {
        return false;
    }

    ScreenRect offer;
    const bool haveOffer =
        offerOpen && OfferWindowRect(&offer.left, &offer.top, &offer.width, &offer.height);
    if (!Present(haveOffer ? &offer : NULL, "hablar"))
    {
        HideCard();
    }
    return true;
}

// The card follows the selection: worked out when it changes and every
// kSelectionRefreshMs after that, only moved in between.
void TickSelection(Character* selected)
{
    const DWORD now = GetTickCount();
    if (selected != g_lastSelection)
    {
        g_lastSelection = selected;
        HideCard();
        g_nextRefresh = now;
    }
    if (selected == NULL)
    {
        return;
    }
    // A dismissed card costs nothing: no evaluation until it is shown again (which
    // schedules a refresh at once).
    if (g_dismissed)
    {
        HideUi();
        return;
    }

    if (TimeReached(now, g_nextRefresh))
    {
        // Scheduled whether or not it worked: someone the mod does not talk to is
        // asked again at the next refresh, not every frame.
        g_nextRefresh = now + kSelectionRefreshMs;
        if (!WorkOut(selected, "select"))
        {
            HideCard();
            return;
        }
        g_mode = Card_Selected;
    }

    if (g_mode == Card_Selected && !Present(NULL, "select"))
    {
        HideCard();
    }
}

const char* PressWord(MousePress press)
{
    return press == Press_Left ? "left" : (press == Press_Right ? "right" : "no");
}

// Takes the rules' answer. A card shown again is worked out at once, since nothing
// was evaluated while it was away.
void ApplyDismissal(const CardDismissInput& input, Character* npc)
{
    const bool was = g_dismissed;
    g_dismissed = NextCardDismissed(g_dismissed, input);
    if (was == g_dismissed)
    {
        return;
    }
    if (!g_dismissed)
    {
        g_nextRefresh = GetTickCount();
    }
    if (GetConfig().probeOnClick)
    {
        LogInfo("[readout] " + CharacterName(npc) +
                (g_dismissed ? " dismissed: " : " shown again: ") + PressWord(input.press) +
                (input.pinRequested ? " (hablar)" : (input.selectionChanged ? " (selection)" : "")));
    }
}

// Whether the press at the cursor is on this NPC's body. Projected only here, at a
// press, never per frame.
bool PressOnNpc(Character* npc)
{
    int x = 0;
    int y = 0;
    BodySpan body;
    return npc != NULL && ReadMousePosition(&x, &y) && ReadoutBodySpanFor(npc, &body) &&
           PointOnBody(x, y, body);
}

// Once per frame, before the card is drawn: a fresh press elsewhere dismisses it.
// The mouse is only read while there is a card (shown or put away); otherwise the
// edge reader is reset, so a button held as a card appears is not a press.
void UpdateDismissal(Character* selected)
{
    CardDismissInput input;
    input.selectionChanged = selected != g_dismissSelection;
    g_dismissSelection = selected;

    if (g_mode == Card_None && selected == NULL)
    {
        // No card and no NPC one could come back for: nothing to put away.
        g_dismissed = false;
    }
    if (g_mode == Card_None && !g_dismissed)
    {
        ResetPressEdges(&g_pressEdges);
    }
    else
    {
        bool left = false;
        bool right = false;
        ReadMouseButtons(&left, &right);
        input.press = NextPressEdge(&g_pressEdges, left, right);
    }

    // The card's NPC: the pinned one, else the selection (a put-away card may hold
    // no target while its NPC stays selected).
    Character* npc = g_shown != NULL ? g_shown : selected;
    if (input.press != Press_None && !input.selectionChanged)
    {
        input.overMyGui = IsMouseOverMyGui();
        input.pinnedWithOffer = g_mode == Card_Pinned && IsOfferOpen();
        if (!input.overMyGui && !input.pinnedWithOffer)
        {
            input.onNpc = PressOnNpc(npc);
        }
    }
    ApplyDismissal(input, npc);
}

void ForgetDismissal()
{
    g_dismissed = false;
    ResetPressEdges(&g_pressEdges);
    g_dismissSelection = NULL;
}
} // namespace

void RequestReadout(Character* character)
{
    const Config& config = GetConfig();
    if (character == NULL || !config.enabled || !config.readoutEnabled)
    {
        return;
    }

    HideCard();
    if (!WorkOut(character, "hablar"))
    {
        HideCard();
        return;
    }
    // Hablar shows a card that a click had put away.
    CardDismissInput pin;
    pin.pinRequested = true;
    ApplyDismissal(pin, character);
    g_mode = Card_Pinned;
    g_pinnedAt = GetTickCount();
    g_offerSeen = false;
    // The pin outlives whatever was selected at the click; only a later change of
    // selection (with no offer open) ends it.
    Character* selected = config.readoutOnSelect ? SelectedNpc() : NULL;
    g_lastSelection = selected;
    // Placed beside the offer at once, not one frame late.
    TickPinned(selected);
}

void ReadoutTick()
{
    const Config& config = GetConfig();
    if (!config.enabled || !config.readoutEnabled)
    {
        if (g_mode != Card_None)
        {
            HideCard();
        }
        g_lastSelection = NULL;
        ForgetDismissal();
        return;
    }

    Character* selected = config.readoutOnSelect ? SelectedNpc() : NULL;
    UpdateDismissal(selected);

    if (g_mode == Card_Pinned)
    {
        if (TickPinned(selected))
        {
            return;
        }
        // The pin is over. Forgetting the last selection makes the selection mode
        // decide afresh this frame: the card comes back (worked out again, since the
        // offer may have changed things) only if that NPC is still selected.
        HideCard();
        g_lastSelection = NULL;
    }

    TickSelection(selected);
}

void ForgetReadout()
{
    HideCard();
    g_lastSelection = NULL;
    g_nextRefresh = 0;
    g_lastLogged.clear();
    g_anchorLogTarget = NULL;
    g_anchorLogged = 0;
    ForgetDismissal();
}

void ShutdownReadout()
{
    ForgetReadout();
    ReadoutUiDestroy();
}
} // namespace totalrecruit
