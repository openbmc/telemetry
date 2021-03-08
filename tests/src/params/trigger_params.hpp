#pragma once

#include "types/milliseconds.hpp"
#include "types/trigger_types.hpp"

#include <sdbusplus/message.hpp>

#include <chrono>
#include <utility>

class TriggerParams
{
  public:
    TriggerParams& name(std::string val)
    {
        nameProperty = std::move(val);
        return *this;
    }

    const std::string& name() const
    {
        return nameProperty;
    }

    TriggerParams& isDiscrete(bool val)
    {
        discreteProperty = val;
        return *this;
    }

    bool isDiscrete() const
    {
        return discreteProperty;
    }

    bool logToJournal() const
    {
        return logToJournalProperty;
    }

    bool logToRedfish() const
    {
        return logToRedfishProperty;
    }

    TriggerParams& updateReport(bool updateReport)
    {
        updateReportProperty = updateReport;
        return *this;
    }

    bool updateReport() const
    {
        return updateReportProperty;
    }

    const std::vector<LabeledSensorInfo>& sensors() const
    {
        return labeledSensorsProperty;
    }

    const std::vector<std::string>& reportNames() const
    {
        return reportNamesProperty;
    }

    TriggerParams& thresholdParams(LabeledTriggerThresholdParams val)
    {
        labeledThresholdsProperty = std::move(val);
        return *this;
    }

    const LabeledTriggerThresholdParams& thresholdParams() const
    {
        return labeledThresholdsProperty;
    }

  private:
    std::string nameProperty = "Trigger1";
    bool discreteProperty = false;
    bool logToJournalProperty = false;
    bool logToRedfishProperty = false;
    bool updateReportProperty = true;
    std::vector<LabeledSensorInfo> labeledSensorsProperty = {
        {"service1", "/xyz/openbmc_project/sensors/temperature/BMC_Temp",
         "metadata1"}};

    std::vector<std::string> reportNamesProperty = {"Report1"};

    LabeledTriggerThresholdParams labeledThresholdsProperty =
        std::vector<numeric::LabeledThresholdParam>{
            numeric::LabeledThresholdParam{numeric::Type::lowerCritical,
                                           Milliseconds(10).count(),
                                           numeric::Direction::decreasing, 0.5},
            numeric::LabeledThresholdParam{
                numeric::Type::upperCritical, Milliseconds(10).count(),
                numeric::Direction::increasing, 90.2}};
};
