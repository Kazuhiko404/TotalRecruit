#include "common.h"
#include "char_key_reader.h"
#include "character_identity.h"
#include "identity_registry.h"
#include "log.h"

namespace totalrecruit
{
namespace
{
// Each guarded function holds only POD (C2712): the CharKey, which owns a
// std::string, is built by the callers.
bool ReadHandleGuarded(Character* character, unsigned int* index, unsigned int* serial)
{
    TR_TRY
    {
        const hand& self = character->getHandle();
        *index  = self.index;
        *serial = self.serial;
        return true;
    }
    TR_CATCH { return false; }
}

// Minting an id for a freed character would bind it to a stranger's address.
bool ReadIsValidGuarded(Character* character)
{
    TR_TRY { return character->isValid(); }
    TR_CATCH { return false; }
}

void NoteFallback(bool identityReady)
{
    static bool loggedUnavailable = false;
    static bool loggedPerCharacter = false;
    if (!identityReady)
    {
        if (!loggedUnavailable)
        {
            loggedUnavailable = true;
            LogError("[identity] persisted character id unavailable; per-character state keyed by "
                     "handle (it changes on a game relaunch).");
        }
        return;
    }
    if (!loggedPerCharacter)
    {
        loggedPerCharacter = true;
        LogInfo("[identity] a character had no usable id (conflicted or not valid); "
                "its handle key was used instead.");
    }
}

bool ReadKey(Character* character, CharKey* out, bool mint)
{
    if (character == NULL || out == NULL)
    {
        return false;
    }

    unsigned int index = 0;
    unsigned int serial = 0;
    if (!ReadHandleGuarded(character, &index, &serial))
    {
        return false;
    }

    const bool ready = CharacterIdentity::IsReady();
    std::string id;
    bool found = false;
    if (ready && ReadIsValidGuarded(character))
    {
        found = mint ? CharacterIdentity::GetOrCreate(character, id)
                     : CharacterIdentity::Lookup(character, id);
    }

    bool fellBack = false;
    *out = identity::ChooseKey(ready, found, id, CharKey(index, serial), &fellBack);
    // A read-only miss is not a failure to identify: the character just has no
    // id yet, so there is nothing to find under it.
    if (fellBack && (mint || !ready))
    {
        NoteFallback(ready);
    }
    return true;
}
} // namespace

bool SafeCharKey(Character* character, CharKey* out)
{
    return ReadKey(character, out, true);
}

bool SafeFindCharKey(Character* character, CharKey* out)
{
    return ReadKey(character, out, false);
}
} // namespace totalrecruit
