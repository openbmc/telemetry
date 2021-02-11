#pragma once

#include "report_manager.hpp"
#include "types/types.hpp"

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

    ReportParams& logToMetricReportCollection(bool val)
    {
        logToMetricReportCollectionProperty = val;
        return *this;
    }

    bool logToMetricReportCollection() const
    {
        return logToMetricReportCollectionProperty;
    }

    ReportParams& interval(std::chrono::milliseconds val)
    {
        intervalProperty = val;
        return *this;
    }

    std::chrono::milliseconds interval() const
    {
        return intervalProperty;
    }

    ReportParams& readingParameters(ReadingParameters val)
    {
        readingParametersProperty = std::move(val);
        return *this;
    }

    const ReadingParameters& readingParameters() const
    {
        return readingParametersProperty;
    }

  private:
    std::string reportNameProperty = "TestReport";
    std::string reportingTypeProperty = "OnRequest";
    bool emitReadingUpdateProperty = true;
    bool logToMetricReportCollectionProperty = true;
    std::chrono::milliseconds intervalProperty = ReportManager::minInterval;
    ReadingParameters readingParametersProperty = {
        {{sdbusplus::message::object_path(
             "/xyz/openbmc_project/sensors/power/p1")},
         utils::enumToString(OperationType::single),
         "MetricId1",
         "Metadata1",
         utils::enumToString(CollectionTimeScope::point),
         0u},
        {{sdbusplus::message::object_path(
             "/xyz/openbmc_project/sensors/power/p2")},
         utils::enumToString(OperationType::single),
         "MetricId2",
         "Metadata2",
         utils::enumToString(CollectionTimeScope::point),
         0u}};
};
