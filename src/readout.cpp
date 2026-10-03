#include "common.h"
#include "readout.h"
#include "config.h"
#include "defeats.h"
#include "engine_reads.h"
#include "log.h"
#include "mercy.h"
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

const char* RouteWord(ReadoutRoute route)
{
    switch (route)
    {
    case ReadoutRoute_Captivity: return "captivity";
    case ReadoutRoute_Rescue:    return "rescue";
    case ReadoutRoute_Surrender: return "surrender";
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

// Draws the card that was worked out, over its NPC: no evaluation. Hidden (but kept)
// while the NPC is off the screen or, unless it stands beside a window, while a
// right-click menu is open. `avoid` is the window the card steps beside (the offer).
// False when the NPC died: the card is over.
bool Present(const ReadoutRect* avoid)
{
    bool dead = false;
    if (!SafeIsDead(g_shown, &dead) || dead)
    {
        return false;
    }

    int ax = 0, ay = 0;
    const bool underMenu = avoid == NULL && IsContextMenuOpen();
    if (underMenu || !ReadoutAnchorFor(g_shown, &ax, &ay))
    {
        if (ReadoutUiVisible())
        {
            ReadoutUiHide();
        }
        return true;
    }

    // Re-showing an unchanged card does not lay it out again (LayoutKey): it is a move.
    if (!ReadoutUiVisible() &&
        !ReadoutUiShow(g_view, g_texts, GetConfig().readoutCompact, ax, ay))
    {
        return true;
    }
    ReadoutUiMove(ax, ay, avoid);
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

    ReadoutRect offer;
    const bool haveOffer =
        offerOpen && OfferWindowRect(&offer.left, &offer.top, &offer.width, &offer.height);
    if (!Present(haveOffer ? &offer : NULL))
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

    if (g_mode == Card_Selected && !Present(NULL))
    {
        HideCard();
    }
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
        return;
    }

    Character* selected = config.readoutOnSelect ? SelectedNpc() : NULL;

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
}

void ShutdownReadout()
{
    ForgetReadout();
    ReadoutUiDestroy();
}
} // namespace totalrecruit
