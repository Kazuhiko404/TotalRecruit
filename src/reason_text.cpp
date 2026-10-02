#include "reason_text.h"
#include "reason_catalog.h"
#include "engine_reads.h"
#include "messages.h"

namespace totalrecruit
{
std::string RenderReason(const Verdict& verdict, Character* who)
{
    if (verdict.reasonKey == Reason_None)
    {
        return std::string();
    }

    // The band only matters to the reasons that vary by standing; every one of
    // those is produced after the band was read, so "neutral" is only a guard.
    const std::string band = verdict.band.empty() ? std::string("neutral") : verdict.band;
    const std::string line = Say(ReasonMessageKey(verdict.reasonKey, band),
                                 ReasonDefaultText(verdict.reasonKey), who);

    return Fill(line, CharacterName(who), verdict.cats, 0, verdict.purse);
}
} // namespace totalrecruit
