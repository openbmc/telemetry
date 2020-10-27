#pragma once

#include "interfaces/report.hpp"
#include "interfaces/types.hpp"

#include <memory>
#include <string>

namespace interfaces
{

class ReportManager
{
  public:
    virtual ~ReportManager() = default;

    virtual void removeReport(const interfaces::Report* report) = 0;

    virtual std::unique_ptr<interfaces::Report>& addReport(
        const std::string& reportName, const std::string& reportingType,
        const bool emitsReadingsUpdate, const bool logToMetricReportsCollection,
        const uint64_t interval, const ReadingParameters& metricParams) = 0;
};

} // namespace interfaces
