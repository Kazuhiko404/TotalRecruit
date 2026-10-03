#pragma once

// Everything the rules need to know about one NPC, as plain values.
//
// Evaluate used to read the engine in the middle of deciding: a level here, a
// relation there, the same level three times over. The reads are gathered first
// (character_facts.cpp, the only place that touches the engine) into these structs,
// and the route rules (route_*.cpp) decide from them alone -- so a rule can be
// tested without a game and a wrong answer traces to one input.
//
// CoreFacts is what every route may look at and is read once per question. Each
// route that needs more has its own small extra (relation, purse, price...), read
// only when that route is the one answering.

#include "archetype.h"
#include "char_key.h"
#include "price_rules.h"
#include "temperament.h"

namespace totalrecruit
{
struct CoreFacts
{
    // The engine could say who they are (personality and archetype). Without it
    // there is nothing to price against, and the mod falls back to a plain recruit.
    bool traitsReadable;

    // Who.
    bool animal;
    bool unique;
    Temperament temperament;
    Archetype archetype;
    // Slave by any of the engine's three words for it.
    bool enslaved;
    int npcLevel;
    int playerLevel;

    // Shape they are in.
    bool pretending;
    bool corpse;
    bool unconscious;
    bool dying;
    bool starving;
    bool wounded;
    // Raw engine readings the stress bar and the taming bar are derived from.
    float hunger;
    float health;
    float bloodFraction;
    float worstDamage;
    float restedState;

    // What they remember and where they are.
    bool grudge;
    // The raw memory tag. The rescue ROUTE also needs rescue switched on; the cage
    // route's animal branch reads the tag whatever the switch says.
    bool rememberedAsSaviour;
    int defeats;
    bool inMyCage;
    // The composed "beaten and yielding" reading (surrender.cpp), already false
    // when the route is switched off or they are an animal.
    bool surrendered;

    CoreFacts()
        : traitsReadable(true), animal(false), unique(false),
          temperament(Temperament_Normal), archetype(Archetype_Other),
          enslaved(false), npcLevel(1), playerLevel(1), pretending(false),
          corpse(false), unconscious(false), dying(false), starving(false),
          wounded(false), hunger(3.0f), health(1.0f), bloodFraction(1.0f),
          worstDamage(0.0f), restedState(1.0f), grudge(false),
          rememberedAsSaviour(false), defeats(0), inMyCage(false),
          surrendered(false)
    {
    }
};

// Faction standing, when the engine could say.
struct RelationFact
{
    bool known;
    float value;

    RelationFact() : known(false), value(0.0f) {}
};

// What the player is carrying, when the engine could say.
struct PurseFact
{
    bool known;
    int cats;

    PurseFact() : known(false), cats(0) {}
};

// The key a stable roll is seeded from. Only read for a route that can roll, so
// asking about a bystander does not mint an id for them.
struct RollFact
{
    bool haveKey;
    CharKey key;

    RollFact() : haveKey(false), key() {}
};

// What the animal route needs beyond the core.
struct AnimalExtra
{
    // Belongs to an organisation of people rather than roaming loose, and the
    // config says that matters. Only read in the open: caged, it is already yours
    // to tame.
    bool belongsToSomeone;
    // How grown it is, 0..1 of its lifespan (1.0 when the engine cannot say).
    float maturity;

    AnimalExtra() : belongsToSomeone(false), maturity(1.0f) {}
};

struct CheatFacts
{
    bool inMyCage;
    // Hours held in the cage, only read when it can matter.
    float hoursHeld;
    // The clock was read (a peek reads it only when a running clock already exists).
    bool holdKnown;

    CheatFacts() : inMyCage(false), hoursHeld(0.0f), holdKnown(false) {}
};

struct RescueExtra
{
    RelationFact relation;
    RollFact roll;
};

// Whether the NPC can be asked anything in the open at all. Read by both routes
// that talk to someone outside the player's cage (field-yield and the free
// world), and decided once (DecideAccessRefusal, route_decisions.h).
struct AccessFact
{
    // In somebody else's cage, shackled or carried off.
    bool inAnyCage;
    // Pestered once too often.
    bool ignoringPlayer;

    AccessFact() : inAnyCage(false), ignoringPlayer(false) {}
};

struct YieldExtra
{
    AccessFact access;
    RollFact roll;
};

struct FreeExtra
{
    AccessFact access;
    RelationFact relation;
    RollFact roll;
    PurseFact purse;
    PriceInputs price;
    AnimalExtra animal;
};

struct CageExtra
{
    // Hours held in THIS cage, only read when the easy wait is on.
    float hoursHeld;
    // The clock was read; false when the wait is off or a peek found no clock.
    bool holdKnown;
    RollFact roll;
    PurseFact purse;
    PriceInputs price;
    AnimalExtra animal;

    CageExtra() : hoursHeld(0.0f), holdKnown(false) {}
};
} // namespace totalrecruit
