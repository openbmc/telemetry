#pragma once

#include "interfaces/types.hpp"
#include "report_manager.hpp"

#include <chrono>
#include <string>

class ReportParams final
{
  public:
    ReportParams& reportName(std::string val)
    {
        reportNameProperty = std::move(val);
        return *this;
    }

    const std::string& reportName() const
    {
        return reportNameProperty;
    }

    ReportParams& reportingType(std::string val)
    {
        reportingTypeProperty = std::move(val);
        return *this;
    }

    const std::string& reportingType() const
    {
        return reportingTypeProperty;
    }

    ReportParams& emitReadingUpdate(bool val)
    {
        emitReadingUpdateProperty = val;
        return *this;
    }

    bool emitReadingUpdate() const
    {
        return emitReadingUpdateProperty;
    }

    bool logToMetricReportCollection() const
    {
        return logToMetricReportCollectionProperty;
    }

    uint64_t interval() const
    {
        return intervalProperty;
    }

    std::chrono::milliseconds intervalDuration() const
    {
        return std::chrono::milliseconds(intervalProperty);
    }

    const ReadingParameters& readingParameters() const
    {
        return readingParametersProperty;
    }

  protected:
    std::string reportNameProperty = "TestReport";
    std::string reportingTypeProperty = "OnRequest";
    bool emitReadingUpdateProperty = true;
    bool logToMetricReportCollectionProperty = true;
    uint64_t intervalProperty = ReportManager::minInterval.count();
    ReadingParameters readingParametersProperty = {
        {{sdbusplus::message::object_path(
             "/xyz/openbmc_project/sensors/power/p1")},
         "SINGLE",
         "MetricId1",
         "Metadata1"},
        {{sdbusplus::message::object_path(
             "/xyz/openbmc_project/sensors/power/p2")},
         "SINGLE",
         "MetricId2",
         "Metadata2"},
    };
};
