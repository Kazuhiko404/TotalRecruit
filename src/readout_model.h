#pragma once

// What the readout card shows for one NPC, without the engine.
//
// The card never decides anything. It is handed the facts the decision read and
// the verdict the decision reached (the same Evaluate path, read-only), and this
// says what to draw: the route, the stress and what it is made of, the chance, the
// defeats, the stomach of an animal, and the one line that stands in the way. Every
// number is one the decision already computed -- the chance is the verdict's own,
// the price is the verdict's own, the stress is StressFromState -- so the card can
// never disagree with what a click will do.
//
// It also never says whether the roll passes: the roll is stable and seeded from
// the character, so the odds are the information and the result is the click's.
// (The peek that feeds this supplies a roll key only for the rescue, from an
// existing id; see rules.h.)

#include "config.h"
#include "entry_facts.h"
#include "route_rules.h"
#include "verdict.h"

namespace totalrecruit
{
// The seven names the player is told. Mercy is not one of the entry routes: it is the
// plea of a squad member, so it comes from the mercy state rather than the route.
// FieldYield (beaten in the open, breaking) is its own name: it is not the "rather
// than die" surrender, and calling it that told the player the wrong story.
enum ReadoutRoute
{
    ReadoutRoute_Free,
    ReadoutRoute_Captivity,
    ReadoutRoute_Rescue,
    ReadoutRoute_Surrender,
    ReadoutRoute_Mercy,
    ReadoutRoute_Animal,
    ReadoutRoute_FieldYield
};

enum ReadoutTone
{
    ReadoutTone_Plain,
    // They will join.
    ReadoutTone_Good,
    // The honest no: nothing the player can do changes it.
    ReadoutTone_Refused
};

// One line of the card: which line (readout_catalog.h) and the numbers it carries.
struct ReadoutLineView
{
    // -1 when there is no line.
    int id;
    int cats;
    int purse;
    float hours;
    int food;
    int need;
    int chance;
    ReadoutTone tone;

    ReadoutLineView()
        : id(-1), cats(0), purse(0), hours(0.0f), food(0), need(0), chance(0),
          tone(ReadoutTone_Plain)
    {
    }
    bool Present() const { return id >= 0; }
};

struct ReadoutInput
{
    // The route that answered. Meaningless when `haveCore` is false (the Cheat
    // preset answers before reading who they are).
    Route route;
    bool haveCore;
    CoreFacts core;
    Verdict verdict;
    // Hours held in the player's cage, when that was read.
    bool holdKnown;
    float hoursHeld;
    // This NPC's plea is the one on screen.
    bool mercyPending;
    // The evaluation had the roll key, so a rolled verdict is the click's own
    // outcome. False for the card's peek (EvaluatePeek mints no id; only its
    // rescue rolls, from an existing id): a roll it could not make is shown as
    // odds, never as a "not yet".
    bool rollKeyed;
    // A temporary member of the player's squad (temp_member.h): the card is not about
    // joining any more, only how many days they have left (`tempDaysLeft`). Nothing
    // else above is read.
    bool tempMember;
    int tempDaysLeft;
    // A free helper escorting the player (helper.h): the card only counts the days
    // they have left (`helperDaysLeft`), like a temporary member's.
    bool helper;
    int helperDaysLeft;
    // The head-to-head duel record of the selected squad member against this NPC,
    // set by the caller only when the duel is on offer (duel_challenge_rules.h
    // CanOfferDuel) and TotalSparring could read it. It becomes the second line when
    // that is free ("Duelos: 2-1").
    bool duelRecordKnown;
    int duelWins;
    int duelLosses;
    int duelDraws;
    // What the next win does (duel_join_rules.h DuelNextWinOdds), carried on the record
    // line: "Duelos: 2-1, si ganas 65% se une", or "si ganas se une" when sure (Cheat).
    bool duelJoinKnown;
    bool duelJoinSure;
    int duelJoinChance;

    ReadoutInput()
        : route(Route_FreeWorld), haveCore(true), core(), verdict(),
          holdKnown(false), hoursHeld(0.0f), mercyPending(false), rollKeyed(false),
          tempMember(false), tempDaysLeft(0), helper(false), helperDaysLeft(0),
          duelRecordKnown(false), duelWins(0), duelLosses(0), duelDraws(0),
          duelJoinKnown(false), duelJoinSure(false), duelJoinChance(0)
    {
    }
};

struct ReadoutView
{
    // False for the verdicts that are not about a person to talk to (nobody, a
    // corpse, a faker, animals switched off): the card stays hidden.
    bool visible;
    ReadoutRoute route;

    bool showStress;
    int stressPct;
    // What the stress is made of, in percentage points; they add up to stressPct.
    int hungerPts;
    int woundsPts;
    int tiredPts;
    int beatenPts;   // the beating stress (Golpear / Noquear), beating_stress_rules.h

    bool showChance;
    int chancePct;

    bool showDefeats;
    int defeats;
    int defeatsNeeded;

    // An animal's stomach and the fullness it needs, in percent.
    bool showFood;
    int foodPct;
    int foodNeededPct;

    ReadoutLineView primary;
    // The easy wait, while it is still running next to some other way in.
    ReadoutLineView secondary;

    ReadoutView()
        : visible(false), route(ReadoutRoute_Free), showStress(false), stressPct(0),
          hungerPts(0), woundsPts(0), tiredPts(0), beatenPts(0), showChance(false),
          chancePct(0),
          showDefeats(false), defeats(0), defeatsNeeded(0), showFood(false),
          foodPct(0), foodNeededPct(0), primary(), secondary()
    {
    }
};

ReadoutView BuildReadout(const ReadoutInput& in, const Config& config);
} // namespace totalrecruit
