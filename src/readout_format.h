#pragma once

// How the readout writes its numbers, without the engine.
//
// The card's lines are short templates with placeholders ("Precio {cats} cats
// (tienes {purse})"); this fills them. Cats are written the way the mod's players
// write them, with a dot between thousands, and time left is rounded UP: telling
// someone "faltan 2h" and joining at 2h10 is a lie in the wrong direction.

#include <string>

namespace totalrecruit
{
struct LineArgs
{
    int cats;
    int purse;
    float hours;
    int food;
    int need;
    // A join chance in percent.
    int chance;

    LineArgs() : cats(0), purse(0), hours(0.0f), food(0), need(0), chance(0) {}
};

// 9570 -> "9.570"; 0 -> "0"; a negative number keeps its sign.
std::string FormatCats(int cats);

// Time left in game hours: "3h" from one hour up (rounded up), "45min" below it
// (rounded up, never "60min"), "0min" for nothing left.
std::string FormatHoursLeft(float hours);

// Replaces {cats} {purse} {hours} {food} {need} {chance} in a template; anything else is
// left as written.
std::string FillLine(const std::string& text, const LineArgs& args);
} // namespace totalrecruit
