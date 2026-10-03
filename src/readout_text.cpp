#include "common.h"
#include "readout_text.h"
#include "readout_catalog.h"
#include "readout_format.h"
#include "messages.h"

#include <cstdio>

namespace totalrecruit
{
namespace
{
std::string Pct(int value)
{
    char buf[16] = { 0 };
    _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%d%%", value);
    return buf;
}

std::string Line(const ReadoutLineView& line, Character* who)
{
    if (!line.Present())
    {
        return std::string();
    }
    LineArgs args;
    args.cats = line.cats;
    args.purse = line.purse;
    args.hours = line.hours;
    args.food = line.food;
    args.need = line.need;
    args.chance = line.chance;
    return FillLine(Say(ReadoutLineMessageKey(line.id), ReadoutLineDefaultText(line.id), who),
                    args);
}

std::string WithNumber(const std::string& label, int value)
{
    char buf[16] = { 0 };
    _snprintf_s(buf, sizeof(buf), _TRUNCATE, " %d", value);
    return label + buf;
}
} // namespace

ReadoutTexts BuildReadoutTexts(const ReadoutView& view, const std::string& name,
                               Character* who)
{
    ReadoutTexts t;
    t.name = name;
    t.route = Say(ReadoutRouteMessageKey(view.route), ReadoutRouteDefaultText(view.route), NULL);

    t.stressLabel = Say("readout.stress", "ESTRES", NULL);
    t.stressValue = Pct(view.stressPct);
    t.hungerLegend = WithNumber(Say("readout.hunger", "Hambre", NULL), view.hungerPts);
    t.woundsLegend = WithNumber(Say("readout.wounds", "Heridas", NULL), view.woundsPts);
    t.tiredLegend = WithNumber(Say("readout.tired", "Cansancio", NULL), view.tiredPts);

    t.chanceLabel = Say("readout.chance", "CHANCE", NULL);
    t.chanceValue = Pct(view.chancePct);

    t.defeatsLabel = Say("readout.defeats", "DERROTAS", NULL);
    char frac[24] = { 0 };
    _snprintf_s(frac, sizeof(frac), _TRUNCATE, "%d/%d", view.defeats, view.defeatsNeeded);
    t.defeatsValue = frac;

    t.foodLabel = Say("readout.food", "BARRIGA", NULL);
    t.foodValue = Pct(view.foodPct);

    t.primary = Line(view.primary, who);
    t.secondary = Line(view.secondary, who);
    return t;
}
} // namespace totalrecruit
