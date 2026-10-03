#include "readout_format.h"

#include <cmath>
#include <cstdio>

namespace totalrecruit
{
std::string FormatCats(int cats)
{
    char digits[16] = { 0 };
    const bool negative = cats < 0;
    // -(INT_MIN) overflows; a price is never anywhere near it.
    unsigned int magnitude = negative ? (unsigned int)(-(long long)cats) : (unsigned int)cats;
    _snprintf_s(digits, sizeof(digits), _TRUNCATE, "%u", magnitude);

    std::string text;
    const int length = (int)std::string(digits).size();
    for (int i = 0; i < length; ++i)
    {
        if (i > 0 && (length - i) % 3 == 0)
        {
            text += '.';
        }
        text += digits[i];
    }
    return negative ? "-" + text : text;
}

std::string FormatHoursLeft(float hours)
{
    char buf[24] = { 0 };
    if (hours <= 0.0f)
    {
        return "0min";
    }
    if (hours < 1.0f)
    {
        int minutes = (int)std::ceil(hours * 60.0f);
        if (minutes < 1) minutes = 1;
        if (minutes > 59) minutes = 59;
        _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%dmin", minutes);
        return buf;
    }
    // A hair under a whole number (3.0000002 after float arithmetic) is that
    // number, not the next one.
    _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%dh", (int)std::ceil(hours - 0.0001f));
    return buf;
}

namespace
{
void ReplaceAll(std::string* text, const char* token, const std::string& value)
{
    const std::string needle = token;
    std::string::size_type at = 0;
    while ((at = text->find(needle, at)) != std::string::npos)
    {
        text->replace(at, needle.size(), value);
        at += value.size();
    }
}

std::string Number(int value)
{
    char buf[16] = { 0 };
    _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%d", value);
    return buf;
}
} // namespace

std::string FillLine(const std::string& text, const LineArgs& args)
{
    std::string out = text;
    ReplaceAll(&out, "{cats}", FormatCats(args.cats));
    ReplaceAll(&out, "{purse}", FormatCats(args.purse));
    ReplaceAll(&out, "{hours}", FormatHoursLeft(args.hours));
    ReplaceAll(&out, "{food}", Number(args.food));
    ReplaceAll(&out, "{need}", Number(args.need));
    ReplaceAll(&out, "{chance}", Number(args.chance));
    return out;
}
} // namespace totalrecruit
