#pragma once

class Character;

namespace totalrecruit
{
// Route C -- surrender.
//
// A beaten NPC about to die can yield rather than be cut down. A second
// narrative route into the squad that needs no cage at all.
//
// The plan deferred this because TARGET_IS_SURRENDERED has no named predicate
// in the SDK -- its two enum neighbours (targetIsUnarmed, _targetLooksLikeASlave)
// are right there in AI/AI.h, but the one in the middle is missing -- and the
// only route seemed to be a static address, which the field notes warn against.
//
// That turned out to be wrong. AI::stateIsTrue is an ordinary exported method
// that takes a TaskStateData, and TaskStateData is a plain POD with an exported
// constructor. So the engine's own state machine can be asked the question
// directly, through the linker, with no pinned addresses at all.
bool IsSurrendered(Character* character);

// True when this character was beaten by the player and is in no state to keep
// fighting. Composed from verified predicates, and used when the engine's own
// state answer is unavailable.
bool HasYieldedToPlayer(Character* character);
} // namespace totalrecruit
