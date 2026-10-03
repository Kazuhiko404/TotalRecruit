// Tests for src/readout_format.cpp: how the card writes its numbers.

#include "test_support.h"
#include "readout_format.h"

using namespace totalrecruit;

namespace
{
void CatsGetDotsBetweenThousands()
{
    const char* name = "cats: a dot between thousands, as the players write them";
    CHECK(FormatCats(0) == "0");
    CHECK(FormatCats(7) == "7");
    CHECK(FormatCats(999) == "999");
    CHECK(FormatCats(1000) == "1.000");
    CHECK(FormatCats(9570) == "9.570");
    CHECK(FormatCats(150000) == "150.000");
    CHECK(FormatCats(1234567) == "1.234.567");
    CHECK(FormatCats(-4120) == "-4.120");
}

void HoursLeftRoundUp()
{
    const char* name = "hours left: rounded up, minutes under an hour, never 60min";
    CHECK(FormatHoursLeft(3.0f) == "3h");
    CHECK(FormatHoursLeft(2.1f) == "3h");
    CHECK(FormatHoursLeft(1.0f) == "1h");
    CHECK(FormatHoursLeft(0.75f) == "45min");
    CHECK(FormatHoursLeft(0.5f) == "30min");
    CHECK(FormatHoursLeft(0.01f) == "1min");
    CHECK(FormatHoursLeft(0.999f) == "59min");
    CHECK(FormatHoursLeft(0.0f) == "0min");
    CHECK(FormatHoursLeft(-2.0f) == "0min");
}

void PlaceholdersAreFilled()
{
    const char* name = "fill: every placeholder, repeated ones too, unknown ones left alone";
    LineArgs a;
    a.cats = 9570;
    a.purse = 12000;
    a.hours = 3.0f;
    a.food = 56;
    a.need = 80;
    a.chance = 41;
    CHECK(FillLine("{chance}%: puede unirse si le hablas", a) == "41%: puede unirse si le hablas");
    CHECK(FillLine("Precio {cats} cats (tienes {purse})", a) == "Precio 9.570 cats (tienes 12.000)");
    CHECK(FillLine("Faltan {hours} en la jaula", a) == "Faltan 3h en la jaula");
    CHECK(FillLine("Hambre {food}% / necesita {need}%", a) == "Hambre 56% / necesita 80%");
    CHECK(FillLine("{cats}{cats}", a) == "9.5709.570");
    CHECK(FillLine("sin {nada} aqui", a) == "sin {nada} aqui");
    CHECK(FillLine("", a) == "");
}
} // namespace

void RunReadoutFormatTests()
{
    CatsGetDotsBetweenThousands();
    HoursLeftRoundUp();
    PlaceholdersAreFilled();
}
