#pragma once

// Writing a file so a crash cannot leave half of it.

#include <string>

namespace totalrecruit
{
// Writes `content` to `path` through a temporary file in the same directory and
// replaces the target in one step, so the file is always either the old content or
// the new content. The temporary file is removed on failure. Returns false (and
// fills `error` when given) if anything went wrong; the target is untouched then.
bool WriteFileAtomically(const std::string& path, const std::string& content,
                         std::string* error);
} // namespace totalrecruit
