#pragma once

// What a faction relation number means, without the engine.
//
// Relation runs -100..100, and the player reads it constantly without being
// taught: the cursor turns yellow over an ally. Naming the bands here means the
// rules and the dialogue always agree about what a number means.
//
// The refusal gates (config.relationRefuseGoodNatured / relationRefuseAll) are a
// separate, configurable matter: they decide who will not talk, the band only
// names how the two of you get on.

namespace totalrecruit
{
// "aliado" (>= 75), "amistoso" (>= 25), "cordial" (>= 5), "neutral" (>= -5),
// "receloso" (>= -15), "hostil" (>= -30) and "enemigo" below that. These are the
// keys messages.json files its greetings and reasons under.
const char* RelationBand(float relation);
} // namespace totalrecruit
