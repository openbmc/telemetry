#pragma once

#include "interfaces/trigger_types.hpp"

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

    const std::vector<std::pair<sdbusplus::message::object_path, std::string>>&
        sensors() const
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
    std::vector<std::pair<sdbusplus::message::object_path, std::string>>
        sensorsProperty = {
            {sdbusplus::message::object_path(
                 "/xyz/openbmc_project/sensors/temperature/BMC_Temp"),
             ""}};
    std::vector<std::string> reportNamesProperty = {"Report1"};
    TriggerThresholdParams thresholdsProperty =
        std::vector<numeric::ThresholdParam>{
            {static_cast<int>(numeric::Type::lowerCritical),
             std::chrono::milliseconds(10).count(),
             static_cast<int>(numeric::Direction::decreasing), 0.0},
            {static_cast<int>(numeric::Type::upperCritical),
             std::chrono::milliseconds(10).count(),
             static_cast<int>(numeric::Direction::increasing), 90.0}};
};
