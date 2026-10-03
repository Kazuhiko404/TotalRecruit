#include "common.h"
#include "temp_member.h"
#include "engine_reads.h"
#include "log.h"

#include <cstdio>

namespace totalrecruit
{
void OnRecruitedTemporarily(Character* npc, int days)
{
    if (npc == NULL)
    {
        return;
    }
    char note[64] = { 0 };
    _snprintf_s(note, sizeof(note), _TRUNCATE, " [temp] joined for %d days", days);
    LogInfo(CharacterName(npc) + note);
}
} // namespace totalrecruit
