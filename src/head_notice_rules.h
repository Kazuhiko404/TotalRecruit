#pragma once

// Who has a notice over their head, without the engine.
//
// When an NPC calls out to the player (a ready captive's hail, a beaten NPC walking
// up to ask to join), the roller says "X quiere hablar contigo" -- and a player who
// did not read the name does not know who. So that NPC also carries a small card
// over the head (head_notice.h) for a configurable while (headNoticeSeconds).
//
// The board keeps at most kHeadNoticeCap notices, one per NPC: raising one for an
// NPC who already has one replaces it (new text, fresh expiry, newest); raising one
// for a new NPC on a full board drops the oldest. A notice goes when its time is up,
// when the player selects that NPC (the normal readout card takes over, so the two
// never overlap), or when the engine side drops it (death, joined, load).
//
// Times are the notice clock's seconds: real seconds, frozen while the game is
// paused (head_notice.cpp advances it like the submission director's clock).
//
// head_notice.cpp owns the board and every engine read; everything here is plain
// values. `subject` is the engine's character pointer, carried opaquely so the
// engine side can draw the notice; nothing here dereferences it.

#include "char_key.h"
#include "screen_projection.h"

#include <string>
#include <vector>

namespace totalrecruit
{
const int kHeadNoticeCap = 8;

// headNoticeSeconds: default and the clamp applied when config.json is read.
const float kHeadNoticeSecondsDefault = 300.0f;
const float kHeadNoticeSecondsMin = 10.0f;
const float kHeadNoticeSecondsMax = 3600.0f;

struct HeadNotice
{
    CharKey who;
    // Opaque: the engine's Character*, never dereferenced by the rules.
    const void* subject;
    // The NPC's name and the line under it, worked out once when raised.
    std::string name;
    std::string text;
    float raisedAt;
    float expiresAt;

    HeadNotice() : subject(0), raisedAt(0.0f), expiresAt(0.0f) {}
};

class HeadNoticeBoard
{
public:
    // Raises (or replaces) the notice for `who`, expiring `seconds` after `now`. The
    // notice becomes the newest. A new NPC on a full board drops the oldest first.
    void Raise(const CharKey& who, const void* subject, const std::string& name,
               const std::string& text, float now, float seconds);

    // Drops the notice for `who`. True when there was one.
    bool Dismiss(const CharKey& who);

    // Drops every notice whose time is up (expiresAt <= now). Returns how many went.
    int Expire(float now);

    // The notice for `who`, or NULL.
    const HeadNotice* Find(const CharKey& who) const;

    // Oldest first.
    size_t Count() const;
    const HeadNotice& At(size_t index) const;

    void Clear();

private:
    std::vector<HeadNotice> notices_;
};

// What the engine side does with one notice this frame.
enum NoticeFrame
{
    // Drawn over the head.
    NoticeFrame_Draw,
    // Kept, not drawn this frame (off the screen, a right-click menu open, no room
    // beside the offer window, or the normal readout card is in the way).
    NoticeFrame_Hide,
    // Gone for good: the player selected this NPC.
    NoticeFrame_Dismiss
};

struct NoticeFrameInput
{
    // The player's selection changed this frame to this notice's NPC.
    bool selectedNow;
    // The head projects onto the screen and the card has a place there.
    bool placed;
    // A right-click menu is open (the readout card hides under it too).
    bool menuOpen;
    // The normal readout card is on the screen, at `card`.
    bool cardVisible;
    ScreenRect card;
    // Where this notice would be drawn.
    ScreenRect notice;

    NoticeFrameInput()
        : selectedNow(false), placed(false), menuOpen(false), cardVisible(false), card(),
          notice()
    {
    }
};

// Selection dismisses; otherwise drawn only when placed, no menu is open and it does
// not touch the normal readout card (the normal card wins).
NoticeFrame DecideNoticeFrame(const NoticeFrameInput& input);

// True when the two rectangles share any pixel. Empty rectangles never overlap.
bool RectsOverlap(const ScreenRect& a, const ScreenRect& b);

// headNoticeSeconds as read from config.json, pulled back into
// [kHeadNoticeSecondsMin, kHeadNoticeSecondsMax].
float ClampHeadNoticeSeconds(float seconds);
} // namespace totalrecruit
