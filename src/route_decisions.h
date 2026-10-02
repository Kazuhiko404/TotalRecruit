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
//   route_free.cpp    the open world: allies, honour, bribes
//   route_cage.cpp    the player's cage: the wait, intimidation, the price table
//   route_cats.cpp    the price both of the last two quote

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
// gesture rather than a cost. Nothing else is asked.
Verdict DecideCheat(const CheatFacts& facts, const Config& config);

// route_rescue
Verdict DecideUnconscious(const CoreFacts& core, const Config& config);
Verdict DecideDying(const CoreFacts& core);
Verdict DecideEasyHealthy(const CoreFacts& core);
// Someone you pulled out of the dirt owes you nothing else: a saved life weighs
// more than any other opening. A roll decides whether they can follow; if they
// cannot, they pay it forward.
Verdict DecideRescue(const CoreFacts& core, const RescueExtra& extra, const Config& config);

// route_yield
Verdict DecideGrudgeRefusal(const CoreFacts& core);
Verdict DecideFieldYield(const CoreFacts& core, const YieldExtra& extra, const Config& config);
Verdict DecideSurrendered(const CoreFacts& core);

// route_animal: a loose animal is tamed, not negotiated with. `core.inMyCage`
// matters: an animal that belongs to someone is only refused in the open.
Verdict DecideAnimal(const CoreFacts& core, const AnimalExtra& extra, const Config& config);

// route_free: the open-world conversation.
Verdict DecideFreeWorld(const CoreFacts& core, const FreeExtra& extra, const Config& config);
// Whether the free-world route can roll at all for this NPC (an ally who is not an
// animal). The roll's key is only minted when it can, so asking about a bystander
// does not give them an id.
bool FreeWorldMayRoll(const CoreFacts& core, const RelationFact& relation, const Config& config);

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
// purse, the reason and willJoin of `verdict`.
void ApplyCatsOffer(Verdict* verdict, const PriceInputs& price, const PurseFact& purse,
                    const Prices& prices);
} // namespace totalrecruit
