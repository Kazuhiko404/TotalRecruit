// Tests for src/duel_challenge_rules.cpp: when the popup offers RETAR A DUELO, what a
// finished TotalSparring bout means for a pending challenge, the cooldown after a
// lost duel, the expiry of a challenge nobody finished, and the record line.

#include "test_support.h"
#include "duel_challenge_rules.h"

using namespace totalrecruit;

namespace
{
const DuelChallenges kConfig = DuelChallenges();

CharKey Key(const char* id)
{
    return CharKey::FromId(id);
}

// A level-85 Balanced ally, TotalSparring ready, no cooldown.
DuelOfferFacts Offerable()
{
    DuelOfferFacts f;
    f.enabled = true;
    f.tsAvailable = true;
    f.allyTiered = true;
    f.isAnimal = false;
    f.npcLevel = 85;
    f.eliteFromLevel = 80;
    f.onCooldown = false;
    return f;
}

// The duel TotalSparring reports for `npc`: the squad member `member` on side A.
DuelBoutRecord Duel(const char* member, const char* npc, DuelBoutOutcome outcome)
{
    DuelBoutRecord r;
    r.isDuel = true;
    r.outcome = outcome;
    r.aKnown = true;
    r.a = Key(member);
    r.aIsSquadMember = true;
    r.bKnown = true;
    r.b = Key(npc);
    return r;
}

void TheDuelIsOfferedToAnEliteAllyWhenTotalSparringIsThere()
{
    const char* name = "offer: enabled, TotalSparring ready, tiered ally at 80+, no cooldown";
    CHECK(CanOfferDuel(Offerable()));

    DuelOfferFacts f = Offerable();
    f.npcLevel = 80;
    CHECK(CanOfferDuel(f));          // the cut level belongs to the elite tier
    f.npcLevel = 79;
    CHECK(!CanOfferDuel(f));
    f = Offerable();
    f.npcLevel = 130;
    CHECK(CanOfferDuel(f));          // legend tier too
}

void EachMissingFactTakesTheDuelAway()
{
    const char* name = "offer: each missing fact hides the button";
    DuelOfferFacts f = Offerable();
    f.enabled = false;
    CHECK(!CanOfferDuel(f));
    f = Offerable();
    f.tsAvailable = false;
    CHECK(!CanOfferDuel(f));
    f = Offerable();
    f.allyTiered = false;
    CHECK(!CanOfferDuel(f));
    f = Offerable();
    f.isAnimal = true;
    CHECK(!CanOfferDuel(f));
    f = Offerable();
    f.onCooldown = true;
    CHECK(!CanOfferDuel(f));
    f = Offerable();
    f.eliteFromLevel = 90;
    CHECK(!CanOfferDuel(f));         // the configured cut, not a literal 80
}

void AWonDuelRecruits()
{
    const char* name = "resolve: a duel the squad member won against the challenged NPC -> Won";
    DuelChallengeBook book;
    book.Challenge(Key("npc"), Key("me"), 100.0);
    const DuelResolveResult r = book.Resolve(Duel("me", "npc", DuelBout_AWon), 101.0, kConfig);
    CHECK(r.resolution == DuelResolution_Won);
    CHECK(r.npc == Key("npc"));
    CHECK(r.challenger == Key("me"));
    CHECK(r.winner == Key("me"));
    CHECK(!book.Pending(Key("npc"), NULL));
    CHECK(!book.OnCooldown(Key("npc"), 101.0));
}

void AnotherSquadMemberMayFightIt()
{
    const char* name = "resolve: TotalSparring's window may switch fighters; any squad member's win counts";
    DuelChallengeBook book;
    book.Challenge(Key("npc"), Key("me"), 100.0);
    const DuelResolveResult r = book.Resolve(Duel("friend", "npc", DuelBout_AWon), 100.5, kConfig);
    CHECK(r.resolution == DuelResolution_Won);
    // The record that counts the win is the winner's, not the challenger's.
    CHECK(r.winner == Key("friend"));
    CHECK(r.challenger == Key("me"));
}

void CheatOffersTheDuelOnTheLevelAlone()
{
    const char* name = "offer: Cheat needs no ally tier, only the level, a person and the rest";
    DuelOfferFacts f = Offerable();
    f.allyTiered = false;
    f.cheatPreset = true;
    f.cheatPeopleFromLevel = 80;
    CHECK(CanOfferDuel(f));
    f.npcLevel = 79;
    CHECK(!CanOfferDuel(f));         // Cheat's own cut, here raised to 80
    f.npcLevel = 84;
    f.isAnimal = true;
    CHECK(!CanOfferDuel(f));
    f.isAnimal = false;
    f.onCooldown = true;
    CHECK(!CanOfferDuel(f));
    f.onCooldown = false;
    f.tsAvailable = false;
    CHECK(!CanOfferDuel(f));
}

void CheatOffersTheDuelToAPersonOfAnyLevel()
{
    const char* name = "offer: Cheat reads duelCheatPeopleFromLevel (1), Balanced keeps the elite cut";
    DuelOfferFacts f = Offerable();
    f.allyTiered = false;
    f.cheatPreset = true;
    f.npcLevel = 33;
    CHECK(f.cheatPeopleFromLevel == 1);   // the default: every level
    CHECK(CanOfferDuel(f));
    f.npcLevel = 1;
    CHECK(CanOfferDuel(f));
    f.cheatPeopleFromLevel = 40;
    f.npcLevel = 39;
    CHECK(!CanOfferDuel(f));
    f.npcLevel = 40;
    CHECK(CanOfferDuel(f));

    // Balanced: a tiered ally at 33 still gets nothing; the Cheat cut never applies.
    DuelOfferFacts b = Offerable();
    b.npcLevel = 33;
    b.cheatPeopleFromLevel = 1;
    CHECK(!CanOfferDuel(b));
    b.npcLevel = 80;
    CHECK(CanOfferDuel(b));
}

void AWonDuelThatStaysUsesTheSameCooldownBook()
{
    const char* name = "book: a win cooldown set by the engine hides the duel like a lost one";
    DuelChallengeBook book;
    book.SetCooldownUntil(Key("npc"), 150 * 60);
    CHECK(book.OnCooldown(Key("npc"), 149.5));
    CHECK(!book.OnCooldown(Key("npc"), 150.0));
    SetDuelCooldownUntil(Key("npc-pw"), 150 * 60);
    CHECK(DuelOnCooldown(Key("npc-pw"), 120.0));
    CHECK(SavedDuelCooldownUntil("npc-pw") == 150 * 60);
    SetDuelCooldownUntil(Key("npc-pw"), 0);
    CHECK(!DuelOnCooldown(Key("npc-pw"), 120.0));
}

void ALostOrDrawnDuelStartsTheCooldown()
{
    const char* name = "resolve: B won or a draw -> Lost, the challenge ends and the cooldown runs";
    DuelChallengeBook book;
    book.Challenge(Key("npc"), Key("me"), 100.0);
    const DuelResolveResult lost = book.Resolve(Duel("me", "npc", DuelBout_BWon), 100.4, kConfig);
    CHECK(lost.resolution == DuelResolution_Lost);
    CHECK(lost.npc == Key("npc"));
    CHECK(lost.cooldownUntilMinute == 125 * 60);         // ceil(100.4 + 24), in minutes
    CHECK(book.CooldownUntil(Key("npc")) == 125 * 60);
    CHECK(book.OnCooldown(Key("npc"), 124.9));
    CHECK(!book.OnCooldown(Key("npc"), 125.0));
    CHECK(!book.Pending(Key("npc"), NULL));

    DuelChallengeBook other;
    other.Challenge(Key("npc2"), Key("me"), 10.0);
    const DuelResolveResult draw = other.Resolve(Duel("me", "npc2", DuelBout_Draw), 10.0, kConfig);
    CHECK(draw.resolution == DuelResolution_Lost);
    CHECK(draw.cooldownUntilMinute == 34 * 60);
}

void ASparNeverCounts()
{
    const char* name = "resolve: a spar is ignored and keeps the challenge";
    DuelChallengeBook book;
    book.Challenge(Key("npc"), Key("me"), 100.0);
    DuelBoutRecord spar = Duel("me", "npc", DuelBout_AWon);
    spar.isDuel = false;
    CHECK(book.Resolve(spar, 100.5, kConfig).resolution == DuelResolution_Ignored);
    CHECK(book.Pending(Key("npc"), NULL));
}

void AnUndecidedDuelKeepsTheChallenge()
{
    const char* name = "resolve: cancelled / interrupted -> Ignored, the challenge waits for its expiry";
    DuelChallengeBook book;
    book.Challenge(Key("npc"), Key("me"), 100.0);
    CHECK(book.Resolve(Duel("me", "npc", DuelBout_Undecided), 100.5, kConfig).resolution ==
          DuelResolution_Ignored);
    PendingDuel p;
    CHECK(book.Pending(Key("npc"), &p));
    CHECK(p.challenger == Key("me"));
    CHECK(p.sinceHour == 100.0);
    CHECK(!book.OnCooldown(Key("npc"), 100.5));
}

void AnUnrelatedPairIsIgnored()
{
    const char* name = "resolve: a duel with someone nobody challenged, or an unreadable side, is ignored";
    DuelChallengeBook book;
    book.Challenge(Key("npc"), Key("me"), 100.0);
    CHECK(book.Resolve(Duel("me", "stranger", DuelBout_AWon), 100.5, kConfig).resolution ==
          DuelResolution_Ignored);
    CHECK(!book.OnCooldown(Key("stranger"), 100.5));

    DuelBoutRecord noB = Duel("me", "npc", DuelBout_AWon);
    noB.bKnown = false;
    CHECK(book.Resolve(noB, 100.5, kConfig).resolution == DuelResolution_Ignored);

    DuelBoutRecord notMine = Duel("me", "npc", DuelBout_AWon);
    notMine.aIsSquadMember = false;
    CHECK(book.Resolve(notMine, 100.5, kConfig).resolution == DuelResolution_Ignored);

    DuelBoutRecord noA = Duel("me", "npc", DuelBout_AWon);
    noA.aKnown = false;
    CHECK(book.Resolve(noA, 100.5, kConfig).resolution == DuelResolution_Ignored);
    CHECK(book.Pending(Key("npc"), NULL));
}

// The facts of a TotalSparring duel nobody challenged through TotalRecruit: the switch
// on, no pending challenge, the offer rule already checked and passed.
ExternalBoutFacts ExternalOk()
{
    ExternalBoutFacts f;
    f.countExternal = true;
    f.hasPending = false;
    f.offerChecked = true;
    f.offered = true;
    return f;
}

void AnExternalDuelCountsOnlyWhenEveryFactHolds()
{
    const char* name = "external: a squad member's decided duel against an offerable outsider counts";
    const DuelBoutRecord won = Duel("me", "npc", DuelBout_AWon);
    CHECK(JudgeExternalBout(won, ExternalOk()) == ExternalBout_Counts);
    CHECK(JudgeExternalBout(Duel("me", "npc", DuelBout_BWon), ExternalOk()) == ExternalBout_Counts);
    CHECK(JudgeExternalBout(Duel("me", "npc", DuelBout_Draw), ExternalOk()) == ExternalBout_Counts);

    // The shape passes but the offer rule was not read yet: the engine must read it.
    ExternalBoutFacts unchecked = ExternalOk();
    unchecked.offerChecked = false;
    unchecked.offered = false;
    CHECK(JudgeExternalBout(won, unchecked) == ExternalBout_CheckOffer);

    ExternalBoutFacts f = ExternalOk();
    f.offered = false;
    CHECK(JudgeExternalBout(won, f) == ExternalBout_NotOffered);
    f = ExternalOk();
    f.countExternal = false;
    CHECK(JudgeExternalBout(won, f) == ExternalBout_SwitchOff);
    f = ExternalOk();
    f.hasPending = true;
    CHECK(JudgeExternalBout(won, f) == ExternalBout_Pending);

    DuelBoutRecord spar = won;
    spar.isDuel = false;
    CHECK(JudgeExternalBout(spar, ExternalOk()) == ExternalBout_NotDuel);
    CHECK(JudgeExternalBout(Duel("me", "npc", DuelBout_Undecided), ExternalOk()) ==
          ExternalBout_Undecided);
    DuelBoutRecord squadPair = won;
    squadPair.bIsSquadMember = true;
    CHECK(JudgeExternalBout(squadPair, ExternalOk()) == ExternalBout_SquadPair);
    CHECK(JudgeExternalBout(squadPair, unchecked) == ExternalBout_SquadPair);  // no engine read
    DuelBoutRecord noB = won;
    noB.bKnown = false;
    CHECK(JudgeExternalBout(noB, ExternalOk()) == ExternalBout_SideUnknown);
    DuelBoutRecord noA = won;
    noA.aKnown = false;
    CHECK(JudgeExternalBout(noA, ExternalOk()) == ExternalBout_SideUnknown);
    DuelBoutRecord notMine = won;
    notMine.aIsSquadMember = false;
    CHECK(JudgeExternalBout(notMine, ExternalOk()) == ExternalBout_SideUnknown);
    CHECK(std::string(ExternalBoutReasonWord(ExternalBout_SquadPair)) == "squad pair");
}

void AnExternalDuelResolvesLikeAChallenge()
{
    const char* name = "external: Resolve with no pending challenge -> Won / Lost with the same cooldown";
    DuelChallengeBook book;
    const DuelResolveResult won = book.Resolve(Duel("me", "npc", DuelBout_AWon), 100.0, kConfig, true);
    CHECK(won.resolution == DuelResolution_Won);
    CHECK(won.external);
    CHECK(won.npc == Key("npc"));
    CHECK(won.challenger == Key("me"));
    CHECK(won.winner == Key("me"));
    CHECK(!book.OnCooldown(Key("npc"), 100.0));

    const DuelResolveResult lost = book.Resolve(Duel("me", "npc2", DuelBout_BWon), 100.4, kConfig, true);
    CHECK(lost.resolution == DuelResolution_Lost);
    CHECK(lost.external);
    CHECK(lost.cooldownUntilMinute == 125 * 60);
    CHECK(book.OnCooldown(Key("npc2"), 124.0));

    // Not eligible (the engine said no), or a squad pair: ignored as before.
    CHECK(book.Resolve(Duel("me", "npc3", DuelBout_AWon), 100.0, kConfig, false).resolution ==
          DuelResolution_Ignored);
    DuelBoutRecord squadPair = Duel("me", "npc3", DuelBout_AWon);
    squadPair.bIsSquadMember = true;
    CHECK(book.Resolve(squadPair, 100.0, kConfig, true).resolution == DuelResolution_Ignored);
    CHECK(book.Resolve(Duel("me", "npc3", DuelBout_Undecided), 100.0, kConfig, true).resolution ==
          DuelResolution_Ignored);

    // A pending challenge still answers first, and is not external.
    book.Challenge(Key("npc4"), Key("friend"), 100.0);
    const DuelResolveResult pending = book.Resolve(Duel("me", "npc4", DuelBout_AWon), 100.5, kConfig, true);
    CHECK(pending.resolution == DuelResolution_Won);
    CHECK(!pending.external);
    CHECK(pending.challenger == Key("friend"));
    CHECK(!book.Pending(Key("npc4"), NULL));
}

void AStaleChallengeExpires()
{
    const char* name = "expiry: a challenge older than expiryHours is dropped, and its late result ignored";
    DuelChallengeBook book;
    book.Challenge(Key("npc"), Key("me"), 100.0);
    book.Challenge(Key("fresh"), Key("me"), 105.0);
    std::vector<CharKey> expired;
    CHECK(book.ExpirePending(106.0, 6.0f, &expired) == 0);
    CHECK(book.ExpirePending(106.5, 6.0f, &expired) == 1);
    CHECK(expired.size() == 1 && expired[0] == Key("npc"));
    CHECK(!book.Pending(Key("npc"), NULL));
    CHECK(book.Pending(Key("fresh"), NULL));

    DuelChallengeBook late;
    late.Challenge(Key("npc"), Key("me"), 100.0);
    CHECK(late.Resolve(Duel("me", "npc", DuelBout_AWon), 107.0, kConfig).resolution ==
          DuelResolution_Ignored);
    CHECK(!late.Pending(Key("npc"), NULL));
}

void ExpiryReadsTheClockSafely()
{
    const char* name = "expiry: exactly at the limit still waits; a clock that went back never expires";
    PendingDuel p;
    p.sinceHour = 50.0;
    CHECK(!DuelPendingExpired(p, 56.0, 6.0f));
    CHECK(DuelPendingExpired(p, 56.01, 6.0f));
    CHECK(!DuelPendingExpired(p, 10.0, 6.0f));
}

void ANewChallengeReplacesTheOld()
{
    const char* name = "challenge: a second challenge replaces the first (new challenger, new clock)";
    DuelChallengeBook book;
    book.Challenge(Key("npc"), Key("me"), 100.0);
    book.Challenge(Key("npc"), Key("friend"), 103.0);
    PendingDuel p;
    CHECK(book.Pending(Key("npc"), &p));
    CHECK(p.challenger == Key("friend"));
    CHECK(p.sinceHour == 103.0);
    book.Withdraw(Key("npc"));
    CHECK(!book.Pending(Key("npc"), NULL));
}

void TheCooldownHourRoundsUp()
{
    const char* name = "cooldown: ceil(now + hours) as a minute for an hour or more, to the minute under; no hours, no cooldown";
    CHECK(DuelCooldownUntilMinute(100.0, 24.0f) == 124 * 60);
    CHECK(DuelCooldownUntilMinute(100.2, 24.0f) == 125 * 60);
    CHECK(DuelCooldownUntilMinute(100.2, 0.0f) == 0);
    CHECK(!DuelCooldownActive(DuelCooldownUntilMinute(100.0, 0.0f), 100.0));
    CHECK(DuelCooldownActive(124 * 60, 123.99));
    CHECK(!DuelCooldownActive(0, 0.0));
    // Cheat's minute (priority-one B): 100:15 + 1 min ends at 100:16, not at 101:00.
    const int cheat = DuelCooldownUntilMinute(100.25, 1.0f / 60.0f);
    CHECK(cheat == 100 * 60 + 16);
    CHECK(DuelCooldownActive(cheat, 100.26));
    CHECK(!DuelCooldownActive(cheat, 100.27));
}

void TheCooldownTravelsInTheSave()
{
    const char* name = "cooldown: the saved hour comes back; 0 or less forgets it";
    DuelChallengeBook book;
    book.SetCooldownUntil(Key("npc"), 300 * 60);
    CHECK(book.CooldownUntil(Key("npc")) == 300 * 60);
    CHECK(book.OnCooldown(Key("npc"), 299.0));
    book.SetCooldownUntil(Key("npc"), 0);
    CHECK(book.CooldownUntil(Key("npc")) == 0);
    CHECK(!book.OnCooldown(Key("npc"), 0.0));
    book.SetCooldownUntil(Key("npc"), -5);
    CHECK(book.CooldownUntil(Key("npc")) == 0);

    book.Challenge(Key("x"), Key("me"), 1.0);
    book.SetCooldownUntil(Key("x"), 40);
    book.Clear();
    CHECK(!book.Pending(Key("x"), NULL));
    CHECK(book.CooldownUntil(Key("x")) == 0);
}

void TheProcessWideBookBridgesTheSave()
{
    const char* name = "session: the save bridge reads and restores by persisted id";
    ForgetAllDuelChallenges();
    CHECK(SavedDuelCooldownUntil("abc") == 0);
    RestoreSavedDuelCooldownUntil("abc", 77 * 60);
    CHECK(SavedDuelCooldownUntil("abc") == 77 * 60);
    CHECK(DuelOnCooldown(Key("abc"), 76.0));
    RestoreSavedDuelCooldownUntil("", 99);       // no id, no row
    CHECK(SavedDuelCooldownUntil("") == 0);

    RecordDuelChallenge(Key("n"), Key("me"), 10.0);
    const DuelResolveResult r = ResolveDuelBout(Duel("me", "n", DuelBout_BWon), 11.0, kConfig);
    CHECK(r.resolution == DuelResolution_Lost);
    CHECK(SavedDuelCooldownUntil("n") == 35 * 60);
    CHECK(SavedDuelCooldownLegacyHour("n") == 35);
    RecordDuelChallenge(Key("m"), Key("me"), 10.0);
    CHECK(ExpireDuelChallenges(20.0, 6.0f) == 1);
    RecordDuelChallenge(Key("w"), Key("me"), 10.0);
    WithdrawDuelChallenge(Key("w"));
    CHECK(ResolveDuelBout(Duel("me", "w", DuelBout_AWon), 11.0, kConfig).resolution ==
          DuelResolution_Ignored);
    ForgetAllDuelChallenges();
    CHECK(SavedDuelCooldownUntil("abc") == 0);
}

void AnOldSaveHeldTheWholeHour()
{
    const char* name = "session: the legacy whole-hour row reads back as that hour, and the minute row wins when both are there";
    ForgetAllDuelChallenges();
    RestoreSavedDuelCooldownLegacyHour("old", 77);
    CHECK(SavedDuelCooldownUntil("old") == 77 * 60);
    CHECK(DuelOnCooldown(Key("old"), 76.5));
    CHECK(!DuelOnCooldown(Key("old"), 77.0));
    // A save from this build: the legacy row first (rounded up), then the minute row.
    RestoreSavedDuelCooldownLegacyHour("new", 101);
    RestoreSavedDuelCooldownUntil("new", 100 * 60 + 16);
    CHECK(SavedDuelCooldownUntil("new") == 100 * 60 + 16);
    CHECK(SavedDuelCooldownLegacyHour("new") == 101);
    RestoreSavedDuelCooldownLegacyHour("", 5);
    CHECK(SavedDuelCooldownLegacyHour("") == 0);
    ForgetAllDuelChallenges();
}

void TheRecordReadsWinsDashLosses()
{
    const char* name = "record: \"2-1\", draws only when there are some";
    CHECK(FormatDuelRecord(2, 1, 0) == "2-1");
    CHECK(FormatDuelRecord(0, 0, 0) == "0-0");
    CHECK(FormatDuelRecord(2, 1, 3) == "2-1-3");
    CHECK(FormatDuelRecord(-1, 4, -2) == "0-4");
}
} // namespace

// A free animal of level 52, duelAnimalsEnabled, not excluded, TotalSparring ready.
DuelOfferFacts OfferableAnimal()
{
    DuelOfferFacts f;
    f.enabled = true;
    f.tsAvailable = true;
    f.isAnimal = true;
    f.animalAllowed = true;
    f.animalOwned = false;
    f.animalFromLevel = 40;
    f.npcLevel = 52;
    f.eliteFromLevel = 80;
    return f;
}

void AFreeAnimalIsOfferedTheDuelInBothPresets()
{
    const char* name = "offer: a free animal at duelAnimalsFromLevel gets the duel, no tier, no 80+ cut";
    DuelOfferFacts f = OfferableAnimal();
    CHECK(CanOfferDuel(f));               // Balanced: an animal is never allyTiered
    f.cheatPreset = true;
    CHECK(CanOfferDuel(f));               // Cheat too
    f = OfferableAnimal();
    f.npcLevel = 40;
    CHECK(CanOfferDuel(f));               // the cut level belongs to it
    f.npcLevel = 39;
    CHECK(!CanOfferDuel(f));
    f = OfferableAnimal();
    f.animalFromLevel = 60;
    CHECK(!CanOfferDuel(f));              // the configured cut, not a literal 40
}

void AnOwnedDisallowedOrCoolingAnimalGetsNoDuel()
{
    const char* name = "offer: owned, switched off / excluded, cooldown or no TotalSparring hides an animal's duel";
    DuelOfferFacts f = OfferableAnimal();
    f.animalOwned = true;
    CHECK(!CanOfferDuel(f));
    f.cheatPreset = true;
    CHECK(!CanOfferDuel(f));              // owned is owned in Cheat as well
    f = OfferableAnimal();
    f.animalAllowed = false;
    CHECK(!CanOfferDuel(f));
    f = OfferableAnimal();
    f.onCooldown = true;
    CHECK(!CanOfferDuel(f));
    f = OfferableAnimal();
    f.tsAvailable = false;
    CHECK(!CanOfferDuel(f));
    f = OfferableAnimal();
    f.enabled = false;
    CHECK(!CanOfferDuel(f));
}

void ThePersonBranchIgnoresTheAnimalFacts()
{
    const char* name = "offer: a person keeps the 80+ tiered/Cheat gate whatever the animal facts say";
    DuelOfferFacts f = Offerable();
    f.npcLevel = 52;
    f.animalAllowed = true;
    f.animalFromLevel = 40;
    CHECK(!CanOfferDuel(f));              // a 52 person is under the elite cut
    f = Offerable();
    f.animalOwned = true;
    CHECK(CanOfferDuel(f));               // ownership is an animal fact only
    f = Offerable();
    f.allyTiered = false;
    f.animalAllowed = true;
    CHECK(!CanOfferDuel(f));              // a person in Balanced still needs the tier
}

// duel-any-source U7: an external TotalSparring bout in Balanced counts for any
// recruitable person at duelExternalBalancedFromLevel, no ally tier needed.
DuelOfferFacts ExternalBalancedPerson()
{
    DuelOfferFacts f = Offerable();
    f.allyTiered = false;
    f.npcLevel = 12;
    f.externalBout = true;
    f.recruitable = true;
    f.externalBalancedFromLevel = 1;
    return f;
}

void BalancedExternalBoutsCountForAnyRecruitablePerson()
{
    const char* name = "offer: Balanced external bout counts for a recruitable person at the external level (U7)";
    DuelOfferFacts f = ExternalBalancedPerson();
    CHECK(CanOfferDuel(f));               // level 12, no tier: counts
    f.externalBout = false;
    CHECK(!CanOfferDuel(f));              // the button keeps its 80+ tiered gate

    f = ExternalBalancedPerson();
    f.externalBalancedFromLevel = 13;
    CHECK(!CanOfferDuel(f));              // under the external level
    f.npcLevel = 13;
    CHECK(CanOfferDuel(f));               // the level itself counts

    f = ExternalBalancedPerson();
    f.recruitable = false;
    CHECK(!CanOfferDuel(f));              // excluded (name, faction...) never counts
    f = ExternalBalancedPerson();
    f.onCooldown = true;
    CHECK(!CanOfferDuel(f));
    f = ExternalBalancedPerson();
    f.tsAvailable = false;
    CHECK(!CanOfferDuel(f));
    f = ExternalBalancedPerson();
    f.enabled = false;
    CHECK(!CanOfferDuel(f));

    // A tiered 80+ ally still counts the old way, whatever the external level says.
    f = Offerable();
    f.externalBout = true;
    f.recruitable = false;
    f.externalBalancedFromLevel = 200;
    CHECK(CanOfferDuel(f));
}

void CheatAndAnimalsIgnoreTheExternalBalancedRule()
{
    const char* name = "offer: Cheat and animals keep their own gates on an external bout (U7)";
    DuelOfferFacts f = ExternalBalancedPerson();
    f.cheatPreset = true;
    f.cheatPeopleFromLevel = 20;
    CHECK(!CanOfferDuel(f));              // Cheat's own cut, not the Balanced external one
    f.npcLevel = 20;
    CHECK(CanOfferDuel(f));

    f = ExternalBalancedPerson();
    f.isAnimal = true;
    f.animalAllowed = true;
    f.animalFromLevel = 40;
    CHECK(!CanOfferDuel(f));              // a level-12 animal is under the animal level
    f.npcLevel = 40;
    CHECK(CanOfferDuel(f));
    f.animalOwned = true;
    CHECK(!CanOfferDuel(f));
}

void RunDuelChallengeRulesTests()
{
    BalancedExternalBoutsCountForAnyRecruitablePerson();
    CheatAndAnimalsIgnoreTheExternalBalancedRule();
    AFreeAnimalIsOfferedTheDuelInBothPresets();
    AnOwnedDisallowedOrCoolingAnimalGetsNoDuel();
    ThePersonBranchIgnoresTheAnimalFacts();
    TheDuelIsOfferedToAnEliteAllyWhenTotalSparringIsThere();
    EachMissingFactTakesTheDuelAway();
    AWonDuelRecruits();
    AnotherSquadMemberMayFightIt();
    CheatOffersTheDuelOnTheLevelAlone();
    CheatOffersTheDuelToAPersonOfAnyLevel();
    AWonDuelThatStaysUsesTheSameCooldownBook();
    ALostOrDrawnDuelStartsTheCooldown();
    ASparNeverCounts();
    AnUndecidedDuelKeepsTheChallenge();
    AnUnrelatedPairIsIgnored();
    AnExternalDuelCountsOnlyWhenEveryFactHolds();
    AnExternalDuelResolvesLikeAChallenge();
    AStaleChallengeExpires();
    ExpiryReadsTheClockSafely();
    ANewChallengeReplacesTheOld();
    TheCooldownHourRoundsUp();
    TheCooldownTravelsInTheSave();
    TheProcessWideBookBridgesTheSave();
    AnOldSaveHeldTheWholeHour();
    TheRecordReadsWinsDashLosses();
}
