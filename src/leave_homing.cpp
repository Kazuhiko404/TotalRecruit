#include "common.h"
#include "leave_homing.h"
#include "leave_homing_rules.h"
#include "approach_driver.h"
#include "approach_rules.h"
#include "char_key_reader.h"
#include "engine_reads.h"
#include "npc_motion.h"
#include "log.h"

#include <cmath>
#include <cstdio>
#include <vector>

// See leave_homing.h. The decisions are leave_homing_rules.h; this file reads, acts,
// logs. Game thread only (the frame tick and the leave that starts a walk).

namespace totalrecruit
{
namespace
{
// The walks are looked at this often (real milliseconds).
const unsigned int kCheckMs = 1000;

struct Walk
{
    Character* npc;
    CharKey key;
    std::string name;
    int x;
    int z;
    float startedAt;
    float orderedAt;
};

std::vector<Walk> g_walks;
EventClock g_clock;
DWORD g_nextCheck = 0;

std::string Spot(int x, int z)
{
    char buffer[48] = { 0 };
    _snprintf_s(buffer, sizeof(buffer), _TRUNCATE, "(%d,%d)", x, z);
    return std::string(buffer);
}

// Metres on the ground (x / z) from `npc` to the spot; negative when unread.
float MetersTo(Character* npc, int x, int z)
{
    float px = 0.0f, py = 0.0f, pz = 0.0f;
    if (!NpcMotion::Position(npc, &px, &py, &pz))
        return -1.0f;
    const float dx = px - (float)x;
    const float dz = pz - (float)z;
    return WorldUnitsToMeters(std::sqrt(dx * dx + dz * dz));
}

// The move order to the spot, at the height they stand at now.
bool Order(Character* npc, int x, int z)
{
    float px = 0.0f, py = 0.0f, pz = 0.0f;
    if (!NpcMotion::Position(npc, &px, &py, &pz))
        return false;
    return NpcMotion::MoveToPoint(npc, (float)x, py, (float)z);
}

// The walk's NPC, while the key read back from the pointer still matches.
Character* Alive(const Walk& walk)
{
    CharKey read;
    if (walk.npc == NULL || !SafeFindCharKey(walk.npc, &read) || !(read == walk.key))
        return NULL;
    bool dead = false;
    if (SafeIsDead(walk.npc, &dead) && dead)
        return NULL;
    return walk.npc;
}

// One walk's check; false when it is over.
bool Step(Walk* walk)
{
    Character* npc = Alive(*walk);
    if (npc == NULL)
    {
        LogInfo("[temp] " + walk->name + " stopped heading back: gone or dead.");
        return false;
    }
    const float now = g_clock.Now();
    const float meters = MetersTo(npc, walk->x, walk->z);
    const HomingAction action = StepHoming(now - walk->startedAt, now - walk->orderedAt, meters);
    if (action == Homing_Wait)
        return true;
    if (action == Homing_Reissue)
    {
        if (!Order(npc, walk->x, walk->z))
            LogError("[temp] " + walk->name + " heading back: move order FAILED.");
        walk->orderedAt = now;
        return true;
    }
    char note[64] = { 0 };
    _snprintf_s(note, sizeof(note), _TRUNCATE, " (%.1f m from it, %.0f s)", meters, now - walk->startedAt);
    LogInfo("[temp] " + walk->name + " " + HomingActionText(action) + " at " + Spot(walk->x, walk->z) + note +
            "; back to their own AI.");
    NpcMotion::ResumeRoutine(npc);
    return false;
}
} // namespace

void StartLeaveHoming(Character* npc, int x, int z)
{
    Walk walk;
    walk.npc = npc;
    walk.name = CharacterName(npc);
    walk.x = x;
    walk.z = z;
    if (npc == NULL || !SafeFindCharKey(npc, &walk.key))
    {
        LogError("[temp] " + walk.name + " cannot head back: not tracked.");
        return;
    }
    g_clock.Advance();
    walk.startedAt = g_clock.Now();
    walk.orderedAt = walk.startedAt;
    const bool ordered = Order(npc, x, z);
    for (unsigned int i = 0; i < g_walks.size(); ++i)
    {
        if (g_walks[i].key == walk.key)
        {
            g_walks.erase(g_walks.begin() + i);
            break;
        }
    }
    g_walks.push_back(walk);
    char note[48] = { 0 };
    _snprintf_s(note, sizeof(note), _TRUNCATE, ", %.1f m away", MetersTo(npc, x, z));
    LogInfo("[temp] " + walk.name + " heading back to " + Spot(x, z) + note +
            (ordered ? "." : "; move order FAILED."));
}

void LeaveHomingTick()
{
    g_clock.Advance();
    if (g_walks.empty())
        return;
    const DWORD now = GetTickCount();
    if ((LONG)(now - g_nextCheck) < 0)
        return;
    g_nextCheck = now + kCheckMs;
    for (unsigned int i = 0; i < g_walks.size();)
    {
        if (Step(&g_walks[i]))
            ++i;
        else
            g_walks.erase(g_walks.begin() + i);
    }
}

void ForgetLeaveHoming()
{
    g_walks.clear();
    g_clock.Reset();
    g_nextCheck = 0;
}
} // namespace totalrecruit
