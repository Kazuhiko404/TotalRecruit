// Tests for src/submission_tracker.cpp: who was asked, whose roll failed today,
// and the reject cooldown that rides in the save.

#include "test_support.h"
#include "submission_tracker.h"

using namespace totalrecruit;

namespace
{
const CharKey kAlice = CharKey::FromId("alice");
const CharKey kBob = CharKey::FromId("bob");

void AskedOnceUntilBeatenAgain()
{
    const char* name = "memory: asked at 3 defeats, asks again only past 3";
    SubmissionMemory m;
    CHECK(m.MayAsk(kAlice, 0));
    m.MarkAsked(kAlice, 3);
    CHECK(!m.MayAsk(kAlice, 3));
    CHECK(m.MayAsk(kAlice, 4));
    CHECK(m.MayAsk(kBob, 0));
}

void AFailedRollHoldsForTheDay()
{
    const char* name = "memory: a failed roll is remembered for its day only";
    SubmissionMemory m;
    CHECK(!m.RollFailedOn(kAlice, 7));
    m.NoteRollFailed(kAlice, 7);
    CHECK(m.RollFailedOn(kAlice, 7));
    CHECK(!m.RollFailedOn(kAlice, 8));
    CHECK(!m.RollFailedOn(kBob, 7));
}

void TheCooldownIsKeptAndForgotten()
{
    const char* name = "memory: a cooldown end hour is kept; 0 forgets it; clear drops all";
    SubmissionMemory m;
    CHECK(m.CooldownUntil(kAlice) == 0);
    m.SetCooldownUntil(kAlice, 172);
    CHECK(m.CooldownUntil(kAlice) == 172);
    m.SetCooldownUntil(kAlice, 0);
    CHECK(m.CooldownUntil(kAlice) == 0);
    m.SetCooldownUntil(kAlice, 172);
    m.MarkAsked(kBob, 2);
    m.Clear();
    CHECK(m.CooldownUntil(kAlice) == 0);
    CHECK(m.MayAsk(kBob, 2));
}

void TheCooldownRidesInTheSave()
{
    const char* name = "memory: the save bridge writes the end hour and reads it back sanitised";
    ForgetAllSubmissionMemory();
    SetRejectCooldownUntil(kAlice, 172);
    CHECK(SavedRejectCooldown("alice") == 172);
    CHECK(SavedRejectCooldown("bob") == 0);
    CHECK(SavedRejectCooldown("") == 0);

    ForgetAllSubmissionMemory();
    CHECK(RejectCooldownUntil(kAlice) == 0);
    RestoreSavedRejectCooldown("alice", 172);
    CHECK(RejectCooldownUntil(kAlice) == 172);
    RestoreSavedRejectCooldown("bob", -3);
    CHECK(RejectCooldownUntil(kBob) == 0);
    RestoreSavedRejectCooldown("", 50);
    CHECK(RejectCooldownUntil(CharKey::FromId("")) == 0);
    ForgetAllSubmissionMemory();
}

void AnOldSaveHeldTheWholeHour()
{
    const char* name = "memory: the legacy whole-hour reject row reads back as that hour's minute; the minute row wins";
    ForgetAllSubmissionMemory();
    RestoreSavedRejectCooldownLegacyHour("alice", 172);
    CHECK(RejectCooldownUntil(kAlice) == 172 * 60);
    CHECK(SavedRejectCooldownLegacyHour("alice") == 172);
    RestoreSavedRejectCooldownLegacyHour("bob", 11);
    RestoreSavedRejectCooldown("bob", 616);
    CHECK(RejectCooldownUntil(kBob) == 616);
    CHECK(SavedRejectCooldownLegacyHour("bob") == 11);
    RestoreSavedRejectCooldownLegacyHour("", 9);
    CHECK(SavedRejectCooldownLegacyHour("") == 0);
    ForgetAllSubmissionMemory();
}

void TheSessionMarksGoThroughTheSharedInstance()
{
    const char* name = "memory: the process-wide asked and rolled marks";
    ForgetAllSubmissionMemory();
    MarkSubmissionAsked(kAlice, 2);
    CHECK(!MaySubmissionAsk(kAlice, 2));
    NoteSubmissionRollFailed(kBob, 3);
    CHECK(SubmissionRollFailedOn(kBob, 3));
    ForgetAllSubmissionMemory();
    CHECK(MaySubmissionAsk(kAlice, 2));
    CHECK(!SubmissionRollFailedOn(kBob, 3));
}
} // namespace

void RunSubmissionTrackerTests()
{
    AskedOnceUntilBeatenAgain();
    AFailedRollHoldsForTheDay();
    TheCooldownIsKeptAndForgotten();
    TheCooldownRidesInTheSave();
    AnOldSaveHeldTheWholeHour();
    TheSessionMarksGoThroughTheSharedInstance();
}
