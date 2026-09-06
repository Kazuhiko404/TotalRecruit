#include "common.h"
#include "surrender.h"
#include "rules.h"
#include "config.h"
#include "log.h"

#include <kenshi/Tasker.h>

// AI.h cannot be included here: it redefines CharacterMessage, which Character.h
// already declares (Character.h:110 vs AI/AI.h:48). Same SDK bug family as
// ContextMenu's and BuildingDesignation's double definitions.
//
// Only one method off AI is needed, so it is declared locally instead. The
// signature and its non-virtual thunk are taken verbatim from AI/AI.h:153, and
// the symbol is exported by KenshiLib.lib, so this links normally -- no
// hardcoded address.
class TaskStateData;
class AI
{
public:
    bool _NV_stateIsTrue(const TaskStateData& state, const hand& h,
                         const Ogre::Vector3& v);
};

namespace totalrecruit
{
namespace
{
bool SafePlayerLeader(Character** out)
{
    TR_TRY
    {
        if (ou == NULL || ou->player == NULL)
        {
            return false;
        }
        Platoon* platoon = ou->player->currentPlatoon;
        if (platoon == NULL)
        {
            return false;
        }
        *out = platoon->getSquadLeader();
        return (*out != NULL);
    }
    TR_CATCH { return false; }
}

// TaskStateData is a POD of {StateType key, bool val, bool progressionOnly}, so
// building one and handing it to the engine's own evaluator costs nothing and
// needs no address. _NV_stateIsTrue is used rather than the virtual, because
// GetRealAddress cannot resolve virtuals and the non-virtual thunk is what the
// import library exports.
bool AskEngineState(Character* character, StateType key, bool* out)
{
    AI* ai = character->getAI();
    if (ai == NULL)
    {
        return false;
    }

    const TaskStateData state(key, true, false);
    const hand& self = character->getHandle();
    *out = ai->_NV_stateIsTrue(state, self, character->getPosition());
    return true;
}

// TaskStateData and hand both have destructors, so the construction lives in a
// plain function and only the call crosses the guard; C2712 counts every object
// in the whole frame, not just those inside the __try block.
bool SafeAskEngineState(Character* character, StateType key, bool* out)
{
    TR_TRY { return AskEngineState(character, key, out); }
    TR_CATCH { return false; }
}

bool SafeIsFleeing(Character* character, bool* out)
{
    TR_TRY { *out = character->isFleeing(); return true; }
    TR_CATCH { return false; }
}

// Awake and on their feet, which is what yielding requires.
bool SafeIsConscious(Character* character, bool* out)
{
    TR_TRY
    {
        *out = !character->isDown() && !character->isUnconcious();
        return true;
    }
    TR_CATCH { return false; }
}

// lastGuyWhoDefeatedMe is a hand, so it survives the character it points at
// being freed -- which is exactly why the engine stores it that way.
bool SafeBeatenBy(Character* character, Character* who, bool* out)
{
    TR_TRY
    {
        *out = (character->lastGuyWhoDefeatedMe.getCharacter() == who);
        return true;
    }
    TR_CATCH { return false; }
}

bool SafeMemoryTag(Character* character, Character* about,
                   CharacterPerceptionTags_LongTerm tag, bool* out)
{
    TR_TRY
    {
        *out = character->getCharacterMemoryTag(about, tag);
        return true;
    }
    TR_CATCH { return false; }
}
} // namespace

bool IsSurrendered(Character* character)
{
    if (character == NULL)
    {
        return false;
    }

    // The composed reading is the gate, not the engine's own state.
    //
    // TARGET_IS_SURRENDERED is an AI world-state used for the engine's own
    // decisions, and in practice it answers true for characters who are simply
    // not fighting -- a guard standing at his post reads as surrendered. Trusting
    // it alone let any healthy NPC be recruited on sight, which is exactly the
    // "too cheat" behaviour this route was supposed to avoid.
    //
    // So a yield requires what a yield actually looks like: you beat them, and
    // they are down, fleeing, or dying. The engine's state is then only allowed
    // to confirm, never to grant on its own.
    if (!HasYieldedToPlayer(character))
    {
        return false;
    }

    bool surrendered = false;
    if (SafeAskEngineState(character, TARGET_IS_SURRENDERED, &surrendered) && surrendered)
    {
        return true;
    }

    // The composed conditions already held, so a missing or negative engine
    // answer does not veto them.
    return true;
}

bool HasYieldedToPlayer(Character* character)
{
    if (character == NULL)
    {
        return false;
    }

    Character* leader = NULL;
    if (!SafePlayerLeader(&leader))
    {
        return false;
    }

    // It has to have been you who beat them. Someone else's victim owes you
    // nothing.
    bool beatenByPlayer = false;
    SafeBeatenBy(character, leader, &beatenByPlayer);
    if (!beatenByPlayer)
    {
        bool squadBeaten = false;
        SafeMemoryTag(character, leader, LT_DEFEATED_MY_SQUAD_ONCE, &squadBeaten);
        if (!squadBeaten)
        {
            return false;
        }
    }

    // And they have to be beaten but still awake. Yielding is something a
    // person does; an unconscious body cannot do it, and treating a knockout as
    // a surrender is what let a senseless character be talked into joining.
    // Those belong to the rescue route instead, which waits for them to wake.
    bool conscious = false;
    if (!SafeIsConscious(character, &conscious) || !conscious)
    {
        return false;
    }

    // On their feet but running from you.
    bool fleeing = false;
    SafeIsFleeing(character, &fleeing);
    if (fleeing)
    {
        return true;
    }

    // Or still standing and bleeding out, with the fight visibly gone.
    return IsGenuinelyDying(character);
}
} // namespace totalrecruit
