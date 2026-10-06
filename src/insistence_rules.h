#pragma once

// Which refused talks count as insistence (R2.4), without the engine.
//
// Insistence exists so that asking again someone who already said no costs
// something. It used to count every click on Hablar, before the verdict was even
// known: a wounded animal the player was told to heal, someone out cold, a yes --
// all of it filled the counter, and the player got "harto de tu insistencia" for
// following the mod's own instructions. Only a real decision against the player
// counts now, and an animal never counts: it is tamed with food, not argued with.
//
// The table, one row per ReasonKey (verdict.h). COUNTS = the NPC decided against
// the player; every other reason is a state that blocks the question or a "do this
// first" the player is busy doing.
//
//   COUNTS   GrudgeNever          killed one of theirs: the honest no
//   COUNTS   FieldYieldNotYet     beaten, rolled, and did not yield
//   COUNTS   AllyMaybe            an ally rolled and said not yet
//   COUNTS   FactionHonour        honour will not deal with your reputation
//   COUNTS   FactionGreedy        hates you too much, not even for money
//   COUNTS   HonourRefusesBribe   honour refuses a bribe
//   COUNTS   MilitaryLoyalty      loyal to their faction, not for sale
//   COUNTS   Defiant              the cage roll failed: still defiant
//   COUNTS   UniqueZealot         a named zealot: no price for what they believe
//   COUNTS   AllyLeaderTooWeak    Balanced tier: you are too weak for them
//   COUNTS   AllyCruel            Balanced tier: cruelty walks beside nobody
//   COUNTS   AllyGreedy           Balanced tier: worth more than you could pay
//
//   no       None, Nobody, AnimalsDisabled, UnreadableTraits, Pretending, Dead
//                                 gates: the question could not be asked
//   no       CheatJoinsFreely, CheatNeedsCage, CheatNeedsAnyCage, CheatWaiting, CheatJoins
//                                 Cheat: a cage and a minute, never a no
//   no       UnconsciousOwesYou, UnconsciousDying, UnconsciousWait, Dying
//                                 out cold or dying: a state, not a decision
//   no       RescueJoins, RescueRepays, FieldYieldJoins, Surrendered
//                                 yes, or cannot follow and pays instead
//   no       InOthersCage         somebody else's prisoner: a state
//   no       Insistence           the lockout itself never extends itself
//   no       AllyJoins, CatsJoin, Intimidated, FreedFollows, CaredFor,
//            BrokenNoPride, TameReady, AllyTemporary
//                                 yes (a talk that ends in a yes never counts)
//   no       HonourStarving       hungry: feed them first
//   no       CatsAsk              a price on the table: bring the cats
//   no       CaptivityDisabled    switched off in the config
//   no       Starving, Wounded, StarvingAndWounded
//                                 the cage's "feed / heal them first"
//   no       AnimalOwned, TameWounded, TameHungry, TameNotFed
//                                 animals: owned, heal first, feed first
//   no       RejectCooldown       the player rejected them: its own lockout
//
// A reason added to verdict.h later is a no until it is classified here: a missed
// row costs the pestering penalty, never a lockout the player did not earn.

#include "verdict.h"

namespace totalrecruit
{
// True when a refusal with this reason is a real decision against the player
// (the COUNTS rows above). Unknown and out-of-range keys are false.
bool ReasonCountsAsInsistence(ReasonKey key);

// Whether this talk adds one attempt to the insistence counter: a refusal
// (`!willJoin`) whose reason counts, from someone who is not an animal (`animal`
// or `verdict.isAnimal`).
bool CountsAsInsistence(const Verdict& verdict, bool animal);
} // namespace totalrecruit
