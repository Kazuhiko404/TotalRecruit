#include "common.h"
#include "readout.h"
#include "config.h"
#include "engine_reads.h"
#include "log.h"
#include "mercy.h"
#include "offer_popup.h"
#include "readout_model.h"
#include "readout_target.h"
#include "readout_text.h"
#include "readout_ui.h"
#include "recruit_core.h"
#include "rules.h"

#include <cstdio>

namespace totalrecruit
{
namespace
{
// Hover (opt-in) re-evaluates four times a second: enough to follow the cursor.
const unsigned int kHoverIntervalMs = 250;
// A card pinned by Hablar with no offer window to close it goes after this long.
const unsigned int kPinTimeoutMs = 8000;

enum CardMode
{
    Card_None,
    Card_Pinned, // the Hablar click
    Card_Hover   // readoutOnHover
};

CardMode g_mode = Card_None;
// The character the card is showing. Raw: only ever read behind a guard, and cleared
// at a load. Never dereferenced here.
Character* g_shown = NULL;
DWORD g_nextHoverUpdate = 0;
DWORD g_pinnedAt = 0;
// The offer window has been seen open since the pin: its closing ends the card.
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

// What is drawn, in one line. Only logged when it changes, so hovering does not
// write four lines a second.
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

// Works the card out for this character and shows it: the one full evaluation.
// False (card hidden) when there is nothing to show. `fallback` is where to hang
// the card when the character cannot be projected (the cursor, for hover).
bool ShowFor(Character* character, const int* fallbackX, const int* fallbackY,
             const char* source)
{
    const Config& config = GetConfig();
    if (!IsRecruitable(character, NULL))
    {
        return false;
    }

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

    const ReadoutTexts texts = BuildReadoutTexts(view, CharacterName(character), character);

    int ax = 0, ay = 0;
    if (!ReadoutAnchorFor(character, &ax, &ay))
    {
        if (fallbackX == NULL || fallbackY == NULL)
        {
            return false;
        }
        ax = *fallbackX;
        ay = *fallbackY - 24;
    }

    if (!ReadoutUiShow(view, texts, config.readoutCompact, ax, ay))
    {
        return false;
    }

    if (config.probeOnClick)
    {
        LogReadout(character, view, texts, source);
    }
    return true;
}

// Follows the pinned NPC, beside the offer window while it is open; ends the pin
// when its reason to be on screen is gone. No evaluation: the card was worked out
// once, at the click.
void TickPinned()
{
    const bool offerOpen = IsOfferOpen();
    if (offerOpen)
    {
        g_offerSeen = true;
    }
    else if (g_offerSeen || GetTickCount() - g_pinnedAt >= kPinTimeoutMs)
    {
        HideCard();
        return;
    }

    bool dead = false;
    int ax = 0, ay = 0;
    if (!SafeIsDead(g_shown, &dead) || dead || !ReadoutAnchorFor(g_shown, &ax, &ay))
    {
        HideCard();
        return;
    }

    ReadoutRect offer;
    if (offerOpen && OfferWindowRect(&offer.left, &offer.top, &offer.width, &offer.height))
    {
        ReadoutUiMove(ax, ay, &offer);
    }
    else
    {
        ReadoutUiMove(ax, ay);
    }
}

// The opt-in hover: the card follows the NPC every frame and is worked out again
// every kHoverIntervalMs for whoever is under the cursor.
void TickHover()
{
    if (IsOfferOpen())
    {
        if (g_mode != Card_None)
        {
            HideCard();
        }
        return;
    }

    if (g_mode == Card_Hover)
    {
        int ax = 0, ay = 0;
        if (ReadoutAnchorFor(g_shown, &ax, &ay))
        {
            ReadoutUiMove(ax, ay);
        }
    }

    const DWORD now = GetTickCount();
    if (now < g_nextHoverUpdate)
    {
        return;
    }
    g_nextHoverUpdate = now + kHoverIntervalMs;

    ReadoutTarget target;
    if (!FindHoverTarget(&target) ||
        !ShowFor(target.character, &target.mouseX, &target.mouseY, "hover"))
    {
        HideCard();
        return;
    }
    g_mode = Card_Hover;
    g_shown = target.character;
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
    if (!ShowFor(character, NULL, NULL, "hablar"))
    {
        HideCard();
        return;
    }
    g_mode = Card_Pinned;
    g_shown = character;
    g_pinnedAt = GetTickCount();
    g_offerSeen = false;
    // Placed beside the offer at once, not one frame late.
    TickPinned();
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
        return;
    }

    if (g_mode == Card_Pinned)
    {
        TickPinned();
        return;
    }

    // Default: nothing to do. Hover only runs when the player asked for it.
    if (config.readoutOnHover)
    {
        TickHover();
    }
    else if (g_mode == Card_Hover)
    {
        HideCard();
    }
}

void ForgetReadout()
{
    HideCard();
    g_nextHoverUpdate = 0;
    g_lastLogged.clear();
}

void ShutdownReadout()
{
    ForgetReadout();
    ReadoutUiDestroy();
}
} // namespace totalrecruit
