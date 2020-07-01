#pragma once

#include <string>
#include <vector>

namespace core
{

enum class ReportAction
{
    log,
    event,
};

const std::vector<std::pair<ReportAction, std::string>>
    reportActionConvertData = {{ReportAction::log, "Log"},
                               {ReportAction::event, "Event"}};

} // namespace core
