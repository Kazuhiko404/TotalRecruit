#pragma once

// Which route answers an NPC first, without the engine.
//
// Evaluate used to be one 700-line function whose order of checks WAS the
// design, and the order is where the bugs lived: a branch near the top could
// swallow a case that a later rule was written to handle. The order is one
// function now, over plain facts, so a test can state it.

namespace totalrecruit
{
enum Route
{
    // Beaten enough times in the open: yield or "not yet", no cage involved.
    Route_FieldYield,
    // Out cold: nobody agrees to anything while unconscious.
    Route_Unconscious,
    // Killed one of theirs and they hold to it: an absolute no.
    Route_GrudgeRefusal,
    // Saved their life.
    Route_Rescue,
    // Still dying and not yet saved: the price is care.
    Route_Dying,
    // Easy mode, the healing half: patched all the way up in your cage.
    Route_EasyHealthy,
    // Beaten and on the edge, still awake: yields rather than be cut down.
    Route_Surrendered,
    // Not in the player's cage: the free-world conversation.
    Route_FreeWorld,
    // In the player's cage: easy wait, intimidation, then the price table.
    Route_Cage
};

struct RouteFacts
{
    bool animal;
    bool surrenderEnabled;
    bool countDefeats;
    int defeats;
    int defeatsToBreak;

    bool unconscious;
    bool dying;

    bool grudge;
    bool grudgeUnforgivable;
    bool inMyCage;

    bool rescueEnabled;
    bool remembersSaviour;

    // Easy mode is on and the prisoner is fed, unwounded and at full health.
    bool easyHealthy;
    // The composed "beaten and yielding" reading from surrender.cpp.
    bool surrendered;

    RouteFacts()
        : animal(false), surrenderEnabled(true), countDefeats(true),
          defeats(0), defeatsToBreak(2), unconscious(false), dying(false),
          grudge(false), grudgeUnforgivable(false), inMyCage(false),
          rescueEnabled(true), remembersSaviour(false), easyHealthy(false),
          surrendered(false)
    {
    }
};

// The first route that applies, in priority order.
Route ChooseEntryRoute(const RouteFacts& facts);

const char* RouteName(Route route);
} // namespace totalrecruit
