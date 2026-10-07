#pragma once

// What each route answers, without the engine.
//
// ChooseEntryRoute (route_rules.h) says WHICH route speaks first; these say what
// that route decides. One function per route, each from the plain facts of
// entry_facts.h and the config alone, each returning a Verdict whose reason is a
// ReasonKey. Nothing here reads the game, speaks, charges or rolls a die that is
// not seeded from the character -- Evaluate (rules.cpp) gathers the facts, asks
// the right function and turns the reason into the player's line.
//
//   route_gate.cpp    questions that cannot be asked (nobody, a corpse, a faker)
//   route_cheat.cpp   the Cheat preset: a cage and a moment, nothing else
//   route_rescue.cpp  out cold, dying, saved, healed back up
//   route_yield.cpp   beaten: the grudge, the field yield, the surrender
//   route_animal.cpp  taming: food and nothing else
//   route_free.cpp    the open world: allies (by level tier in Balanced), honour, bribes
//   route_cage.cpp    the player's cage: the wait, intimidation, the price table
//   route_cats.cpp    the price both of the last two quote

#include "ally_tier_rules.h"
#include "config.h"
#include "entry_facts.h"
#include "verdict.h"

namespace totalrecruit
{
// The verdict every route starts from: it already says whether they are an animal
// and whether they are starving, which the offer window reads whatever the route.
Verdict BaseVerdict(const CoreFacts& core);

// route_gate: false when the question can be asked; true when `out` is already the
// answer (animals switched off, the engine could not say who they are, a faker, a
// corpse).
bool DecideGate(const CoreFacts& core, const Config& config, Verdict* out);

// route_cheat: R1.1 / R1.2 -- Cheat keeps the gesture and drops the price. The cage
// is required and the wait is a minute of game time, because in Cheat time is a
// gesture rather than a cost. Nothing else is asked. An animal that belongs to
// nobody and that the player fed (Reason_TameReady), healed (Reason_CheatAnimalHealed)
// or whose squad knocked it out (Reason_CheatAnimalDefeated) joins at once instead,
// caged or not; otherwise, outside the cage, it asks for the cage and takes food.
Verdict DecideCheat(const CheatFacts& facts, const Config& config);

// route_cheat (cheat-any-cage): whether the prisoner's cage is a cage Cheat accepts:
// the player's own, or any cage, shackles or kidnap while Config::cheatAnyCage is on.
// Only meaningful under Preset_Cheat (the caller already is).
bool CheatCageSatisfied(bool inMyCage, bool inAnyCage, const Config& config);

// route_cheat (cheat-any-cage): whether the hold clock (hold_tracking.cpp) runs for
// any cage, not only the player's: Cheat, with the cage required and cheatAnyCage on.
bool HoldClockCountsAnyCage(const Config& config);

// route_cheat (prisoner-hold-time): whether this cage is one the hold clock counts:
// the player's own in every preset, or any cage, shackles or kidnap where
// HoldClockCountsAnyCage. The readout card only shows the time held for such a cage.
bool HoldClockCounts(bool inMyCage, bool inAnyCage, const Config& config);

// route_rescue
Verdict DecideUnconscious(const CoreFacts& core, const Config& config);
Verdict DecideDying(const CoreFacts& core);
// Someone you pulled out of the dirt owes you nothing else: a saved life weighs
// more than any other opening. A roll decides whether they can follow; if they
// cannot, they pay it forward.
Verdict DecideRescue(const CoreFacts& core, const RescueExtra& extra, const Config& config);

// route_yield
Verdict DecideGrudgeRefusal(const CoreFacts& core);
Verdict DecideFieldYield(const CoreFacts& core, const YieldExtra& extra, const Config& config);
Verdict DecideSurrendered(const CoreFacts& core);

// route_animal_yield (animal-duel-submission D3): a free animal beaten defeatsToBreak
// times in the open. The same field-yield roll a person gets, with temperament Normal
// (an animal has none here) and no loyalty penalty (it serves no organisation), on
// the same daily seed. A failed roll is not a closed door: the taming route still
// answers (`animal`), so a fed animal past the bar joins and an unfed one keeps the
// feed button and the stomach bar. Another faction's prisoner is refused first; the
// insistence lockout never applies to an animal.
Verdict DecideAnimalFieldYield(const CoreFacts& core, const YieldExtra& yield,
                               const AnimalExtra& animal, const Config& config);

// route_animal: an animal is tamed, not negotiated with -- loose or caged
// (DecideCage hands a caged one straight here). It joins only when the player has
// fed it (`extra.fedByPlayer`) and its stomach reached the bar. `core.inMyCage`
// matters: an animal that belongs to someone is only refused in the open.
Verdict DecideAnimal(const CoreFacts& core, const AnimalExtra& extra, const Config& config);
// route_animal: an animal in the player's cage (priority-one D). The same taming,
// with the bar lowered by the beating stress Golpear / Noquear left on it
// (animal_beating_rules.h). DecideCage hands a caged animal here; Cheat never asks.
Verdict DecideCagedAnimal(const CoreFacts& core, const AnimalExtra& extra, const Config& config);

// route_free: whether someone outside the player's cage can be asked anything at
// all -- not another faction's prisoner, not one who stopped listening. True when
// `out` is already the answer. Field-yield and the free world both ask it first.
bool DecideAccessRefusal(const CoreFacts& core, const AccessFact& access, Verdict* out);
// Military and law enforcement serve a standing organisation: the free world
// cannot buy them, and field-yield weighs that loyalty against the beating.
bool ServesStandingOrganisation(Archetype archetype);

// route_free: the open-world conversation.
Verdict DecideFreeWorld(const CoreFacts& core, const FreeExtra& extra, const Config& config);
// Whether the free-world route can roll at all for this NPC (an ally who is not an
// animal). The roll's key is only minted when it can, so asking about a bystander
// does not give them an id.
bool FreeWorldMayRoll(const CoreFacts& core, const RelationFact& relation, const Config& config);
// route_free: the level tier a Balanced ally falls in (ally_tier_rules.h), from
// their level, the leader's, their personality group and the relation. False when
// the tiers do not apply: another preset, the tiers switched off, an animal, or
// not an ally. The free-world route decides by it and character_facts prices by
// it (the friendship discount), so the two can never disagree.
bool DecideBalancedAllyTier(const CoreFacts& core, const RelationFact& relation,
                            const Config& config, AllyTierDecision* out);

// route_cage: the player's own cage.
Verdict DecideCage(const CoreFacts& core, const CageExtra& extra, const Config& config);
// Which currency this archetype deals in. Condition takes precedence over
// archetype: a starving bandit wants food before he wants cats.
Currency ChooseCurrency(const CoreFacts& core);
// A named character overrides the ordinary table, because for them the currency
// IS the story. Someone who already sells people, or who enjoys the cruelty,
// understands a transaction; a zealot does not.
bool UniqueTakesMoney(Temperament temperament, Archetype archetype);

// route_cats: the quote, and whether the player can meet it. Fills currency, cats,
// purse, the reason and willJoin of `verdict`. An NPC offended by a lowball today
// quotes the marked-up ask (OffendedAsk, haggle_rules.h), and the purse is held
// against that.
void ApplyCatsOffer(Verdict* verdict, const PriceInputs& price, const PurseFact& purse,
                    const HaggleFact& haggle, const Prices& prices);
} // namespace totalrecruit
