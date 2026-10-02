#include "json_merge.h"

#include <cstdio>
#include <cstdlib>
#include <cmath>

namespace totalrecruit
{
namespace
{
struct Member
{
    std::string key;
    size_t keyStart;
    size_t valueStart;
    size_t valueEnd;
    int depth;

    Member() : keyStart(0), valueStart(0), valueEnd(0), depth(0) {}
};

bool IsSpace(char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

// Finds where every member's value starts and ends. Understands strings (with
// escapes), nested objects and arrays, // and /* */ comments and a trailing comma;
// anything else is an opaque literal that runs to the next delimiter. It does not
// validate numbers or keywords -- the config reader treats those leniently too.
class Scanner
{
public:
    explicit Scanner(const std::string& text)
        : text_(text), pos_(0), openBrace_(0), lastSigEnd_(0), topMembers_(0)
    {
    }

    bool Run()
    {
        // A byte-order mark belongs to the file, not to the JSON.
        if (text_.compare(0, 3, "\xEF\xBB\xBF") == 0)
        {
            pos_ = 3;
        }
        if (!SkipSpace() || pos_ >= text_.size() || text_[pos_] != '{')
        {
            return false;
        }
        if (!ParseObject(1))
        {
            return false;
        }
        if (!SkipSpace())
        {
            return false;
        }
        return pos_ >= text_.size();   // nothing but whitespace and comments after
    }

    const std::vector<Member>& Members() const { return members_; }
    size_t OpenBrace() const { return openBrace_; }
    size_t LastSignificantEnd() const { return lastSigEnd_; }
    int TopLevelMembers() const { return topMembers_; }

private:
    bool AtEnd() const { return pos_ >= text_.size(); }

    // Skips whitespace and comments. False for an unterminated block comment.
    bool SkipSpace()
    {
        while (!AtEnd())
        {
            const char c = text_[pos_];
            if (IsSpace(c))
            {
                ++pos_;
            }
            else if (c == '/' && pos_ + 1 < text_.size() && text_[pos_ + 1] == '/')
            {
                while (!AtEnd() && text_[pos_] != '\n')
                {
                    ++pos_;
                }
            }
            else if (c == '/' && pos_ + 1 < text_.size() && text_[pos_ + 1] == '*')
            {
                const size_t close = text_.find("*/", pos_ + 2);
                if (close == std::string::npos)
                {
                    return false;
                }
                pos_ = close + 2;
            }
            else
            {
                break;
            }
        }
        return true;
    }

    // pos_ is at the opening quote; leaves it after the closing one.
    bool ParseString(std::string* raw)
    {
        const size_t open = pos_;
        ++pos_;
        while (!AtEnd())
        {
            const char c = text_[pos_];
            if (c == '\\')
            {
                pos_ += 2;
                continue;
            }
            if (c == '"')
            {
                if (raw != NULL)
                {
                    *raw = text_.substr(open + 1, pos_ - open - 1);
                }
                ++pos_;
                return true;
            }
            ++pos_;
        }
        return false;
    }

    bool StartsComment() const
    {
        return text_[pos_] == '/' && pos_ + 1 < text_.size() &&
               (text_[pos_ + 1] == '/' || text_[pos_ + 1] == '*');
    }

    bool ParseValue(int depth)
    {
        if (AtEnd())
        {
            return false;
        }
        const char c = text_[pos_];
        if (c == '"')
        {
            return ParseString(NULL);
        }
        if (c == '{')
        {
            return ParseObject(depth + 1);
        }
        if (c == '[')
        {
            return ParseArray(depth + 1);
        }

        const size_t start = pos_;
        while (!AtEnd())
        {
            const char d = text_[pos_];
            if (d == ',' || d == '}' || d == ']' || IsSpace(d) || StartsComment())
            {
                break;
            }
            ++pos_;
        }
        return pos_ > start;
    }

    bool ParseArray(int depth)
    {
        ++pos_;   // '['
        for (;;)
        {
            if (!SkipSpace() || AtEnd())
            {
                return false;
            }
            if (text_[pos_] == ']')
            {
                ++pos_;
                return true;
            }
            if (!ParseValue(depth))
            {
                return false;
            }
            if (!SkipSpace() || AtEnd())
            {
                return false;
            }
            if (text_[pos_] == ',')
            {
                ++pos_;
            }
            else if (text_[pos_] != ']')
            {
                return false;
            }
        }
    }

    bool ParseObject(int depth)
    {
        if (depth == 1)
        {
            openBrace_ = pos_;
        }
        ++pos_;   // '{'
        for (;;)
        {
            if (!SkipSpace() || AtEnd())
            {
                return false;
            }
            if (text_[pos_] == '}')
            {
                ++pos_;
                return true;
            }
            if (text_[pos_] != '"')
            {
                return false;
            }

            Member member;
            member.depth = depth;
            member.keyStart = pos_;
            if (!ParseString(&member.key))
            {
                return false;
            }
            if (!SkipSpace() || AtEnd() || text_[pos_] != ':')
            {
                return false;
            }
            ++pos_;
            if (!SkipSpace())
            {
                return false;
            }
            member.valueStart = pos_;
            if (!ParseValue(depth))
            {
                return false;
            }
            member.valueEnd = pos_;
            members_.push_back(member);
            if (depth == 1)
            {
                ++topMembers_;
                lastSigEnd_ = member.valueEnd;
            }

            if (!SkipSpace() || AtEnd())
            {
                return false;
            }
            if (text_[pos_] == ',')
            {
                ++pos_;
                if (depth == 1)
                {
                    lastSigEnd_ = pos_;
                }
            }
            else if (text_[pos_] != '}')
            {
                return false;
            }
        }
    }

    const std::string& text_;
    size_t pos_;
    size_t openBrace_;
    size_t lastSigEnd_;
    int topMembers_;
    std::vector<Member> members_;
};

std::string Trim(const std::string& text)
{
    size_t a = 0;
    size_t b = text.size();
    while (a < b && IsSpace(text[a])) ++a;
    while (b > a && IsSpace(text[b - 1])) --b;
    return text.substr(a, b - a);
}

// The text without whitespace outside strings, so [ "a",  "b" ] equals ["a","b"].
std::string Canonical(const std::string& value)
{
    std::string out;
    bool inString = false;
    for (size_t i = 0; i < value.size(); ++i)
    {
        const char c = value[i];
        if (inString)
        {
            out += c;
            if (c == '\\' && i + 1 < value.size())
            {
                out += value[++i];
            }
            else if (c == '"')
            {
                inString = false;
            }
        }
        else if (c == '"')
        {
            inString = true;
            out += c;
        }
        else if (!IsSpace(c))
        {
            out += c;
        }
    }
    return out;
}

bool LooksNumeric(const std::string& text)
{
    if (text.empty())
    {
        return false;
    }
    const char c = text[0];
    return (c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.';
}

bool ParseNumber(const std::string& text, double* out)
{
    char* end = NULL;
    *out = strtod(text.c_str(), &end);
    return end != NULL && *end == '\0' && end != text.c_str();
}

// "Already what we would write": numbers compare as the floats the config holds
// (so 0.60 equals 0.6 and 6.0 equals 6), everything else by its text.
bool ValuesEqual(const std::string& existing, const std::string& wanted)
{
    const std::string a = Trim(existing);
    const std::string b = Trim(wanted);
    if (LooksNumeric(a) && LooksNumeric(b))
    {
        double x = 0.0;
        double y = 0.0;
        if (ParseNumber(a, &x) && ParseNumber(b, &y))
        {
            return (float)x == (float)y;
        }
    }
    return Canonical(a) == Canonical(b);
}

std::string DetectIndent(const std::string& text, const std::vector<Member>& members)
{
    const Member* first = NULL;
    for (size_t i = 0; i < members.size(); ++i)
    {
        if (members[i].depth == 1 && (first == NULL || members[i].keyStart < first->keyStart))
        {
            first = &members[i];
        }
    }
    if (first == NULL)
    {
        return "  ";
    }

    size_t lineStart = first->keyStart;
    while (lineStart > 0 && text[lineStart - 1] != '\n')
    {
        --lineStart;
    }
    const std::string lead = text.substr(lineStart, first->keyStart - lineStart);
    for (size_t i = 0; i < lead.size(); ++i)
    {
        if (lead[i] != ' ' && lead[i] != '\t')
        {
            return "  ";
        }
    }
    return lead.empty() ? std::string("  ") : lead;
}

const Member* FindFirst(const std::vector<Member>& members, const std::string& key)
{
    const Member* best = NULL;
    for (size_t i = 0; i < members.size(); ++i)
    {
        if (members[i].key == key && (best == NULL || members[i].keyStart < best->keyStart))
        {
            best = &members[i];
        }
    }
    return best;
}

struct Replacement
{
    size_t start;
    size_t end;
    std::string value;
};

std::string Lines(const std::vector<JsonUpdate>& items, const std::string& indent,
                  const std::string& newline)
{
    std::string out;
    for (size_t i = 0; i < items.size(); ++i)
    {
        if (i > 0)
        {
            out += "," + newline;
        }
        out += indent + JsonString(items[i].key) + ": " + items[i].value;
    }
    return out;
}

// Later updates for the same key win; the first one's position is kept.
std::vector<JsonUpdate> Deduplicated(const std::vector<JsonUpdate>& updates)
{
    std::vector<JsonUpdate> out;
    for (size_t i = 0; i < updates.size(); ++i)
    {
        bool merged = false;
        for (size_t j = 0; j < out.size(); ++j)
        {
            if (out[j].key == updates[i].key)
            {
                out[j].value = updates[i].value;
                merged = true;
                break;
            }
        }
        if (!merged)
        {
            out.push_back(updates[i]);
        }
    }
    return out;
}

bool IsBlank(const std::string& text)
{
    size_t start = 0;
    if (text.compare(0, 3, "\xEF\xBB\xBF") == 0)
    {
        start = 3;
    }
    for (size_t i = start; i < text.size(); ++i)
    {
        if (!IsSpace(text[i]))
        {
            return false;
        }
    }
    return true;
}
} // namespace

MergeResult MergeJsonKeys(const std::string& existing, const std::vector<JsonUpdate>& rawUpdates)
{
    MergeResult result;
    const std::vector<JsonUpdate> updates = Deduplicated(rawUpdates);

    if (IsBlank(existing))
    {
        result.ok = true;
        result.text = "{\n" + Lines(updates, "  ", "\n") + (updates.empty() ? "" : "\n") + "}\n";
        result.appended = (int)updates.size();
        return result;
    }

    Scanner scanner(existing);
    if (!scanner.Run())
    {
        return result;   // ok == false
    }

    const std::vector<Member>& members = scanner.Members();
    std::vector<Replacement> replacements;
    std::vector<JsonUpdate> missing;

    for (size_t i = 0; i < updates.size(); ++i)
    {
        const Member* found = FindFirst(members, updates[i].key);
        if (found == NULL)
        {
            missing.push_back(updates[i]);
            continue;
        }

        const std::string current = existing.substr(found->valueStart,
                                                    found->valueEnd - found->valueStart);
        if (ValuesEqual(current, updates[i].value))
        {
            ++result.kept;
            continue;
        }

        Replacement r;
        r.start = found->valueStart;
        r.end = found->valueEnd;
        r.value = updates[i].value;
        replacements.push_back(r);
        ++result.replaced;
    }

    std::string text = existing;

    // The append sits after every member, so it goes in first and the replacements
    // (all before it) keep their offsets.
    if (!missing.empty())
    {
        const std::string newline = (existing.find("\r\n") != std::string::npos) ? "\r\n" : "\n";
        const std::string body = Lines(missing, DetectIndent(existing, members), newline);
        if (scanner.TopLevelMembers() > 0)
        {
            const size_t at = scanner.LastSignificantEnd();
            const bool hasComma = at > 0 && existing[at - 1] == ',';
            text.insert(at, std::string(hasComma ? "" : ",") + newline + body);
        }
        else
        {
            text.insert(scanner.OpenBrace() + 1, newline + body + newline);
        }
        result.appended = (int)missing.size();
    }

    // Highest offset first so earlier ones stay valid.
    for (size_t i = 0; i < replacements.size(); ++i)
    {
        for (size_t j = i + 1; j < replacements.size(); ++j)
        {
            if (replacements[j].start > replacements[i].start)
            {
                const Replacement swap = replacements[i];
                replacements[i] = replacements[j];
                replacements[j] = swap;
            }
        }
    }
    for (size_t i = 0; i < replacements.size(); ++i)
    {
        text.replace(replacements[i].start, replacements[i].end - replacements[i].start,
                     replacements[i].value);
    }

    result.ok = true;
    result.text = text;
    return result;
}

std::string JsonBool(bool value)
{
    return value ? "true" : "false";
}

std::string JsonInt(int value)
{
    char buffer[32] = { 0 };
    _snprintf_s(buffer, sizeof(buffer), _TRUNCATE, "%d", value);
    return buffer;
}

std::string JsonFloat(float value)
{
    if (!(value == value) || value > 3.4e38f || value < -3.4e38f)
    {
        return "0";   // JSON has no NaN or infinity
    }

    char buffer[48] = { 0 };
    for (int digits = 1; digits <= 9; ++digits)
    {
        _snprintf_s(buffer, sizeof(buffer), _TRUNCATE, "%.*g", digits, (double)value);
        if ((float)atof(buffer) == value)
        {
            break;
        }
    }
    return buffer;
}

std::string JsonString(const std::string& value)
{
    std::string out = "\"";
    for (size_t i = 0; i < value.size(); ++i)
    {
        const unsigned char c = (unsigned char)value[i];
        switch (c)
        {
        case '"':  out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            if (c < 0x20)
            {
                char escape[8] = { 0 };
                _snprintf_s(escape, sizeof(escape), _TRUNCATE, "\\u%04x", (unsigned)c);
                out += escape;
            }
            else
            {
                out += (char)c;
            }
        }
    }
    out += "\"";
    return out;
}

std::string JsonStringArray(const std::vector<std::string>& values)
{
    std::string out = "[";
    for (size_t i = 0; i < values.size(); ++i)
    {
        if (i > 0)
        {
            out += ", ";
        }
        out += JsonString(values[i]);
    }
    out += "]";
    return out;
}
} // namespace totalrecruit
