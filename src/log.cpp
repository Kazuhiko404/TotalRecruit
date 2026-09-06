#include "common.h"
#include "log.h"

namespace totalrecruit
{
namespace
{
const char* const kLogPrefix = "[TotalRecruit] ";
}

void LogInfo(const std::string& message)
{
    DebugLog(std::string(kLogPrefix) + message);
}

void LogError(const std::string& message)
{
    ErrorLog(std::string(kLogPrefix) + message);
}
} // namespace totalrecruit
