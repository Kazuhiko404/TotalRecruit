#include "common.h"
#include "defeats.h"
#include "config.h"
#include "log.h"
#include "rules.h"
#include "mercy.h"
#include "recruit_core.h"
#include "rules.h"

#include <map>
#include <sstream>

namespace totalrecruit
{
namespace
{
// Engine pointers can already be freed, so every dereference is guarded, and
// each guard keeps a POD-only frame -- C2712 counts every C++ object in the
// whole function, not just those inside the __try block.

void (*g_knockout_orig)(MedicalSystem*, float) = NULL;

// Keyed by the handle's index and serial, never by the pointer. An address is
// not an identity: once a character is freed the allocator can hand the same
// address to somebody else, who would then inherit a stranger's defeats.
struct Key
{
    unsigned int index;
    unsigned int serial;

    Key() : index(0), serial(0) {}

    bool operator<(const Key& other) const
    {
        if (index != other.index) return index < other.index;
        return serial < other.serial;
    }
};

struct Record
{
    int count;
    double lastHour;

    Record() : count(0), lastHour(-1000.0) {}
};

std::map<Key, Record> g_defeats;

bool SafeKey(Character* character, Key* out)
{
    TR_TRY
    {
        const hand& self = character->getHandle();
        out->index  = self.index;
        out->serial = self.serial;
        return true;
    }
    TR_CATCH { return false; }
}

bool SafeGameHours(double* out)
{
    TR_TRY
    {
        if (ou == NULL)
        {
            return false;
        }
        *out = ou->getTimeStamp_inGameHours().getTotalHours();
        return true;
    }
    TR_CATCH { return false; }
}

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

// LT_SQUAD_LOST_TO_ME_ONCE reads, from the NPC's side, as "this player's people
// beat mine" -- which is exactly what a defeat is. It persists in the save,
// which is the whole reason for using engine memory rather than a file.
bool SafeHasFloor(Character* character, Character* player, bool* out)
{
    TR_TRY
    {
        *out = character->getCharacterMemoryTag(player, LT_SQUAD_LOST_TO_ME_ONCE);
        return true;
    }
    TR_CATCH { return false; }
}

bool SafeWriteFloor(Character* character, Character* player)
{
    TR_TRY
    {
        character->rememberCharacter(player, LT_SQUAD_LOST_TO_ME_ONCE);
        return true;
    }
    TR_CATCH { return false; }
}

// I4 is still open: the engine reads these same tags, and nothing in the
// headers says what it does with them. So every write is logged next to what
// the AI intended at that moment, and the probe prints the same fields -- a
// before-and-after diff is then just two clicks and a grep.
// Split in two because C2712 counts every C++ object in the frame, and both the
// std::string from CharacterName and the LogInfo call are objects: the guard
// gets a POD-only frame, and the reporting happens outside it.
bool ReadIntent(Character* character, const char* when, char* buf, size_t size)
{
    TR_TRY
    {
        StateBroadcastData* broadcast = character->getStateBroadcast();
        if (broadcast == NULL)
        {
            return false;
        }
        _snprintf_s(buf, size, _TRUNCATE,
                    "[I4 %s] intendedAggression=%.2f aggroTowards=%.2f "
                    "distress=%.2f intendsToAttack=%d tempNeutral=%d",
                    when, broadcast->intendedAggression,
                    broadcast->aggressionTowardsTarget, broadcast->distress,
                    (broadcast->intendsToAttack.getCharacter() != NULL) ? 1 : 0,
                    broadcast->isTemporaryNeutral ? 1 : 0);
        return true;
    }
    TR_CATCH { return false; }
}

void SafeLogIntent(Character* character, const char* when)
{
    char buf[224] = { 0 };
    if (ReadIntent(character, when, buf, sizeof(buf)))
    {
        LogInfo(CharacterName(character) + " " + buf);
    }
}

bool SafeIsPlayerCharacter(Character* character, bool* out)
{
    TR_TRY { *out = character->isPlayerCharacter(); return true; }
    TR_CATCH { return false; }
}

// R3.1 -- begging for your life when you are close to losing it.
//
// The engine announces a lost limb as an event, but bleeding out raises none,
// so the knockout is the moment to ask. Blood over max blood is the measure:
// getOverallHealthRating returns 0 for anyone unconscious however intact, and
// this is asked precisely about characters who are down.
bool SafeNearDeath(Character* character, bool* out, float* outFraction, bool* outDead)
{
    TR_TRY
    {
        *out = false;
        *outDead = false;
        *outFraction = 1.0f;
        MedicalSystem* medical = character->getMedical();
        if (medical == NULL)
        {
            return false;
        }
        const float maxBlood = medical->getMaxBlood();
        if (maxBlood <= 0.0f)
        {
            return false;
        }
        const float fraction = medical->blood / maxBlood;
        *outFraction = fraction;

        // Negative blood is a body that has bled out -- the one honest signal
        // for actual death, per the notes. Without this check the begging
        // fired for corpses: two bodies at -0.42 and -0.95 both asked for
        // mercy, which is the opposite of what "close to death" means.
        *outDead = (fraction < 0.0f) || character->isDead() ||
                   medical->isProbablyDying();

        *out = !(*outDead) &&
               (fraction <= GetConfig().submission.nearDeathBlood);
        return true;
    }
    TR_CATCH { return false; }
}

// The engine calls knockout for every character it puts down, including the
// player's own and including deaths in the field the player never saw. Only a
// knockout the player was responsible for should count towards yielding to
// them, and lastGuyWhoDefeatedMe is the engine's own answer to who that was.
bool SafeBeatenByPlayer(Character* character, bool* out)
{
    TR_TRY
    {
        *out = false;
        Character* winner = character->lastGuyWhoDefeatedMe.getCharacter();
        if (winner == NULL)
        {
            return false;
        }
        *out = winner->isPlayerCharacter();
        return true;
    }
    TR_CATCH { return false; }
}

// Its own POD-only frame: C2712 counts the std::string the logging below
// builds, even though it never enters the guard.
Character* ReadVictim(MedicalSystem* medical)
{
    TR_TRY { return medical->me; }
    TR_CATCH { return NULL; }
}

void Knockout_hook(MedicalSystem* thisptr, float skill)
{
    if (g_knockout_orig != NULL)
    {
        g_knockout_orig(thisptr, skill);
    }

    if (!GetConfig().enabled || thisptr == NULL)
    {
        return;
    }

    Character* victim = ReadVictim(thisptr);
    if (victim != NULL)
    {
        // Logged before any filtering, because "the count stayed at zero" has
        // two very different causes -- the hook never firing, or firing and
        // being discarded -- and they need opposite fixes.
        if (GetConfig().probeOnClick)
        {
            LogInfo("[defeat-hook] knockout fired for " + CharacterName(victim));
        }
        NoteDefeat(victim);
    }
}
} // namespace

void NoteDefeat(Character* character)
{
    if (character == NULL || !GetConfig().submission.countDefeats)
    {
        return;
    }

    const bool trace = GetConfig().probeOnClick;

    // The player's own people going down is not a defeat the player inflicted.
    bool mine = false;
    if (SafeIsPlayerCharacter(character, &mine) && mine)
    {
        if (trace) LogInfo("[defeat-skip] " + CharacterName(character) + ": is yours");
        return;
    }

    bool beatenByPlayer = false;
    const bool readWinner = SafeBeatenByPlayer(character, &beatenByPlayer);
    if (!readWinner || !beatenByPlayer)
    {
        if (trace)
        {
            LogInfo("[defeat-skip] " + CharacterName(character) + ": " +
                    (readWinner ? "lastGuyWhoDefeatedMe is not yours"
                                : "lastGuyWhoDefeatedMe unreadable/empty"));
        }
        return;
    }

    Key key;
    if (!SafeKey(character, &key))
    {
        return;
    }

    double now = 0.0;
    if (!SafeGameHours(&now))
    {
        return;
    }

    // One knockout can raise several engine calls as the body settles, and a
    // character kept down keeps being "knocked out" while they lie there. A
    // debounce in game hours is what separates a second beating from an echo of
    // the first.
    Record& record = g_defeats[key];
    if (now - record.lastHour < GetConfig().submission.defeatDebounceHours)
    {
        if (trace) LogInfo("[defeat-skip] " + CharacterName(character) + ": debounced");
        return;
    }
    record.lastHour = now;
    record.count++;

    Character* leader = NULL;
    if (SafePlayerLeader(&leader))
    {
        bool hadFloor = false;
        SafeHasFloor(character, leader, &hadFloor);
        if (!hadFloor)
        {
            SafeLogIntent(character, "before-mark");
            if (SafeWriteFloor(character, leader))
            {
                SafeLogIntent(character, "after-mark");
            }
        }
    }

    bool nearDeath = false;
    bool alreadyGone = false;
    float bloodFraction = 1.0f;
    SafeNearDeath(character, &nearDeath, &bloodFraction, &alreadyGone);

    std::ostringstream note;
    note << CharacterName(character) << " defeated by the player ("
         << record.count << " this session, floor+session="
         << DefeatCount(character) << ") blood=" << bloodFraction
         << " nearDeath=" << (nearDeath ? "yes" : "no")
         << " dead=" << (alreadyGone ? "yes" : "no");
    LogInfo(note.str());

    // R3.1: close enough to death to plead. The limb-loss path already opens
    // the same window from its own event; this covers bleeding out, which the
    // engine announces to nobody.
    if (nearDeath && !alreadyGone && GetConfig().submission.begWhenNearDeath &&
        GetConfig().surrenderEnabled)
    {
        OfferMercy(character);
    }
}

int DefeatCount(Character* character)
{
    if (character == NULL)
    {
        return 0;
    }

    int session = 0;
    Key key;
    if (SafeKey(character, &key))
    {
        std::map<Key, Record>::const_iterator it = g_defeats.find(key);
        if (it != g_defeats.end())
        {
            session = it->second.count;
        }
    }

    // The floor is what a reload leaves behind. It cannot say how many times,
    // only that it happened at least once -- so precision is lost and progress
    // is not, which is the trade this design set out to make.
    Character* leader = NULL;
    int floor = 0;
    if (SafePlayerLeader(&leader))
    {
        bool marked = false;
        if (SafeHasFloor(character, leader, &marked) && marked)
        {
            floor = GetConfig().submission.persistedFloorCount;
        }
    }

    return session > floor ? session : floor;
}

// R2.3 -- the beating.
//
// The proud and the honourable are meant to be expensive, but "wait in a cage"
// was the only lever the player had against them, and waiting is not a choice.
// This is: a deliberate, visible act with a cost, on a cooldown, that cannot
// kill.
//
// knockoutForceTimer is the engine's own way to put someone out for a set time,
// which is what a beating amounts to here. Damages is not fabricated: building
// an engine struct from a plugin to inflict real wounds is how a prisoner ends
// up dead by accident, and R2.3 asks explicitly for a ceiling.
bool SafeForceKnockout(Character* character, float seconds)
{
    TR_TRY
    {
        MedicalSystem* medical = character->getMedical();
        if (medical == NULL)
        {
            return false;
        }
        medical->knockoutForceTimer(seconds);
        return true;
    }
    TR_CATCH { return false; }
}

bool SafeIsDown(Character* character, bool* out)
{
    TR_TRY { *out = character->isDown(); return true; }
    TR_CATCH { return false; }
}

// The engine's own predicate. isDown alone is true for anyone who cannot stand,
// including someone wide awake with a crippled leg.
bool SafeIsLiterallyOut(Character* character, bool* out)
{
    TR_TRY
    {
        *out = character->_NV_isLiterallyUnconciousNotPretending();
        return true;
    }
    TR_CATCH { return false; }
}

bool CanBeatPrisoner(Character* character, double* outHoursLeft)
{
    if (outHoursLeft) *outHoursLeft = 0.0;

    const Submission& sub = GetConfig().submission;
    if (character == NULL || !sub.allowBeating)
    {
        return false;
    }

    if (!IsInPlayerCage(character))
    {
        return false;
    }

    // Hitting someone who is already unconscious teaches them nothing and only
    // risks killing them.
    bool down = false;
    if (SafeIsDown(character, &down) && down)
    {
        return false;
    }

    Key key;
    double now = 0.0;
    if (!SafeKey(character, &key) || !SafeGameHours(&now))
    {
        return false;
    }

    std::map<Key, Record>::const_iterator it = g_defeats.find(key);
    if (it != g_defeats.end())
    {
        const double since = now - it->second.lastHour;
        if (since < sub.beatingCooldownHours)
        {
            if (outHoursLeft) *outHoursLeft = sub.beatingCooldownHours - since;
            return false;
        }
    }
    return true;
}

bool BeatPrisoner(Character* character, std::string* outMessage)
{
    double hoursLeft = 0.0;
    if (!CanBeatPrisoner(character, &hoursLeft))
    {
        if (outMessage)
        {
            *outMessage = (hoursLeft > 0.0)
                              ? "todavia se esta recuperando del anterior"
                              : "no puedes hacer eso ahora";
        }
        return false;
    }

    if (!SafeForceKnockout(character, GetConfig().submission.beatingKnockoutSeconds))
    {
        if (outMessage) *outMessage = "no se pudo";
        return false;
    }

    // Counted like any other defeat, so the beating feeds the same submission
    // arithmetic rather than being an action with no consequence.
    NoteDefeat(character);

    if (outMessage) *outMessage = CharacterName(character) + ": lo dejaste sin sentido";
    return true;
}

// A second way in, because MedicalSystem::knockout may not be the call that
// runs when someone drops: bleeding out, hunger and a leg giving way all end
// with a body on the floor, and only some of those go through a knockout.
//
// So a character seen unconscious is counted too, once per time they go down.
// The debounce that stops one beating counting twice does the same job here,
// and NoteDefeat still applies every filter -- this only adds a trigger.
void NoteIfDown(Character* character)
{
    if (character == NULL || !GetConfig().submission.countDefeats)
    {
        return;
    }

    bool down = false;
    if (!SafeIsLiterallyOut(character, &down) || !down)
    {
        return;
    }

    NoteDefeat(character);
}

bool InstallDefeatTracking()
{
    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&MedicalSystem::knockout),
            Knockout_hook, &g_knockout_orig))
    {
        LogError("Could not hook MedicalSystem::knockout.");
        return false;
    }

    LogInfo("Defeat tracking installed.");
    return true;
}

void RemoveDefeatTracking()
{
    g_defeats.clear();
}
} // namespace totalrecruit
