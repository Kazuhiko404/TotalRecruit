#include "duel_record_cache_rules.h"

namespace totalrecruit
{
bool DuelRecordNeedsRead(const DuelRecordStamp& cached, const void* npc,
                         const void* challenger, unsigned boutGeneration)
{
    return !cached.valid || cached.npc != npc || cached.challenger != challenger ||
           cached.generation != boutGeneration;
}
} // namespace totalrecruit
