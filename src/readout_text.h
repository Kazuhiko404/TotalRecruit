#pragma once

// The words on the readout card, in the player language (messages.json "readout"
// block, with built-in text when the file has no entry). The view-model says WHAT
// to show; this turns each piece into the string the widget draws. It owns no
// layout and no rule.

#include <string>

#include "readout_model.h"

namespace totalrecruit
{
struct ReadoutTexts
{
    std::string name;
    std::string route;
    std::string stressLabel;
    std::string stressValue;
    std::string hungerLegend;
    std::string woundsLegend;
    std::string tiredLegend;
    std::string beatenLegend;
    std::string chanceLabel;
    std::string chanceValue;
    // The cared-for row under the chance: "LO CUIDASTE" and "+12%".
    std::string caredForLabel;
    std::string caredForValue;
    std::string defeatsLabel;
    std::string defeatsValue;
    std::string foodLabel;
    std::string foodValue;
    std::string primary;
    std::string secondary;
};

// `who` only picks which message variant is used, as everywhere else.
ReadoutTexts BuildReadoutTexts(const ReadoutView& view, const std::string& name,
                               Character* who);
} // namespace totalrecruit
