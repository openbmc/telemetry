#pragma once

#include <string>
#include <vector>

namespace core
{

enum class ReportingType
{
    periodic,
    onChange,
    onRequest
};

const std::vector<std::pair<ReportingType, std::string>>
    reportingTypeConvertData = {{ReportingType::periodic, "Periodic"},
                                {ReportingType::onChange, "OnChange"},
                                {ReportingType::onRequest, "OnRequest"}};

} // namespace core
