#pragma once

#include "interfaces/json_storage.hpp"
#include "interfaces/report.hpp"
#include "interfaces/report_manager.hpp"
#include "interfaces/types.hpp"

#include <chrono>
#include <memory>

namespace interfaces
{

class ReportFactory
{
  public:
    virtual ~ReportFactory() = default;

    virtual std::unique_ptr<interfaces::Report> make(
        const std::string& name, const std::string& reportingType,
        bool emitsReadingsSignal, bool logToMetricReportsCollection,
        std::chrono::milliseconds period, const ReadingParameters& metricParams,
        ReportManager& reportManager, JsonStorage& reportStorage) const = 0;
};

} // namespace interfaces
