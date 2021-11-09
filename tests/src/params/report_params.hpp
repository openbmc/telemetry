#pragma once

#include "report_manager.hpp"
#include "types/report_types.hpp"

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

    ReportParams& reportingType(const ReportingType val)
    {
        reportingTypeProperty = val;
        return *this;
    }

    ReportingType reportingType() const
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

    ReportParams& interval(Milliseconds val)
    {
        intervalProperty = val;
        return *this;
    }

    Milliseconds interval() const
    {
        return intervalProperty;
    }

    ReportParams& enabled(bool val)
    {
        enabledProperty = val;
        return *this;
    }

    bool enabled() const
    {
        return enabledProperty;
    }

    ReportParams& metricParameters(std::vector<LabeledMetricParameters> val)
    {
        metricParametersProperty = std::move(val);
        return *this;
    }

    const std::vector<LabeledMetricParameters>& metricParameters() const
    {
        return metricParametersProperty;
    }

  private:
    std::string reportNameProperty = "TestReport";
    ReportingType reportingTypeProperty = ReportingType::onRequest;
    bool emitReadingUpdateProperty = true;
    bool logToMetricReportCollectionProperty = true;
    Milliseconds intervalProperty = ReportManager::minInterval;
    std::vector<LabeledMetricParameters> metricParametersProperty{
        {LabeledMetricParameters{
             {LabeledSensorParameters{"Service",
                                      "/xyz/openbmc_project/sensors/power/p1"}},
             OperationType::single,
             "MetricId1",
             "Metadata1",
             CollectionTimeScope::point,
             CollectionDuration(Milliseconds(0u))},
         LabeledMetricParameters{
             {LabeledSensorParameters{"Service",
                                      "/xyz/openbmc_project/sensors/power/p2"}},
             OperationType::single,
             "MetricId2",
             "Metadata2",
             CollectionTimeScope::point,
             CollectionDuration(Milliseconds(0u))}}};
    bool enabledProperty = true;
};
