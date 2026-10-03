// Tests for src/head_notice_rules.cpp: who has a "wants to talk" card over the
// head, for how long, and when the normal readout card takes over.

#include "test_support.h"
#include "head_notice_rules.h"
#include "config.h"

using namespace totalrecruit;

namespace
{
CharKey Key(unsigned int i)
{
    return CharKey(i, 1);
}

void ARaisedNoticeExpiresAfterItsSeconds()
{
    const char* name = "ARaisedNoticeExpiresAfterItsSeconds";
    HeadNoticeBoard board;
    board.Raise(Key(1), 0, "Ana", "Quiere hablar contigo", 10.0f, 300.0f);
    CHECK(board.Count() == 1);
    CHECK(board.Find(Key(1)) != 0);
    CHECK(board.Find(Key(1))->expiresAt == 310.0f);
    CHECK(board.Find(Key(1))->name == "Ana");
    CHECK(board.Find(Key(1))->text == "Quiere hablar contigo");
    // Still up one second before, gone at the second it ends.
    CHECK(board.Expire(309.0f) == 0);
    CHECK(board.Count() == 1);
    CHECK(board.Expire(310.0f) == 1);
    CHECK(board.Count() == 0);
    CHECK(board.Find(Key(1)) == 0);
}

void RaisingAgainReplacesAndRenews()
{
    const char* name = "RaisingAgainReplacesAndRenews";
    HeadNoticeBoard board;
    board.Raise(Key(1), 0, "Ana", "Quiere hablar contigo", 0.0f, 300.0f);
    board.Raise(Key(2), 0, "Bo", "Quiere hablar contigo", 5.0f, 300.0f);
    board.Raise(Key(1), 0, "Ana", "Se acerca para hablar contigo", 100.0f, 300.0f);
    // One per NPC, the new text, a fresh expiry, and now the newest.
    CHECK(board.Count() == 2);
    CHECK(board.Find(Key(1))->text == "Se acerca para hablar contigo");
    CHECK(board.Find(Key(1))->expiresAt == 400.0f);
    CHECK(board.At(0).who == Key(2));
    CHECK(board.At(1).who == Key(1));
    // The first raise's expiry no longer applies.
    board.Expire(300.0f);
    CHECK(board.Find(Key(1)) != 0);
}

void AFullBoardDropsTheOldest()
{
    const char* name = "AFullBoardDropsTheOldest";
    HeadNoticeBoard board;
    for (int i = 0; i < kHeadNoticeCap; ++i)
    {
        board.Raise(Key((unsigned int)i + 1), 0, "n", "t", (float)i, 300.0f);
    }
    CHECK(board.Count() == (size_t)kHeadNoticeCap);
    board.Raise(Key(100), 0, "new", "t", 50.0f, 300.0f);
    CHECK(board.Count() == (size_t)kHeadNoticeCap);
    CHECK(board.Find(Key(1)) == 0);
    CHECK(board.Find(Key(2)) != 0);
    CHECK(board.Find(Key(100)) != 0);
    // Replacing someone already on a full board drops nobody.
    board.Raise(Key(2), 0, "n", "t2", 60.0f, 300.0f);
    CHECK(board.Count() == (size_t)kHeadNoticeCap);
    CHECK(board.Find(Key(3)) != 0);
}

void DismissDropsOnlyThatNpc()
{
    const char* name = "DismissDropsOnlyThatNpc";
    HeadNoticeBoard board;
    board.Raise(Key(1), 0, "Ana", "t", 0.0f, 300.0f);
    board.Raise(Key(2), 0, "Bo", "t", 0.0f, 300.0f);
    CHECK(board.Dismiss(Key(1)));
    CHECK(!board.Dismiss(Key(1)));
    CHECK(board.Find(Key(1)) == 0);
    CHECK(board.Find(Key(2)) != 0);
    // A reused slot (same index, new serial) is a stranger.
    CHECK(!board.Dismiss(CharKey(2, 9)));
    CHECK(board.Count() == 1);
    board.Clear();
    CHECK(board.Count() == 0);
}

void ExpireTakesOnlyTheDueOnes()
{
    const char* name = "ExpireTakesOnlyTheDueOnes";
    HeadNoticeBoard board;
    board.Raise(Key(1), 0, "a", "t", 0.0f, 10.0f);
    board.Raise(Key(2), 0, "b", "t", 0.0f, 300.0f);
    board.Raise(Key(3), 0, "c", "t", 0.0f, 20.0f);
    CHECK(board.Expire(25.0f) == 2);
    CHECK(board.Count() == 1);
    CHECK(board.Find(Key(2)) != 0);
}

NoticeFrameInput Visible()
{
    NoticeFrameInput in;
    in.placed = true;
    in.notice = ScreenRect(100, 100, 150, 40);
    return in;
}

void SelectingThatNpcDismissesTheNotice()
{
    const char* name = "SelectingThatNpcDismissesTheNotice";
    NoticeFrameInput in = Visible();
    CHECK(DecideNoticeFrame(in) == NoticeFrame_Draw);
    in.selectedNow = true;
    CHECK(DecideNoticeFrame(in) == NoticeFrame_Dismiss);
    // Even when it could not be drawn this frame.
    in.placed = false;
    in.menuOpen = true;
    CHECK(DecideNoticeFrame(in) == NoticeFrame_Dismiss);
}

void TheNormalCardWinsACollision()
{
    const char* name = "TheNormalCardWinsACollision";
    NoticeFrameInput in = Visible();
    in.cardVisible = true;
    in.card = ScreenRect(200, 120, 264, 150);
    CHECK(DecideNoticeFrame(in) == NoticeFrame_Hide);
    // Another head elsewhere on the screen: both shown.
    in.card = ScreenRect(600, 120, 264, 150);
    CHECK(DecideNoticeFrame(in) == NoticeFrame_Draw);
    // A card that is not on the screen does not count.
    in.card = ScreenRect(200, 120, 264, 150);
    in.cardVisible = false;
    CHECK(DecideNoticeFrame(in) == NoticeFrame_Draw);
}

void UnplacedOrUnderAMenuIsHiddenNotDropped()
{
    const char* name = "UnplacedOrUnderAMenuIsHiddenNotDropped";
    NoticeFrameInput in = Visible();
    in.placed = false;
    CHECK(DecideNoticeFrame(in) == NoticeFrame_Hide);
    in = Visible();
    in.menuOpen = true;
    CHECK(DecideNoticeFrame(in) == NoticeFrame_Hide);
}

void RectsOverlapOnSharedPixelsOnly()
{
    const char* name = "RectsOverlapOnSharedPixelsOnly";
    const ScreenRect a(0, 0, 10, 10);
    CHECK(RectsOverlap(a, ScreenRect(9, 9, 5, 5)));
    CHECK(RectsOverlap(a, ScreenRect(2, 2, 2, 2)));
    // Touching edges share no pixel.
    CHECK(!RectsOverlap(a, ScreenRect(10, 0, 5, 5)));
    CHECK(!RectsOverlap(a, ScreenRect(0, 10, 5, 5)));
    CHECK(!RectsOverlap(a, ScreenRect(3, 3, 0, 5)));
}

void SecondsAreClamped()
{
    const char* name = "SecondsAreClamped";
    CHECK(ClampHeadNoticeSeconds(300.0f) == 300.0f);
    CHECK(ClampHeadNoticeSeconds(1.0f) == kHeadNoticeSecondsMin);
    CHECK(ClampHeadNoticeSeconds(-5.0f) == kHeadNoticeSecondsMin);
    CHECK(ClampHeadNoticeSeconds(99999.0f) == kHeadNoticeSecondsMax);
    CHECK(kHeadNoticeSecondsDefault == 300.0f);
}

void TheConfigDefaultIsFiveMinutes()
{
    const char* name = "TheConfigDefaultIsFiveMinutes";
    const Config config;
    CHECK(config.headNoticeSeconds == kHeadNoticeSecondsDefault);
    // A different value reaches the expiry.
    HeadNoticeBoard board;
    board.Raise(Key(1), 0, "a", "t", 0.0f, ClampHeadNoticeSeconds(60.0f));
    CHECK(board.Find(Key(1))->expiresAt == 60.0f);
}
} // namespace

void RunHeadNoticeRulesTests()
{
    ARaisedNoticeExpiresAfterItsSeconds();
    RaisingAgainReplacesAndRenews();
    AFullBoardDropsTheOldest();
    DismissDropsOnlyThatNpc();
    ExpireTakesOnlyTheDueOnes();
    SelectingThatNpcDismissesTheNotice();
    TheNormalCardWinsACollision();
    UnplacedOrUnderAMenuIsHiddenNotDropped();
    RectsOverlapOnSharedPixelsOnly();
    SecondsAreClamped();
    TheConfigDefaultIsFiveMinutes();
}
