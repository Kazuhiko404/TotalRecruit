// Tests for src/atomic_file.cpp against the real file system (a temp directory).

#include "test_support.h"
#include "atomic_file.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include <fstream>
#include <sstream>
#include <string>

using namespace totalrecruit;

namespace
{
std::string TempDir()
{
    char buffer[MAX_PATH] = { 0 };
    GetTempPathA(MAX_PATH, buffer);
    std::string dir = std::string(buffer) + "totalrecruit_atomic_test";
    CreateDirectoryA(dir.c_str(), NULL);
    return dir;
}

std::string ReadAll(const std::string& path)
{
    std::ifstream file(path.c_str(), std::ios::binary);
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool Exists(const std::string& path)
{
    return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

void CreatesANewFile()
{
    const char* name = "atomic_file: writes a file that did not exist";
    const std::string path = TempDir() + "\\fresh.json";
    DeleteFileA(path.c_str());
    std::string error;
    CHECK(WriteFileAtomically(path, "{ \"a\": 1 }\n", &error));
    CHECK(ReadAll(path) == "{ \"a\": 1 }\n");
    CHECK(!Exists(path + ".tmp"));
    DeleteFileA(path.c_str());
}

void ReplacesAnExistingFile()
{
    const char* name = "atomic_file: replaces the old content in one step";
    const std::string path = TempDir() + "\\replace.json";
    {
        std::ofstream old(path.c_str(), std::ios::binary | std::ios::trunc);
        old << "OLD CONTENT THAT IS LONGER THAN THE NEW ONE";
    }
    std::string error;
    CHECK(WriteFileAtomically(path, "new", &error));
    CHECK(ReadAll(path) == "new");     // no tail of the old file
    CHECK(!Exists(path + ".tmp"));
    DeleteFileA(path.c_str());
}

void LeavesNoTempFileAndKeepsBytesExact()
{
    const char* name = "atomic_file: binary content (CRLF, NUL) is written exactly";
    const std::string path = TempDir() + "\\bytes.json";
    const std::string content("a\r\nb\0c\n", 7);
    std::string error;
    CHECK(WriteFileAtomically(path, content, &error));
    CHECK(ReadAll(path) == content);
    DeleteFileA(path.c_str());
}

void FailureLeavesTheTargetAlone()
{
    const char* name = "atomic_file: a failed write reports it and does not touch the target";
    std::string error;
    const std::string missingDir = TempDir() + "\\no_such_dir\\file.json";
    CHECK(!WriteFileAtomically(missingDir, "x", &error));
    CHECK(!error.empty());
    CHECK(!Exists(missingDir));

    const std::string empty;
    CHECK(!WriteFileAtomically(empty, "x", &error));
}

void ALockedTargetKeepsItsContentAndTheTempIsRemoved()
{
    const char* name = "atomic_file: if the target cannot be replaced it keeps its old content";
    const std::string path = TempDir() + "\\locked.json";
    {
        std::ofstream old(path.c_str(), std::ios::binary | std::ios::trunc);
        old << "KEEP ME";
    }
    // Another process holding the file open without delete sharing blocks the replace.
    HANDLE hold = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    CHECK(hold != INVALID_HANDLE_VALUE);

    std::string error;
    const bool wrote = WriteFileAtomically(path, "replacement", &error);
    CloseHandle(hold);

    CHECK(!wrote);
    CHECK(ReadAll(path) == "KEEP ME");
    CHECK(!Exists(path + ".tmp"));
    DeleteFileA(path.c_str());
}
} // namespace

void RunAtomicFileTests()
{
    CreatesANewFile();
    ReplacesAnExistingFile();
    LeavesNoTempFileAndKeepsBytesExact();
    FailureLeavesTheTargetAlone();
    ALockedTargetKeepsItsContentAndTheTempIsRemoved();
}
