#include "common.h"
#include "defeats.h"
#include "config.h"
#include "log.h"
#include "char_key_reader.h"
#include "engine_reads.h"
#include "defeat_rules.h"
#include "defeat_episode_rules.h"
#include "defeat_ledger.h"
#include "cage_commands.h"

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

NearDeathListener g_nearDeathListener = NULL;

// What the plugin remembers about the current knockout of a character the
// player has put down. Keyed by CharKey (char_key.h), never by the pointer: an
// address is not an identity, and a freed character's address is handed to the
// next one. Session-only: a load starts every episode over.
struct Record
{
    DefeatEpisode::State episode;
};

std::map<CharKey, Record> g_records;

// The counts themselves. The serialise hook reads them from wherever the engine
// saves, so the ledger is the one piece of this module behind a lock.
DefeatLedger g_ledger;

struct LedgerMutex
{
    CRITICAL_SECTION value;
    LedgerMutex() { InitializeCriticalSection(&value); }
    ~LedgerMutex() { DeleteCriticalSection(&value); }
} g_ledgerMutex;

struct LedgerGuard
{
    LedgerGuard() { EnterCriticalSection(&g_ledgerMutex.value); }
    ~LedgerGuard() { LeaveCriticalSection(&g_ledgerMutex.value); }
private:
    LedgerGuard(const LedgerGuard&);
    LedgerGuard& operator=(const LedgerGuard&);
};

int LedgerGet(const CharKey& key)
{
    LedgerGuard lock;
    return g_ledger.Get(key);
}

int LedgerAdd(const CharKey& key)
{
    LedgerGuard lock;
    return g_ledger.Add(key);
}

void LedgerSet(const CharKey& key, int count)
{
    LedgerGuard lock;
    g_ledger.Set(key, count);
}

void LedgerClear()
{
    LedgerGuard lock;
    g_ledger.Clear();
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

// Its own POD-only frame: C2712 counts the std::string the logging below
// builds, even though it never enters the guard.
Character* ReadVictim(MedicalSystem* medical)
{
    TR_TRY { return medical->me; }
    TR_CATCH { return NULL; }
}

const char* CreditSkipReason(DefeatCredit credit)
{
    return (credit == DefeatCredit_SkipPlayersOwn)       ? "is yours"
         : (credit == DefeatCredit_SkipWinnerUnreadable) ? "lastGuyWhoDefeatedMe unreadable/empty"
                                                         : "lastGuyWhoDefeatedMe is not yours";
}

// The record of a character the player has already put down at least once this
// session, or NULL. Never mints an id and never adds a record: a bystander the
// hail sweep looks at has nothing to re-arm.
Record* ExistingRecord(Character* character)
{
    CharKey key;
    if (!SafeFindCharKey(character, &key))
    {
        return NULL;
    }
    std::map<CharKey, Record>::iterator it = g_records.find(key);
    return it == g_records.end() ? NULL : &it->second;
}

void ReArm(Character* character, Record* record)
{
    const bool wasCounted = record->episode.countedThisEpisode;
    record->episode = DefeatEpisode::NoteStanding(record->episode);
    if (wasCounted && GetConfig().probeOnClick)
    {
        LogInfo("[defeat] " + CharacterName(character) +
                " seen standing: the next knockout counts again");
    }
}

// Counts one defeat if this sighting is a new one. `sighting` says whether the
// fall itself was seen. No caller vouches for a knockout any more (the
// DecideDefeatCredit override stays false): the knockouts the player caused
// through a cage command are beating stress, not defeats (NoteCageKnockout).
void RecordDefeat(Character* character, DefeatEpisode::Sighting sighting)
{
    if (character == NULL || !GetConfig().submission.countDefeats)
    {
        return;
    }

    const bool trace = GetConfig().probeOnClick;

    bool mine = false;
    const bool readMine = SafeIsPlayerCharacter(character, &mine);
    // The engine calls knockout for every character it puts down, including the
    // player's own and including deaths in the field the player never saw. Only a
    // knockout the player was responsible for counts towards yielding to them.
    bool beatenByPlayer = false;
    const bool readWinner = SafeDefeatedByPlayer(character, &beatenByPlayer);

    const DefeatCredit credit = DecideDefeatCredit(readMine && mine, readWinner,
                                                   beatenByPlayer, false);
    if (credit != DefeatCredit_Counts)
    {
        if (trace)
        {
            LogInfo("[defeat-skip] " + CharacterName(character) + ": " + CreditSkipReason(credit));
        }
        return;
    }

    CharKey key;
    if (!SafeCharKey(character, &key))
    {
        return;
    }

    // One knockout raises several engine calls as the body settles, and the
    // right-click looks at the same body again on every click: only the first
    // sighting of the episode counts, and only seeing them up again re-arms it.
    // Real time for the echo window: the hook's twin call ~1.2 s later reads them
    // standing too, and must not count a second defeat.
    const unsigned int nowMs = GetTickCount();
    Record& record = g_records[key];
    const DefeatEpisode::Verdict verdict = DefeatEpisode::Judge(record.episode, sighting, nowMs);
    if (!DefeatEpisode::CountsForPlayer(credit, verdict))
    {
        if (trace)
        {
            LogInfo("[defeat-skip] " + CharacterName(character) + ": " +
                    DefeatEpisode::VerdictName(verdict));
        }
        return;
    }
    record.episode = DefeatEpisode::AfterCount(record.episode, nowMs);
    const int count = LedgerAdd(key);

    bool nearDeath = false;
    bool alreadyGone = false;
    float bloodFraction = 1.0f;
    SafeNearDeath(character, &nearDeath, &bloodFraction, &alreadyGone);

    std::ostringstream note;
    note << CharacterName(character) << " defeated by the player ("
         << DefeatEpisode::VerdictName(verdict) << "; count " << count
         << ", " << key.ToString() << ") blood=" << bloodFraction
         << " nearDeath=" << (nearDeath ? "yes" : "no")
         << " dead=" << (alreadyGone ? "yes" : "no");
    LogInfo(note.str());

    // R3.1: close enough to death to plead. The limb-loss path already opens
    // the same window from its own event; this covers bleeding out, which the
    // engine announces to nobody.
    if (nearDeath && !alreadyGone && GetConfig().submission.begWhenNearDeath &&
        GetConfig().surrenderEnabled)
    {
        if (g_nearDeathListener != NULL)
        {
            g_nearDeathListener(character);
        }
    }
}

// cage-beating-stress: a knockout Golpear or Noquear caused on a prisoner in the
// player's cage is not a defeat -- the count and the field-yield route never see
// it -- but its episode is marked counted all the same, so the hook's twin call
// and the right-click fallback (both of which may find the engine naming the
// player) cannot count this same knockout by the back door. Seeing them standing
// again re-arms the episode as usual.
void NoteCageKnockout(Character* victim, const CageKnockout& cage)
{
    CharKey key;
    if (SafeFindCharKey(victim, &key))
    {
        Record& record = g_records[key];
        record.episode = DefeatEpisode::AfterCount(record.episode, GetTickCount());
    }
    char note[96];
    _snprintf_s(note, sizeof(note), _TRUNCATE, " cage knockout: not a defeat (stress +%d%%%s)",
                (int)(cage.stressGain * 100.0f + 0.5f),
                cage.kind == OrderedCredit_Beat ? ", the blow added its own" : "");
    LogInfo("[defeat] " + CharacterName(victim) + note);
}

void Knockout_hook(MedicalSystem* thisptr, float skill)
{
    // Read before the native knockout runs: standing now means this call is the
    // fall itself, not the body settling or a character kept down.
    Character* victim = NULL;
    bool readBefore = false;
    bool downBefore = true;
    if (GetConfig().enabled && thisptr != NULL)
    {
        victim = ReadVictim(thisptr);
        readBefore = victim != NULL && SafeIsKnockedOut(victim, &downBefore);
    }

    if (g_knockout_orig != NULL)
    {
        g_knockout_orig(thisptr, skill);
    }

    if (victim == NULL)
    {
        return;
    }

    // Logged before any filtering, because "the count stayed at zero" has two
    // very different causes -- the hook never firing, or firing and being
    // discarded -- and they need opposite fixes.
    if (GetConfig().probeOnClick)
    {
        LogInfo("[defeat-hook] knockout fired for " + CharacterName(victim) +
                (readBefore && !downBefore ? " (was standing)" : " (already down)"));
    }
    // A knockout the player caused with Golpear's blow or Noquear's order is the
    // cage command's, whatever the engine recorded as the winner: the blow and the
    // fallback knockout name nobody at all. Claimed once, inside that command's
    // window: 5 s around a blow, 2 min after a knockout order (cage_command_rules.h
    // kBeatCreditWindowMs / kOrderedKnockoutWindowMs). It is beating stress, not
    // a defeat; every other knockout goes to the ordinary defeat rules.
    const CageKnockout cage = ResolveCageKnockout(victim);
    if (IsCageKnockout(cage.kind))
    {
        NoteCageKnockout(victim, cage);
        return;
    }
    RecordDefeat(victim, (readBefore && !downBefore) ? DefeatEpisode::Sighting_KnockoutFromStanding
                                                     : DefeatEpisode::Sighting_FoundDown);
}
} // namespace

void NoteDefeat(Character* character)
{
    RecordDefeat(character, DefeatEpisode::Sighting_FoundDown);
}

int DefeatCount(Character* character)
{
    CharKey key;
    if (character == NULL || !SafeFindCharKey(character, &key))
    {
        return 0;
    }
    return LedgerGet(key);
}

void ClearDefeats(Character* character)
{
    CharKey key;
    if (character == NULL || !SafeFindCharKey(character, &key))
    {
        return;
    }
    LedgerSet(key, 0);
    g_records.erase(key);
    LogInfo("[defeat] " + CharacterName(character) + ": defeats cleared");
}

// A second way in, because MedicalSystem::knockout may not be the call that
// runs when someone drops: bleeding out, hunger and a leg giving way all end
// with a body on the floor, and only some of those go through a knockout.
//
// So a character the player right-clicks while out cold is counted too -- once
// per episode, and only when the engine names the player as the one who put
// them down. One standing is seen standing, which re-arms the next knockout.
void NoteIfDown(Character* character)
{
    if (character == NULL || !GetConfig().submission.countDefeats)
    {
        return;
    }

    bool down = false;
    if (!SafeIsKnockedOut(character, &down))
    {
        return;
    }

    if (down)
    {
        RecordDefeat(character, DefeatEpisode::Sighting_FoundDown);
        return;
    }

    Record* record = ExistingRecord(character);
    if (record != NULL)
    {
        ReArm(character, record);
    }
}

void NoteSeenStanding(Character* character)
{
    if (character == NULL || !GetConfig().submission.countDefeats)
    {
        return;
    }

    // Cheapest first: most characters looked at were never put down by the
    // player, and those already re-armed need no engine read.
    Record* record = ExistingRecord(character);
    if (record == NULL || record->episode.seenStanding)
    {
        return;
    }

    bool down = true;
    if (!SafeIsKnockedOut(character, &down) || down)
    {
        return;
    }
    ReArm(character, record);
}

void SetNearDeathListener(NearDeathListener listener)
{
    g_nearDeathListener = listener;
}

int SavedDefeatCount(const std::string& persistedId)
{
    if (persistedId.empty())
    {
        return 0;
    }
    return LedgerGet(CharKey::FromId(persistedId));
}

void RestoreSavedDefeatCount(const std::string& persistedId, int count)
{
    if (persistedId.empty())
    {
        return;
    }
    LedgerSet(CharKey::FromId(persistedId), count);
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

void ForgetAllDefeats()
{
    g_records.clear();
    LedgerClear();
}

void RemoveDefeatTracking()
{
    ForgetAllDefeats();
}
} // namespace totalrecruit
