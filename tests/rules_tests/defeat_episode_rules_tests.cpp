// Tests for src/defeat_episode_rules.cpp: one defeat per knockout, re-armed only
// once the character has been seen standing again, and never twice inside the
// echo window.

#include "test_support.h"
#include "defeat_episode_rules.h"
#include "defeat_rules.h"

#include <cstring>

using namespace totalrecruit;
using namespace totalrecruit::DefeatEpisode;

namespace
{
// Real-time milliseconds far enough apart that the echo window never joins them.
const unsigned int kT0 = 1000;
const unsigned int kLater = kT0 + 10u * 60u * 1000u;

// The bug this replaces: a long knockout outlived the 0.5 h debounce and the
// right-click counted it a second time.
void OneKnockoutCountsOnceAcrossTheHookAndRightClicks()
{
    const char* name = "episode: one knockout counts once across hook echoes and right-clicks";
    State s;
    const Verdict fall = Judge(s, Sighting_KnockoutFromStanding, kT0);
    CHECK(fall == Verdict_CountNewEpisode);
    CHECK(Counts(fall));
    s = AfterCount(s, kT0);
    CHECK(s.countedThisEpisode);

    // The body settling raises more knockout calls, already down.
    CHECK(Judge(s, Sighting_FoundDown, kLater) == Verdict_AlreadyCounted);
    // The player right-clicks the body again and again.
    for (int i = 0; i < 5; ++i)
    {
        CHECK(!Counts(Judge(s, Sighting_FoundDown, kLater)));
    }
}

void SeenStandingReArmsTheNextKnockout()
{
    const char* name = "episode: seen standing again, the next time down counts";
    State s = AfterCount(State(), kT0);
    CHECK(Judge(s, Sighting_FoundDown, kLater) == Verdict_AlreadyCounted);

    s = NoteStanding(s);
    CHECK(!s.countedThisEpisode);
    CHECK(s.seenStanding);
    const Verdict again = Judge(s, Sighting_FoundDown, kLater);
    CHECK(again == Verdict_CountAfterStanding);
    CHECK(Counts(again));

    s = AfterCount(s, kT0);
    CHECK(!s.seenStanding);
    CHECK(Judge(s, Sighting_FoundDown, kLater) == Verdict_AlreadyCounted);
}

// The hook reads them standing just before the native knockout runs, past the echo
// window: they got up while nobody was looking, and this is a second beating.
void TheHookFromStandingAlwaysOpensANewEpisode()
{
    const char* name = "episode: a knockout from standing (past the echo window) counts even when nobody saw them get up";
    const State counted = AfterCount(State(), kT0);
    CHECK(Judge(counted, Sighting_KnockoutFromStanding, kLater) == Verdict_CountNewEpisode);
}

void FirstSeenDownCountsOnce()
{
    const char* name = "episode: someone first seen already down counts once";
    State s;
    const Verdict first = Judge(s, Sighting_FoundDown, kLater);
    CHECK(first == Verdict_CountFirstSeenDown);
    CHECK(Counts(first));
    s = AfterCount(s, kT0);
    CHECK(Judge(s, Sighting_FoundDown, kLater) == Verdict_AlreadyCounted);
}

// The credit is asked first and a skipped credit leaves the episode alone, so a
// first-seen-down body counts only when the engine names the player.
void FirstSeenDownNeedsThePlayersCredit()
{
    const char* name = "episode: first seen down counts only with the player's credit";
    const Verdict first = Judge(State(), Sighting_FoundDown, kLater);
    CHECK(CountsForPlayer(DefeatCredit_Counts, first));
    CHECK(!CountsForPlayer(DefeatCredit_SkipNotThePlayer, first));
    CHECK(!CountsForPlayer(DefeatCredit_SkipWinnerUnreadable, first));
    CHECK(!CountsForPlayer(DefeatCredit_SkipPlayersOwn, first));
    CHECK(!CountsForPlayer(DefeatCredit_Counts, Verdict_AlreadyCounted));
}

// GOLPEARLO: the forced knockout may raise the hook, then the beating credits the
// player explicitly. Whichever comes first counts; the other is the same episode.
void TheBeatingCountsOnceWhetherOrNotTheHookCountedIt()
{
    const char* name = "episode: the beating counts once, hook first or credit first";
    State hookCounted = AfterCount(State(), kT0);
    CHECK(Judge(hookCounted, Sighting_FoundDown, kLater) == Verdict_AlreadyCounted);

    // The hook's credit was skipped: nothing counted, the explicit credit does.
    const State hookSkipped = NoteStanding(State());
    CHECK(Counts(Judge(hookSkipped, Sighting_FoundDown, kLater)));
}

// The bug this fixes: one knockout fires the hook twice ~1.2 s apart and both
// calls read the victim standing just before the native knockout ran, so the
// second opened a "new episode" and every knockout counted two defeats.
void TheHookTwiceInTheEchoWindowIsOneDefeat()
{
    const char* name = "episode: two hook calls 1.2 s apart, both 'was standing', count once";
    State s;
    const Verdict first = Judge(s, Sighting_KnockoutFromStanding, kT0);
    CHECK(Counts(first));
    s = AfterCount(s, kT0);

    const Verdict echo = Judge(s, Sighting_KnockoutFromStanding, kT0 + 1200);
    CHECK(echo == Verdict_AlreadyCounted);
    CHECK(!Counts(echo));
    CHECK(!Counts(Judge(s, Sighting_KnockoutFromStanding, kT0 + kKnockoutEchoWindowMs - 1)));
}

// The sweep or the card may read them standing between the two calls (that is why
// the hook read them standing too): the echo window still holds.
void SeenStandingInsideTheEchoWindowDoesNotReArm()
{
    const char* name = "episode: seen standing between the echoes does not open a new episode";
    State s = AfterCount(State(), kT0);
    s = NoteStanding(s);
    CHECK(Judge(s, Sighting_KnockoutFromStanding, kT0 + 1200) == Verdict_AlreadyCounted);
    CHECK(Judge(s, Sighting_FoundDown, kT0 + 1200) == Verdict_AlreadyCounted);
    // Past the window the standing re-arm counts again, as before.
    CHECK(Judge(s, Sighting_FoundDown, kT0 + kKnockoutEchoWindowMs) == Verdict_CountAfterStanding);
}

void AKnockoutAfterTheEchoWindowIsANewEpisode()
{
    const char* name = "episode: a fall from standing after the echo window is a second defeat";
    State s = AfterCount(State(), kT0);
    CHECK(Judge(s, Sighting_KnockoutFromStanding, kT0 + kKnockoutEchoWindowMs) ==
          Verdict_CountNewEpisode);
    CHECK(kKnockoutEchoWindowMs == 5000);
}

void TheEchoWindowSurvivesATickWrap()
{
    const char* name = "episode: the echo window is measured across a GetTickCount wrap";
    unsigned int nearWrap = 0xFFFFFC00u;  // not const: the wrap is the point
    State s = AfterCount(State(), nearWrap);
    CHECK(Judge(s, Sighting_KnockoutFromStanding, nearWrap + 1200u) == Verdict_AlreadyCounted);
    CHECK(Judge(s, Sighting_KnockoutFromStanding, nearWrap + 6000u) == Verdict_CountNewEpisode);
}

void VerdictNamesAreDistinct()
{
    const char* name = "episode: every verdict has its own log name";
    const Verdict all[] = { Verdict_CountNewEpisode, Verdict_CountAfterStanding,
                            Verdict_CountFirstSeenDown, Verdict_AlreadyCounted };
    const int n = sizeof(all) / sizeof(all[0]);
    for (int i = 0; i < n; ++i)
    {
        CHECK(VerdictName(all[i])[0] != '?');
        for (int j = i + 1; j < n; ++j)
        {
            CHECK(std::strcmp(VerdictName(all[i]), VerdictName(all[j])) != 0);
        }
    }
    CHECK(!Counts(Verdict_AlreadyCounted));
}
} // namespace

void RunDefeatEpisodeRulesTests()
{
    OneKnockoutCountsOnceAcrossTheHookAndRightClicks();
    SeenStandingReArmsTheNextKnockout();
    TheHookFromStandingAlwaysOpensANewEpisode();
    FirstSeenDownCountsOnce();
    FirstSeenDownNeedsThePlayersCredit();
    TheBeatingCountsOnceWhetherOrNotTheHookCountedIt();
    TheHookTwiceInTheEchoWindowIsOneDefeat();
    SeenStandingInsideTheEchoWindowDoesNotReArm();
    AKnockoutAfterTheEchoWindowIsANewEpisode();
    TheEchoWindowSurvivesATickWrap();
    VerdictNamesAreDistinct();
}
