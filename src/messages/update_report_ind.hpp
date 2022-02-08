#pragma once

#include <string>
#include <vector>

namespace messages
{

struct UpdateReportInd
{
    std::vector<std::string> reportIds;
};

} // namespace messages
