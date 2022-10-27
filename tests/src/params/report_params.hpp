#pragma once

#include "report_manager.hpp"
#include "types/report_types.hpp"

#include <chrono>
#include <string>

class ReportParams final
{
  public:
    ReportParams& reportId(std::string_view val)
    {
        reportIdProperty = val;
        return *this;
    }

    const std::string& reportId() const
    {
        return reportIdProperty;
    }

    ReportParams& reportName(std::string_view val)
    {
        reportNameProperty = val;
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

    ReportParams& reportActions(std::vector<ReportAction> val)
    {
        reportActionsProperty = std::move(val);
        return *this;
    }

    std::vector<ReportAction> reportActions() const
    {
        return reportActionsProperty;
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

    ReportParams& appendLimit(uint64_t val)
    {
        appendLimitProperty = val;
        return *this;
    }

    uint64_t appendLimit() const
    {
        return appendLimitProperty;
    }

    ReportParams& reportUpdates(ReportUpdates val)
    {
        reportUpdatesProperty = val;
        return *this;
    }

    ReportUpdates reportUpdates() const
    {
        return reportUpdatesProperty;
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

    ReportParams& readings(Readings val)
    {
        readingsProperty = std::move(val);
        return *this;
    }

    Readings readings() const
    {
        return readingsProperty;
    }

  private:
    std::string reportIdProperty = "TestId";
    std::string reportNameProperty = "TestReport";
    ReportingType reportingTypeProperty = ReportingType::onChange;
    std::vector<ReportAction> reportActionsProperty = {
        ReportAction::logToMetricReportsCollection};
    Milliseconds intervalProperty{};
    uint64_t appendLimitProperty = 123;
    ReportUpdates reportUpdatesProperty = ReportUpdates::overwrite;
    std::vector<LabeledMetricParameters> metricParametersProperty{
        {LabeledMetricParameters{
             {LabeledSensorInfo{"Service",
                                "/xyz/openbmc_project/sensors/power/p1",
                                "metadata1"}},
             OperationType::avg,
             CollectionTimeScope::point,
             CollectionDuration(Milliseconds(0u))},
         LabeledMetricParameters{
             {LabeledSensorInfo{"Service",
                                "/xyz/openbmc_project/sensors/power/p2",
                                "metadata2"}},
             OperationType::avg,
             CollectionTimeScope::point,
             CollectionDuration(Milliseconds(0u))}}};
    bool enabledProperty = true;
    Readings readingsProperty = {};
};
