// Tests for src/defeat_episode_rules.cpp: one defeat per knockout, re-armed only
// once the character has been seen standing again.

#include "test_support.h"
#include "defeat_episode_rules.h"
#include "defeat_rules.h"

#include <cstring>

using namespace totalrecruit;
using namespace totalrecruit::DefeatEpisode;

namespace
{
// The bug this replaces: a long knockout outlived the 0.5 h debounce and the
// right-click counted it a second time.
void OneKnockoutCountsOnceAcrossTheHookAndRightClicks()
{
    const char* name = "episode: one knockout counts once across hook echoes and right-clicks";
    State s;
    const Verdict fall = Judge(s, Sighting_KnockoutFromStanding);
    CHECK(fall == Verdict_CountNewEpisode);
    CHECK(Counts(fall));
    s = AfterCount(s);
    CHECK(s.countedThisEpisode);

    // The body settling raises more knockout calls, already down.
    CHECK(Judge(s, Sighting_FoundDown) == Verdict_AlreadyCounted);
    // The player right-clicks the body again and again.
    for (int i = 0; i < 5; ++i)
    {
        CHECK(!Counts(Judge(s, Sighting_FoundDown)));
    }
}

void SeenStandingReArmsTheNextKnockout()
{
    const char* name = "episode: seen standing again, the next time down counts";
    State s = AfterCount(State());
    CHECK(Judge(s, Sighting_FoundDown) == Verdict_AlreadyCounted);

    s = NoteStanding(s);
    CHECK(!s.countedThisEpisode);
    CHECK(s.seenStanding);
    const Verdict again = Judge(s, Sighting_FoundDown);
    CHECK(again == Verdict_CountAfterStanding);
    CHECK(Counts(again));

    s = AfterCount(s);
    CHECK(!s.seenStanding);
    CHECK(Judge(s, Sighting_FoundDown) == Verdict_AlreadyCounted);
}

// The hook reads them standing just before the native knockout runs: they got up
// while nobody was looking, and this is a second beating.
void TheHookFromStandingAlwaysOpensANewEpisode()
{
    const char* name = "episode: a knockout from standing counts even when nobody saw them get up";
    const State counted = AfterCount(State());
    CHECK(Judge(counted, Sighting_KnockoutFromStanding) == Verdict_CountNewEpisode);
}

void FirstSeenDownCountsOnce()
{
    const char* name = "episode: someone first seen already down counts once";
    State s;
    const Verdict first = Judge(s, Sighting_FoundDown);
    CHECK(first == Verdict_CountFirstSeenDown);
    CHECK(Counts(first));
    s = AfterCount(s);
    CHECK(Judge(s, Sighting_FoundDown) == Verdict_AlreadyCounted);
}

// The credit is asked first and a skipped credit leaves the episode alone, so a
// first-seen-down body counts only when the engine names the player.
void FirstSeenDownNeedsThePlayersCredit()
{
    const char* name = "episode: first seen down counts only with the player's credit";
    const Verdict first = Judge(State(), Sighting_FoundDown);
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
    State hookCounted = AfterCount(State());
    CHECK(Judge(hookCounted, Sighting_FoundDown) == Verdict_AlreadyCounted);

    // The hook's credit was skipped: nothing counted, the explicit credit does.
    const State hookSkipped = NoteStanding(State());
    CHECK(Counts(Judge(hookSkipped, Sighting_FoundDown)));
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
    VerdictNamesAreDistinct();
}
