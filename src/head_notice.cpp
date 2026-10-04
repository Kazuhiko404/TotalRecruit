#include "common.h"
#include "head_notice.h"
#include "head_notice_rules.h"
#include "char_key_reader.h"
#include "config.h"
#include "engine_reads.h"
#include "log.h"
#include "native_windows.h"
#include "offer_popup.h"
#include "readout.h"
#include "readout_target.h"
#include "recruit_overlay.h"
#include "screen_projection.h"
#include "submission_rules.h"
#include "ui_style.h"

#include <set>
#include <vector>

namespace totalrecruit
{
namespace
{
// ------------------------------------------------------------------- State
// Death, joining and a reused address are looked for this often, not every frame:
// each notice costs a few guarded reads. Every frame in between only projects.
const unsigned int kCheckIntervalMs = 500;

HeadNoticeBoard g_board;
// The notice clock: real seconds, frozen while paused, at most 1 s per frame
// (AdvanceEventClock, the submission director's clock rule).
float g_clock = 0.0f;
DWORD g_lastTick = 0;
bool g_haveTick = false;
DWORD g_nextCheck = 0;
// The selection as last seen, to catch the frame it changes. Raw, never
// dereferenced here, cleared at a load.
Character* g_lastSelection = NULL;
// The notices raised with RaiseSquadHeadNotice: their NPC being one of the player's
// is not a reason to drop them. A key outlives its notice harmlessly (an ordinary
// raise for that NPC takes it out).
std::set<CharKey> g_squadNotices;

// --------------------------------------------------------------- Widgets
// The readout card's look (readout_ui.cpp), compact: px values of the
// .head-notice rules in ui-mockup/css/readout.css.
using namespace ui;

const int kPad = 6;
const int kNameH = 18;
const int kLineH = 16;
const int kGap = 3;
const int kMinWidth = 120;
const int kMaxWidth = 264;
const int kArrowGap = 8;
const int kScreenMargin = 4;

const char* const kRootLayer = "ToolTip";
const char* const kRootSkin = "Kenshi_TextBoxEmptySkin";

struct NoticeCard
{
    MyGUI::Widget* root;
    MyGUI::Widget* bg;
    MyGUI::Widget* frame[4];
    MyGUI::Widget* accent;
    MyGUI::Widget* rule;
    MyGUI::TextBox* name;
    MyGUI::TextBox* line;
};

NoticeCard g_cards[kHeadNoticeCap];
// What each card was last laid out for (name and line), and its size then. Laying
// out only happens when this changes; otherwise the card is only moved.
std::string g_layoutKey[kHeadNoticeCap];
int g_width[kHeadNoticeCap];
int g_height[kHeadNoticeCap];
bool g_built = false;
bool g_buildFailed = false;

// Unguarded: builds C++ temporaries (C2712). SafeBuild holds the guard.
bool BuildUnguarded()
{
    MyGUI::Gui* gui = MyGUI::Gui::getInstancePtr();
    if (gui == NULL)
    {
        return false;
    }
    for (int i = 0; i < kHeadNoticeCap; ++i)
    {
        char widgetName[32] = { 0 };
        _snprintf_s(widgetName, sizeof(widgetName), _TRUNCATE, "TR_HeadNotice%d", i);
        NoticeCard& c = g_cards[i];
        c.root = gui->createWidget<MyGUI::Widget>(kRootSkin, MyGUI::IntCoord(0, 0, kMinWidth, 40),
                                                  MyGUI::Align::Default, kRootLayer, widgetName);
        if (c.root == NULL)
        {
            return false;
        }
        c.root->setNeedMouseFocus(false);
        c.root->setNeedKeyFocus(false);
        c.root->setVisible(false);

        // Fills first, in paint order (ui_style.h), then the texts above them.
        c.bg = MakeFill(c.root, kLacquer);
        if (c.bg != NULL) c.bg->setAlpha(0.92f);
        for (int k = 0; k < 4; ++k) c.frame[k] = MakeFill(c.root, kLine);
        c.accent = MakeFill(c.root, kAmber);
        c.rule = MakeFill(c.root, kGoldDim);
        c.name = MakeLabel(c.root, kNameFont, kBrass);
        c.line = MakeLabel(c.root, kBodyFont, kAmber);
        g_width[i] = 0;
        g_height[i] = 0;
    }
    return true;
}

int TextWidth(MyGUI::TextBox* box)
{
    return box != NULL ? box->getTextSize().width : 0;
}

// Unguarded: captions and coords are temporaries (C2712). SafeLayout holds the guard.
void LayoutUnguarded(int slot, const HeadNotice& notice)
{
    NoticeCard& c = g_cards[slot];
    c.name->setCaption(notice.name);
    c.line->setCaption(notice.text);

    int width = TextWidth(c.name);
    const int lineW = TextWidth(c.line);
    if (lineW > width) width = lineW;
    width += 2 * kPad + 4;
    if (width < kMinWidth) width = kMinWidth;
    if (width > kMaxWidth) width = kMaxWidth;
    const int inner = width - 2 * kPad - 4;
    const int x = kPad + 4;

    int y = kPad;
    Place(c.name, x, y, inner, kNameH);
    y += kNameH + 1;
    Place(c.rule, x, y, inner, 1);
    y += 1 + kGap;
    Place(c.line, x, y, inner, kLineH);
    y += kLineH + kPad - 2;

    // The amber edge on the left says "this one is calling you" at a glance.
    Place(c.accent, 1, 1, 3, y - 2);
    Place(c.bg, 0, 0, width, y);
    MyGUI::Widget* frame[4] = { c.frame[0], c.frame[1], c.frame[2], c.frame[3] };
    PlaceFrame(frame, 0, 0, width, y);
    c.root->setSize(width, y);

    g_width[slot] = width;
    g_height[slot] = y;
}

bool SafeBuild()
{
    TR_TRY { return BuildUnguarded(); }
    TR_CATCH { return false; }
}

bool SafeLayout(int slot, const HeadNotice* notice)
{
    TR_TRY
    {
        LayoutUnguarded(slot, *notice);
        return true;
    }
    TR_CATCH { return false; }
}

void SafeShowAt(int slot, int left, int top)
{
    TR_TRY
    {
        MyGUI::Widget* root = g_cards[slot].root;
        if (root != NULL)
        {
            root->setPosition(left, top);
            if (!root->getVisible()) root->setVisible(true);
        }
    }
    TR_CATCH { }
}

void SafeHideSlot(int slot)
{
    TR_TRY
    {
        MyGUI::Widget* root = g_cards[slot].root;
        if (root != NULL && root->getVisible()) root->setVisible(false);
    }
    TR_CATCH { }
}

void SafeDestroy()
{
    TR_TRY
    {
        MyGUI::Gui* gui = MyGUI::Gui::getInstancePtr();
        for (int i = 0; i < kHeadNoticeCap; ++i)
        {
            if (gui != NULL && g_cards[i].root != NULL)
            {
                gui->destroyWidget(g_cards[i].root);
            }
        }
    }
    TR_CATCH { }
}

bool EnsureBuilt()
{
    if (g_built)
    {
        return true;
    }
    if (g_buildFailed)
    {
        return false;
    }
    if (!SafeBuild())
    {
        g_buildFailed = true;
        LogError("[notice] could not build the head notice widgets; notices stay hidden.");
        return false;
    }
    g_built = true;
    return true;
}

// Laid out only when its name or line changed; false when that failed.
bool EnsureLayout(int slot, const HeadNotice& notice)
{
    std::string key = notice.name;
    key += '\x1f';
    key += notice.text;
    if (key == g_layoutKey[slot])
    {
        return true;
    }
    if (!SafeLayout(slot, &notice))
    {
        g_layoutKey[slot].clear();
        return false;
    }
    g_layoutKey[slot] = key;
    return true;
}

void HideSlotsFrom(int first)
{
    if (!g_built)
    {
        return;
    }
    for (int i = first; i < kHeadNoticeCap; ++i)
    {
        SafeHideSlot(i);
    }
}

// --------------------------------------------------------------- Per frame
void AdvanceClock()
{
    const DWORD now = GetTickCount();
    const unsigned int elapsed = g_haveTick ? (unsigned int)(now - g_lastTick) : 0u;
    g_lastTick = now;
    g_haveTick = true;
    bool paused = false;
    SafeIsPaused(&paused);
    g_clock = AdvanceEventClock(g_clock, elapsed, paused);
}

// A notice whose NPC died, joined, or whose address now belongs to someone else.
bool SubjectGone(const HeadNotice& notice)
{
    Character* npc = (Character*)notice.subject;
    bool dead = false;
    if (!SafeIsDead(npc, &dead) || dead)
    {
        return true;
    }
    bool mine = false;
    if (!SafeIsPlayerCharacter(npc, &mine) || (mine && g_squadNotices.count(notice.who) == 0))
    {
        return true;
    }
    CharKey key;
    return !SafeFindCharKey(npc, &key) || !(key == notice.who);
}

void DropGoneSubjects()
{
    const DWORD now = GetTickCount();
    if ((LONG)(now - g_nextCheck) < 0)
    {
        return;
    }
    g_nextCheck = now + kCheckIntervalMs;

    std::vector<CharKey> gone;
    for (size_t i = 0; i < g_board.Count(); ++i)
    {
        if (SubjectGone(g_board.At(i)))
        {
            gone.push_back(g_board.At(i).who);
        }
    }
    for (size_t i = 0; i < gone.size(); ++i)
    {
        g_board.Dismiss(gone[i]);
    }
}

// The key of the NPC the player selected this frame, when the selection changed.
bool SelectionChangedTo(CharKey* out)
{
    Character* selected = SelectedNpc();
    if (selected == g_lastSelection)
    {
        return false;
    }
    g_lastSelection = selected;
    return selected != NULL && SafeFindCharKey(selected, out);
}

void DrawNotices(bool haveSelected, const CharKey& selectedKey)
{
    NoticeFrameInput base;
    base.menuOpen = IsContextMenuOpen();
    base.nativeWindowOpen = NativeWindowCoversOverlays();
    base.cardVisible = ReadoutCardOnScreen(&base.card);
    ScreenRect offer;
    const bool haveOffer =
        IsOfferOpen() && OfferWindowRect(&offer.left, &offer.top, &offer.width, &offer.height);

    CardMetrics metrics;
    metrics.gap = kArrowGap;
    metrics.margin = kScreenMargin;
    metrics.belowGap = kArrowGap * 4;

    std::vector<CharKey> dismissed;
    const int count = (int)g_board.Count();
    for (int i = 0; i < count; ++i)
    {
        const HeadNotice& notice = g_board.At(i);
        NoticeFrameInput in = base;
        in.selectedNow = haveSelected && notice.who == selectedKey;

        int left = 0, top = 0;
        if (!in.selectedNow && !in.menuOpen && !in.nativeWindowOpen && EnsureLayout(i, notice))
        {
            const AnchorReading anchor = ReadoutAnchorFor((Character*)notice.subject);
            if (anchor.result == Anchor_Ok)
            {
                const CardPlacement placement =
                    PlaceCard(anchor.x, anchor.y, g_width[i], g_height[i], anchor.viewW,
                              anchor.viewH, haveOffer ? &offer : NULL, metrics, &left, &top);
                in.placed = placement != Placement_NoRoom;
                in.notice = ScreenRect(left, top, g_width[i], g_height[i]);
            }
        }

        switch (DecideNoticeFrame(in))
        {
        case NoticeFrame_Draw:
            SafeShowAt(i, left, top);
            break;
        case NoticeFrame_Dismiss:
            dismissed.push_back(notice.who);
            SafeHideSlot(i);
            break;
        case NoticeFrame_Hide:
        default:
            SafeHideSlot(i);
            break;
        }
    }
    HideSlotsFrom(count);

    for (size_t i = 0; i < dismissed.size(); ++i)
    {
        g_board.Dismiss(dismissed[i]);
        if (GetConfig().probeOnClick)
        {
            LogInfo("[notice] dismissed by selection [key " + dismissed[i].ToString() + "]");
        }
    }
}

void Raise(Character* npc, const std::string& text, bool squad, HeadNoticeKind kind)
{
    const Config& config = GetConfig();
    if (npc == NULL || !config.enabled)
    {
        return;
    }
    CharKey key;
    if (!SafeCharKey(npc, &key))
    {
        return;
    }
    if (squad)
    {
        g_squadNotices.insert(key);
    }
    else
    {
        g_squadNotices.erase(key);
    }
    const std::string name = CharacterName(npc);
    const float seconds =
        HeadNoticeSecondsFor(kind, config.headNoticeSeconds, config.headNoticeInfoSeconds);
    g_board.Raise(key, npc, name, text, g_clock, seconds);
    LogInfo("[notice] " + name + ": \"" + text + "\" over the head [key " + key.ToString() + "]" +
            (squad ? " (squad)" : "") + (kind == HeadNoticeKind_Info ? " (info)" : " (prompt)"));
}
} // namespace

void RaiseHeadNotice(Character* npc, const std::string& text, HeadNoticeKind kind)
{
    Raise(npc, text, false, kind);
}

void RaiseSquadHeadNotice(Character* member, const std::string& text, HeadNoticeKind kind)
{
    Raise(member, text, true, kind);
}

void DismissHeadNotice(Character* npc)
{
    CharKey key;
    if (npc != NULL && SafeFindCharKey(npc, &key))
    {
        g_board.Dismiss(key);
    }
}

void HeadNoticeTick()
{
    AdvanceClock();

    // Read every frame, notices or not, so the frame the player selects someone is
    // caught even when that NPC's notice was raised while they were selected.
    CharKey selectedKey;
    const bool haveSelected = SelectionChangedTo(&selectedKey);

    if (!GetConfig().enabled)
    {
        g_board.Clear();
    }
    g_board.Expire(g_clock);
    if (g_board.Count() == 0)
    {
        HideSlotsFrom(0);
        return;
    }

    DropGoneSubjects();
    if (!EnsureBuilt())
    {
        return;
    }
    DrawNotices(haveSelected, selectedKey);
}

void ForgetHeadNotices()
{
    g_board.Clear();
    g_squadNotices.clear();
    g_lastSelection = NULL;
    g_nextCheck = 0;
    HideSlotsFrom(0);
}

void ShutdownHeadNotices()
{
    ForgetHeadNotices();
    if (g_built)
    {
        SafeDestroy();
    }
    for (int i = 0; i < kHeadNoticeCap; ++i)
    {
        NoticeCard empty = { 0 };
        g_cards[i] = empty;
        g_layoutKey[i].clear();
    }
    g_built = false;
    g_buildFailed = false;
}
} // namespace totalrecruit
