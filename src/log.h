#pragma once

#include <string>

namespace totalrecruit
{
// Every message is prefixed and routed to the RE_Kenshi debug log.
void LogInfo(const std::string& message);
void LogError(const std::string& message);
} // namespace totalrecruit
