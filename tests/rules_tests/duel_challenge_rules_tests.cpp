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
    CHECK(CanOfferDuel(f));
    f.npcLevel = 79;
    CHECK(!CanOfferDuel(f));
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

void AWonDuelThatStaysUsesTheSameCooldownBook()
{
    const char* name = "book: a win cooldown set by the engine hides the duel like a lost one";
    DuelChallengeBook book;
    book.SetCooldownUntil(Key("npc"), 150);
    CHECK(book.OnCooldown(Key("npc"), 149.5));
    CHECK(!book.OnCooldown(Key("npc"), 150.0));
    SetDuelCooldownUntil(Key("npc-pw"), 150);
    CHECK(DuelOnCooldown(Key("npc-pw"), 120.0));
    CHECK(SavedDuelCooldownUntil("npc-pw") == 150);
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
    CHECK(lost.cooldownUntilHour == 125);                // ceil(100.4 + 24)
    CHECK(book.CooldownUntil(Key("npc")) == 125);
    CHECK(book.OnCooldown(Key("npc"), 124.9));
    CHECK(!book.OnCooldown(Key("npc"), 125.0));
    CHECK(!book.Pending(Key("npc"), NULL));

    DuelChallengeBook other;
    other.Challenge(Key("npc2"), Key("me"), 10.0);
    const DuelResolveResult draw = other.Resolve(Duel("me", "npc2", DuelBout_Draw), 10.0, kConfig);
    CHECK(draw.resolution == DuelResolution_Lost);
    CHECK(draw.cooldownUntilHour == 34);
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
    const char* name = "cooldown: ceil(now + hours); no hours, no cooldown";
    CHECK(DuelCooldownUntilHour(100.0, 24.0f) == 124);
    CHECK(DuelCooldownUntilHour(100.2, 24.0f) == 125);
    CHECK(DuelCooldownUntilHour(100.2, 0.0f) == 101);
    CHECK(!DuelCooldownActive(DuelCooldownUntilHour(100.0, 0.0f), 100.0));
    CHECK(DuelCooldownActive(124, 123.99));
    CHECK(!DuelCooldownActive(0, 0.0));
}

void TheCooldownTravelsInTheSave()
{
    const char* name = "cooldown: the saved hour comes back; 0 or less forgets it";
    DuelChallengeBook book;
    book.SetCooldownUntil(Key("npc"), 300);
    CHECK(book.CooldownUntil(Key("npc")) == 300);
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
    RestoreSavedDuelCooldownUntil("abc", 77);
    CHECK(SavedDuelCooldownUntil("abc") == 77);
    CHECK(DuelOnCooldown(Key("abc"), 76.0));
    RestoreSavedDuelCooldownUntil("", 99);       // no id, no row
    CHECK(SavedDuelCooldownUntil("") == 0);

    RecordDuelChallenge(Key("n"), Key("me"), 10.0);
    const DuelResolveResult r = ResolveDuelBout(Duel("me", "n", DuelBout_BWon), 11.0, kConfig);
    CHECK(r.resolution == DuelResolution_Lost);
    CHECK(SavedDuelCooldownUntil("n") == 35);
    RecordDuelChallenge(Key("m"), Key("me"), 10.0);
    CHECK(ExpireDuelChallenges(20.0, 6.0f) == 1);
    RecordDuelChallenge(Key("w"), Key("me"), 10.0);
    WithdrawDuelChallenge(Key("w"));
    CHECK(ResolveDuelBout(Duel("me", "w", DuelBout_AWon), 11.0, kConfig).resolution ==
          DuelResolution_Ignored);
    ForgetAllDuelChallenges();
    CHECK(SavedDuelCooldownUntil("abc") == 0);
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

void RunDuelChallengeRulesTests()
{
    TheDuelIsOfferedToAnEliteAllyWhenTotalSparringIsThere();
    EachMissingFactTakesTheDuelAway();
    AWonDuelRecruits();
    AnotherSquadMemberMayFightIt();
    CheatOffersTheDuelOnTheLevelAlone();
    AWonDuelThatStaysUsesTheSameCooldownBook();
    ALostOrDrawnDuelStartsTheCooldown();
    ASparNeverCounts();
    AnUndecidedDuelKeepsTheChallenge();
    AnUnrelatedPairIsIgnored();
    AStaleChallengeExpires();
    ExpiryReadsTheClockSafely();
    ANewChallengeReplacesTheOld();
    TheCooldownHourRoundsUp();
    TheCooldownTravelsInTheSave();
    TheProcessWideBookBridgesTheSave();
    TheRecordReadsWinsDashLosses();
}
