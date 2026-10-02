#pragma once

// Reads the engine and answers "which CharKey is this character?" (char_key.h).
//
// The key is the character's persisted id (character_identity.h) whenever there
// is one, so per-character state and the stable join roll survive a game
// relaunch. When the id cannot be had -- the identity hooks failed to install,
// or two live characters claim the same id -- the key is the engine handle,
// logged once, so the mod keeps working the way it did before T7.

#include "char_key.h"

class Character;

namespace totalrecruit
{
// The key to WRITE state under. Creates the character's id on first use (it is
// saved at the next save). Returns false when the handle cannot be read: every
// caller treats that as "no record", because forgetting state is a smaller
// failure than applying it to the wrong character.
bool SafeCharKey(Character* character, CharKey* out);

// The key to READ state under. Never creates an id, so asking about a
// bystander (the hold-clock sweep looks at everyone in range) does not mint one
// for them. A character without an id yet has no id-keyed state, so a miss is
// the correct answer.
bool SafeFindCharKey(Character* character, CharKey* out);
} // namespace totalrecruit
