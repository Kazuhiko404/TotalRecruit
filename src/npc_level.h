#pragma once

// The engine read behind the one level (level_rules.h): CharStats::
// getOverallSkillLevel_0_100(), the number TotalSparring's duel card shows, rounded
// with RoundLevel. Guarded like engine_reads.h.

class Character;

namespace totalrecruit
{
// `character`'s level as TotalSparring shows it. When the stats cannot be read (no
// CharStats, a fault, a non-finite value) it falls back to getRoughLevel() and logs
// that once per session. False only when neither read works (`*out` untouched).
bool SafeNpcLevel(Character* character, int* out);
} // namespace totalrecruit
