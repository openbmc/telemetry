#pragma once

#include "interfaces/types.hpp"

#include <chrono>

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

    bool updateReport() const
    {
        return updateReportProperty;
    }

    const std::vector<sdbusplus::message::object_path>& sensors() const
    {
        return sensorsProperty;
    }

    const std::vector<std::string>& reportNames() const
    {
        return reportNamesProperty;
    }

    const TriggerThresholdParams& thresholdParams() const
    {
        return thresholdsProperty;
    }

  private:
    std::string nameProperty = "Trigger1";
    bool discreteProperty = false;
    bool logToJournalProperty = false;
    bool logToRedfishProperty = false;
    bool updateReportProperty = false;
    std::vector<sdbusplus::message::object_path> sensorsProperty = {
        sdbusplus::message::object_path(
            "/xyz/openbmc_project/sensors/temperature/BMC_Temp")};
    std::vector<std::string> reportNamesProperty = {"Report1"};
    TriggerThresholdParams thresholdsProperty =
        std::vector<numeric::ThresholdParam>{
            {numeric::Level::lowerCritical,
             std::chrono::milliseconds(10).count(),
             numeric::Direction::decreasing, 0.0},
            {numeric::Level::upperCritical,
             std::chrono::milliseconds(10).count(),
             numeric::Direction::increasing, 90.0}};
};
