#include "atomic_file.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include <cstdio>

namespace totalrecruit
{
namespace
{
std::string LastErrorText(const char* what)
{
    char buffer[96] = { 0 };
    _snprintf_s(buffer, sizeof(buffer), _TRUNCATE, "%s failed (Windows error %lu)", what,
                (unsigned long)GetLastError());
    return buffer;
}

void Report(std::string* error, const std::string& text)
{
    if (error != NULL)
    {
        *error = text;
    }
}
} // namespace

bool WriteFileAtomically(const std::string& path, const std::string& content,
                         std::string* error)
{
    if (path.empty())
    {
        Report(error, "empty path");
        return false;
    }

    // Same directory as the target: a rename only stays atomic within one volume.
    const std::string temp = path + ".tmp";
    HANDLE file = CreateFileA(temp.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        Report(error, LastErrorText("creating the temporary file"));
        return false;
    }

    bool ok = true;
    size_t written = 0;
    while (ok && written < content.size())
    {
        DWORD chunk = 0;
        const DWORD want = (DWORD)(content.size() - written);
        ok = WriteFile(file, content.data() + written, want, &chunk, NULL) != 0 && chunk > 0;
        written += chunk;
    }
    if (!ok)
    {
        Report(error, LastErrorText("writing the temporary file"));
    }
    else if (!FlushFileBuffers(file))
    {
        Report(error, LastErrorText("flushing the temporary file"));
        ok = false;
    }
    CloseHandle(file);

    if (ok && !MoveFileExA(temp.c_str(), path.c_str(),
                           MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        Report(error, LastErrorText("replacing the target"));
        ok = false;
    }

    if (!ok)
    {
        DeleteFileA(temp.c_str());
    }
    return ok;
}
} // namespace totalrecruit
