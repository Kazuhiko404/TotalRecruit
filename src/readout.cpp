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
// Four updates a second: enough to follow a hover, cheap enough that the peek (the
// whole decision, with the talent survey for a price) is not noticeable.
const unsigned int kUpdateIntervalMs = 250;

DWORD g_nextUpdate = 0;
// The character the card is showing. Raw: only ever read behind a guard, and cleared
// at a load. Never dereferenced here.
Character* g_shown = NULL;
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
    g_shown = NULL;
    if (ReadoutUiVisible())
    {
        ReadoutUiHide();
    }
}

// What is drawn, in one line. Only logged when it changes, so hovering does not
// write four lines a second.
void LogReadout(Character* character, const ReadoutView& view, const ReadoutTexts& texts,
                ReadoutSource source)
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
                texts.secondary.c_str(), ReadoutSourceName(source));
    const std::string line = "[readout] " + CharacterName(character) + buf;
    if (line != g_lastLogged)
    {
        g_lastLogged = line;
        LogInfo(line);
    }
}

void Update()
{
    const Config& config = GetConfig();

    ReadoutTarget target;
    if (!FindReadoutTarget(config.readoutOnHover, &target))
    {
        HideCard();
        return;
    }

    Character* character = target.character;
    if (!IsRecruitable(character, NULL))
    {
        HideCard();
        return;
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
        HideCard();
        return;
    }

    const ReadoutTexts texts = BuildReadoutTexts(view, CharacterName(character), character);

    int ax = 0, ay = 0;
    if (!ReadoutAnchorFor(character, &ax, &ay))
    {
        ax = target.mouseX;
        ay = target.mouseY - 24;
    }

    if (!ReadoutUiShow(view, texts, config.readoutCompact, ax, ay))
    {
        HideCard();
        return;
    }
    g_shown = character;

    if (config.probeOnClick)
    {
        LogReadout(character, view, texts, target.source);
    }
}
} // namespace

void ReadoutTick()
{
    const Config& config = GetConfig();
    if (!config.enabled || !config.readoutEnabled || IsOfferOpen())
    {
        if (g_shown != NULL || ReadoutUiVisible())
        {
            HideCard();
        }
        return;
    }

    // Between updates the card only follows the NPC it is about.
    if (g_shown != NULL)
    {
        int ax = 0, ay = 0;
        if (ReadoutAnchorFor(g_shown, &ax, &ay))
        {
            ReadoutUiMove(ax, ay);
        }
    }

    const DWORD now = GetTickCount();
    if (now < g_nextUpdate)
    {
        return;
    }
    g_nextUpdate = now + kUpdateIntervalMs;
    Update();
}

void ForgetReadout()
{
    HideCard();
    g_nextUpdate = 0;
    g_lastLogged.clear();
}

void ShutdownReadout()
{
    ForgetReadout();
    ReadoutUiDestroy();
}
} // namespace totalrecruit
