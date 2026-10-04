// Tests for src/helper_rules.cpp: the free helper -- who may offer, the daily roll and
// the days, the contract's step (expiry, warning, the ways it ends), the native
// contract's hours and watch, the fallback escort (pace, spot, assist renewal), who it
// fights for the player, and the table of helpers (HelperBook).

#include "test_support.h"
#include "helper_rules.h"
#include "temp_member_rules.h"

using namespace totalrecruit;

namespace
{
const Helpers kConfig = Helpers();
const float kFriendly = 20.0f;

// Someone who passes every check: an allied, Good, level-60 NPC 10 m from a level-20
// leader, in Balanced.
HelperCandidate Eligible()
{
    HelperCandidate c;
    c.enabled = true;
    c.balanced = true;
    c.up = true;
    c.relationKnown = true;
    c.relation = 50.0f;
    c.temperament = Temperament_Honorable;
    c.npcLevel = 60;
    c.leaderLevel = 20;
    c.distanceMeters = 10.0f;
    return c;
}

CharKey Key(const char* id)
{
    return CharKey::FromId(id);
}

HelperSignals At(double hours)
{
    HelperSignals s;
    s.nowHours = hours;
    return s;
}

void AnEligibleAllyPassesTheScreen()
{
    const char* name = "screen: an allied, Good, much stronger NPC nearby passes";
    CHECK(ScreenHelperCandidate(Eligible(), kConfig, kFriendly) == HelperSkip_None);
}

void EachCheckSkipsWithItsOwnReason()
{
    const char* name = "screen: each failed check names itself";
    HelperCandidate c = Eligible();
    c.enabled = false;
    CHECK(ScreenHelperCandidate(c, kConfig, kFriendly) == HelperSkip_Disabled);

    c = Eligible();
    c.balanced = false;
    CHECK(ScreenHelperCandidate(c, kConfig, kFriendly) == HelperSkip_NotBalanced);

    c = Eligible();
    c.mine = true;
    CHECK(ScreenHelperCandidate(c, kConfig, kFriendly) == HelperSkip_Mine);

    c = Eligible();
    c.animal = true;
    CHECK(ScreenHelperCandidate(c, kConfig, kFriendly) == HelperSkip_Animal);

    c = Eligible();
    c.up = false;
    CHECK(ScreenHelperCandidate(c, kConfig, kFriendly) == HelperSkip_Down);

    c = Eligible();
    c.caged = true;
    CHECK(ScreenHelperCandidate(c, kConfig, kFriendly) == HelperSkip_Caged);

    c = Eligible();
    c.busy = true;
    CHECK(ScreenHelperCandidate(c, kConfig, kFriendly) == HelperSkip_Busy);

    c = Eligible();
    c.inCombat = true;
    CHECK(ScreenHelperCandidate(c, kConfig, kFriendly) == HelperSkip_InCombat);

    c = Eligible();
    c.distanceMeters = 41.0f;
    CHECK(ScreenHelperCandidate(c, kConfig, kFriendly) == HelperSkip_TooFar);
    c.distanceMeters = -1.0f;
    CHECK(ScreenHelperCandidate(c, kConfig, kFriendly) == HelperSkip_TooFar);

    c = Eligible();
    c.relation = 19.0f;
    CHECK(ScreenHelperCandidate(c, kConfig, kFriendly) == HelperSkip_NotAllied);
    c.relation = 50.0f;
    c.relationKnown = false;
    CHECK(ScreenHelperCandidate(c, kConfig, kFriendly) == HelperSkip_NotAllied);

    c = Eligible();
    c.onCooldown = true;
    CHECK(ScreenHelperCandidate(c, kConfig, kFriendly) == HelperSkip_Cooldown);

    c = Eligible();
    c.rolledToday = true;
    CHECK(ScreenHelperCandidate(c, kConfig, kFriendly) == HelperSkip_RolledToday);

    for (int s = HelperSkip_None; s <= HelperSkip_RolledToday; ++s)
        CHECK(HelperSkipText((HelperSkip)s)[0] != '\0');
}

void OnlyGoodAndNeutralOffer()
{
    const char* name = "screen: Good and Neutral offer, the greedy and the cruel never do";
    CHECK(HelperPersonalityAllowed(Temperament_Honorable));
    CHECK(HelperPersonalityAllowed(Temperament_WarmKind));
    CHECK(HelperPersonalityAllowed(Temperament_Normal));
    CHECK(HelperPersonalityAllowed(Temperament_Dumb));
    CHECK(HelperPersonalityAllowed(Temperament_Fearful));
    CHECK(HelperPersonalityAllowed(Temperament_Brave));
    CHECK(!HelperPersonalityAllowed(Temperament_Traitorous));
    CHECK(!HelperPersonalityAllowed(Temperament_Smart));
    CHECK(!HelperPersonalityAllowed(Temperament_ColdCruel));
    CHECK(!HelperPersonalityAllowed(Temperament_Maniacal));

    HelperCandidate c = Eligible();
    c.temperament = Temperament_ColdCruel;
    CHECK(ScreenHelperCandidate(c, kConfig, kFriendly) == HelperSkip_Personality);
    c.temperament = Temperament_Smart;
    CHECK(ScreenHelperCandidate(c, kConfig, kFriendly) == HelperSkip_Personality);
}

void TheHelperMustOutlevelTheLeaderByTheLead()
{
    const char* name = "screen: level must be over leader + helperMinLevelLead (strictly)";
    HelperCandidate c = Eligible();
    c.leaderLevel = 20;
    c.npcLevel = 31;
    CHECK(ScreenHelperCandidate(c, kConfig, kFriendly) == HelperSkip_None);
    c.npcLevel = 30;
    CHECK(ScreenHelperCandidate(c, kConfig, kFriendly) == HelperSkip_LevelLead);
    c.npcLevel = 5;
    CHECK(ScreenHelperCandidate(c, kConfig, kFriendly) == HelperSkip_LevelLead);

    Helpers noLead;
    noLead.minLevelLead = 0;
    c.npcLevel = 21;
    CHECK(ScreenHelperCandidate(c, noLead, kFriendly) == HelperSkip_None);
    c.npcLevel = 20;
    CHECK(ScreenHelperCandidate(c, noLead, kFriendly) == HelperSkip_LevelLead);
}

void TheRollFollowsTheChance()
{
    const char* name = "roll: seed % 10000 under chance * 10000; 0 never, 1 always";
    CHECK(HelperOfferRoll(1499u, 0.15f));
    CHECK(!HelperOfferRoll(1500u, 0.15f));
    CHECK(HelperOfferRoll(11499u, 0.15f));
    CHECK(!HelperOfferRoll(0u, 0.0f));
    CHECK(HelperOfferRoll(9999u, 1.0f));
    CHECK(!HelperOfferRoll(0u, -1.0f));
    CHECK(HelperOfferRoll(123456u, 2.0f));

    int passes = 0;
    for (unsigned int seed = 0; seed < 10000u; ++seed)
        if (HelperOfferRoll(seed, 0.15f))
            ++passes;
    CHECK(passes == 1500);
}

void TheDaysStayWithinTheRange()
{
    const char* name = "days: daysMin..daysMax inclusive, both ends reached";
    bool sawMin = false, sawMax = false, outside = false;
    for (unsigned int seed = 0; seed < 100u; ++seed)
    {
        const int days = HelperDaysFromSeed(seed, kConfig);
        if (days == kConfig.daysMin) sawMin = true;
        if (days == kConfig.daysMax) sawMax = true;
        if (days < kConfig.daysMin || days > kConfig.daysMax) outside = true;
    }
    CHECK(sawMin && sawMax && !outside);
    CHECK(kConfig.daysMin == 2 && kConfig.daysMax == 5);

    Helpers inverted;
    inverted.daysMin = 6;
    inverted.daysMax = 3;
    CHECK(HelperDaysFromSeed(17u, inverted) == 6);
    Helpers zero;
    zero.daysMin = 0;
    zero.daysMax = 0;
    CHECK(HelperDaysFromSeed(17u, zero) == 1);
}

void TheContractRunsUntilItsHour()
{
    const char* name = "step: stays until the expiry hour, then leaves (expired)";
    HelperContract c = NewHelperContract(TempExpiryHour(1000.0, 2));
    CHECK(c.Active() && c.expiryHour == 1048);
    CHECK(StepHelper(&c, At(1010.0), kConfig).end == HelperEnd_None);
    CHECK(StepHelper(&c, At(1047.9), kConfig).end == HelperEnd_None);
    CHECK(StepHelper(&c, At(1048.0), kConfig).end == HelperEnd_Expired);
}

void TheExpiryWarningComesOnce()
{
    const char* name = "step: 'me ire pronto' once, expiryWarningHours before";
    HelperContract c = NewHelperContract(1048);
    CHECK(!StepHelper(&c, At(1035.0), kConfig).expiryWarn);
    CHECK(StepHelper(&c, At(1036.0), kConfig).expiryWarn);
    CHECK(!StepHelper(&c, At(1040.0), kConfig).expiryWarn);

    Helpers noWarning;
    noWarning.expiryWarningHours = 0.0f;
    HelperContract d = NewHelperContract(1048);
    CHECK(!StepHelper(&d, At(1047.0), noWarning).expiryWarn);
}

void TheSquadTurningOnThemEndsIt()
{
    const char* name = "step: attacked or knocked out by the squad ends it before the expiry";
    HelperContract c = NewHelperContract(1048);
    HelperSignals s = At(1048.0);
    s.attackedBySquad = true;
    CHECK(StepHelper(&c, s, kConfig).end == HelperEnd_Attacked);

    s = At(1010.0);
    s.knockedOutBySquad = true;
    CHECK(StepHelper(&c, s, kConfig).end == HelperEnd_KnockedOut);

    s = At(1010.0);
    s.attackedBySquad = true;
    s.knockedOutBySquad = true;
    CHECK(StepHelper(&c, s, kConfig).end == HelperEnd_KnockedOut);

    CHECK(HelperEndIsBetrayal(HelperEnd_Attacked));
    CHECK(HelperEndIsBetrayal(HelperEnd_KnockedOut));
    CHECK(!HelperEndIsBetrayal(HelperEnd_Expired));
    CHECK(!HelperEndIsBetrayal(HelperEnd_Died));
    CHECK(!HelperEndIsBetrayal(HelperEnd_Joined));
}

void DeathAndJoiningEndItQuietly()
{
    const char* name = "step: dead or recruited into the squad ends the contract first";
    HelperContract c = NewHelperContract(1048);
    HelperSignals s = At(1010.0);
    s.dead = true;
    s.attackedBySquad = true;
    CHECK(StepHelper(&c, s, kConfig).end == HelperEnd_Died);

    s = At(1010.0);
    s.joinedSquad = true;
    CHECK(StepHelper(&c, s, kConfig).end == HelperEnd_Joined);

    for (int e = HelperEnd_None; e <= HelperEnd_Joined; ++e)
        CHECK(HelperEndText((HelperEnd)e)[0] != '\0');
}

void TheNativeContractRunsForTheHoursLeft()
{
    const char* name = "native: contract hours = the whole hours left, rounded up, at least 1";
    CHECK(HelperContractHours(1048, 1000.0) == 48);
    CHECK(HelperContractHours(1048, 1000.5) == 48);
    CHECK(HelperContractHours(1048, 1047.9) == 1);
    CHECK(HelperContractHours(1048, 1048.0) == 1);
    CHECK(HelperContractHours(1048, 2000.0) == 1);
    CHECK(HelperContractHours(0, 10.0) == 1);
}

void TheNativeWatchReassignsOnceThenFallsBack()
{
    const char* name = "native: contract present keeps; gone reassigns once; gone again falls back";
    CHECK(NativeWatchStep(true, false) == NativeWatch_Keep);
    CHECK(NativeWatchStep(true, true) == NativeWatch_Keep);
    CHECK(NativeWatchStep(false, false) == NativeWatch_Reassign);
    CHECK(NativeWatchStep(false, true) == NativeWatch_FallBack);
    CHECK(NativeWatchText(NativeWatch_Keep)[0] != '\0');
    CHECK(NativeWatchText(NativeWatch_Reassign)[0] != '\0');
    CHECK(NativeWatchText(NativeWatch_FallBack)[0] != '\0');
}

void ALoneHelperKeepsItsPlatoon()
{
    const char* name = "native: a helper in a squad of others gets a platoon of its own first";
    CHECK(!HelperNeedsOwnPlatoon(0));
    CHECK(!HelperNeedsOwnPlatoon(1));
    CHECK(HelperNeedsOwnPlatoon(2));
    CHECK(HelperNeedsOwnPlatoon(8));
}

void TheFallbackEscortRepathsEverySecond()
{
    const char* name = "fallback: the escort is re-checked every second";
    CHECK(kHelperEscortRepathMs == 1000u);
}

void TheFallbackRunsWhenFarAndWalksWhenClose()
{
    const char* name = "fallback: near stays, mid walks, far runs, unknown walks to the leader";
    EscortPlan p = PlanEscort(kHelperNearMeters);
    CHECK(!p.move);
    p = PlanEscort(1.0f);
    CHECK(!p.move && !p.run);
    p = PlanEscort(kHelperNearMeters + 0.5f);
    CHECK(p.move && !p.run);
    p = PlanEscort(kHelperRunMeters);
    CHECK(p.move && !p.run);
    p = PlanEscort(kHelperRunMeters + 0.5f);
    CHECK(p.move && p.run);
    p = PlanEscort(200.0f);
    CHECK(p.move && p.run);
    p = PlanEscort(-1.0f);
    CHECK(p.move && !p.run);
    CHECK(kHelperNearMeters < kHelperRunMeters);
}

void EachHelperTakesItsOwnSpotByTheLeader()
{
    const char* name = "fallback: the offset is a small ring around the leader, one spot per slot";
    float x0 = 0.0f, z0 = 0.0f, x1 = 0.0f, z1 = 0.0f, x4 = 0.0f, z4 = 0.0f;
    EscortOffsetMeters(0, &x0, &z0);
    EscortOffsetMeters(1, &x1, &z1);
    EscortOffsetMeters(4, &x4, &z4);
    CHECK(x0 != x1 || z0 != z1);
    CHECK(x0 == x4 && z0 == z4);
    const float r2 = x0 * x0 + z0 * z0;
    CHECK(r2 > 1.0f && r2 <= kHelperEscortOffsetMeters * kHelperEscortOffsetMeters + 0.01f);
    // A spot by the leader must count as "near", or the helper would walk forever.
    CHECK(kHelperEscortOffsetMeters < kHelperNearMeters);
    float xn = 9.0f, zn = 9.0f;
    EscortOffsetMeters(-3, &xn, &zn);
    CHECK(xn * xn + zn * zn > 1.0f);
}

void TheAssistOrderIsRenewedWhenItLapses()
{
    const char* name = "fallback: the attack sequence is re-issued on a new target, out of combat, or every few checks";
    CHECK(ShouldReissueAssist(true, true, 0));
    CHECK(ShouldReissueAssist(false, false, 0));
    CHECK(!ShouldReissueAssist(false, true, 0));
    CHECK(!ShouldReissueAssist(false, true, kHelperAssistReissueChecks - 1));
    CHECK(ShouldReissueAssist(false, true, kHelperAssistReissueChecks));
}

void TheHelperFightsTheLeadersEnemyOnly()
{
    const char* name = "assist: the leader's target when the leader fights; never the squad, itself, or its own";
    AssistFacts a;
    a.leaderInCombat = true;
    a.haveTarget = true;
    CHECK(ShouldAssist(a));

    AssistFacts b = a;
    b.leaderInCombat = false;
    CHECK(!ShouldAssist(b));
    b = a;
    b.haveTarget = false;
    CHECK(!ShouldAssist(b));
    b = a;
    b.targetIsSquad = true;
    CHECK(!ShouldAssist(b));
    b = a;
    b.targetIsHelper = true;
    CHECK(!ShouldAssist(b));
    b = a;
    b.targetSameFaction = true;
    CHECK(!ShouldAssist(b));
    b = a;
    b.targetDown = true;
    CHECK(!ShouldAssist(b));
}

void TheBookKeepsContractsCooldownsAndRolls()
{
    const char* name = "book: start/find/update/end, reject cooldown, one roll a day";
    HelperBook book;
    CHECK(book.Find(Key("a")) == NULL && book.Count() == 0);
    book.Start(Key("a"), 1048);
    CHECK(book.Find(Key("a")) != NULL && book.Find(Key("a"))->expiryHour == 1048);
    CHECK(book.Count() == 1 && book.Keys().size() == 1);

    HelperContract c = *book.Find(Key("a"));
    c.expiryWarned = true;
    book.Update(Key("a"), c);
    CHECK(book.Find(Key("a"))->expiryWarned);
    book.Update(Key("b"), c);
    CHECK(book.Find(Key("b")) == NULL);
    book.End(Key("a"));
    CHECK(book.Find(Key("a")) == NULL && book.Count() == 0);

    CHECK(!book.OnRejectCooldown(Key("c"), 100.0));
    book.SetRejectedUntil(Key("c"), 172);
    CHECK(book.OnRejectCooldown(Key("c"), 171.9));
    CHECK(!book.OnRejectCooldown(Key("c"), 172.0));
    CHECK(book.RejectedUntil(Key("c")) == 172);

    CHECK(!book.RolledOn(Key("d"), 5));
    book.NoteRolled(Key("d"), 5);
    CHECK(book.RolledOn(Key("d"), 5));
    CHECK(!book.RolledOn(Key("d"), 6));
}

void TheSaveBridgeRoundTrips()
{
    const char* name = "book: expiry and reject cooldown saved and restored; 0 or less is none";
    HelperBook book;
    book.Start(Key("a"), 1048);
    book.SetRejectedUntil(Key("b"), 300);
    CHECK(book.SavedExpiry(Key("a")) == 1048);
    CHECK(book.SavedExpiry(Key("b")) == 0);
    CHECK(book.SavedRejectedUntil(Key("b")) == 300);
    CHECK(book.SavedRejectedUntil(Key("a")) == 0);

    HelperBook loaded;
    loaded.RestoreExpiry(Key("a"), 1048);
    loaded.RestoreExpiry(Key("z"), 0);
    loaded.RestoreExpiry(Key("y"), -5);
    loaded.RestoreRejectedUntil(Key("b"), 300);
    loaded.RestoreRejectedUntil(Key("x"), 0);
    CHECK(loaded.Find(Key("a")) != NULL && loaded.Find(Key("a"))->expiryHour == 1048);
    CHECK(!loaded.Find(Key("a"))->expiryWarned);
    CHECK(loaded.Find(Key("z")) == NULL && loaded.Find(Key("y")) == NULL);
    CHECK(loaded.Count() == 1);
    CHECK(loaded.RejectedUntil(Key("b")) == 300 && loaded.RejectedUntil(Key("x")) == 0);

    loaded.Clear();
    CHECK(loaded.Count() == 0 && loaded.RejectedUntil(Key("b")) == 0);
}
} // namespace

void RunHelperRulesTests()
{
    AnEligibleAllyPassesTheScreen();
    EachCheckSkipsWithItsOwnReason();
    OnlyGoodAndNeutralOffer();
    TheHelperMustOutlevelTheLeaderByTheLead();
    TheRollFollowsTheChance();
    TheDaysStayWithinTheRange();
    TheContractRunsUntilItsHour();
    TheExpiryWarningComesOnce();
    TheSquadTurningOnThemEndsIt();
    DeathAndJoiningEndItQuietly();
    TheNativeContractRunsForTheHoursLeft();
    TheNativeWatchReassignsOnceThenFallsBack();
    ALoneHelperKeepsItsPlatoon();
    TheFallbackEscortRepathsEverySecond();
    TheFallbackRunsWhenFarAndWalksWhenClose();
    EachHelperTakesItsOwnSpotByTheLeader();
    TheAssistOrderIsRenewedWhenItLapses();
    TheHelperFightsTheLeadersEnemyOnly();
    TheBookKeepsContractsCooldownsAndRolls();
    TheSaveBridgeRoundTrips();
}
