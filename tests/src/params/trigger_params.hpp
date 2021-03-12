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

    TriggerParams& isDiscrete(bool val)
    {
        discreteProperty = std::move(val);
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

    TriggerParams& thresholdParams(TriggerThresholdParams val)
    {
        thresholdsProperty = std::move(val);
        return *this;
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
            {numeric::typeToString(numeric::Type::lowerCritical),
             std::chrono::milliseconds(10).count(),
             numeric::directionToString(numeric::Direction::decreasing), 0.0},
            {numeric::typeToString(numeric::Type::upperCritical),
             std::chrono::milliseconds(10).count(),
             numeric::directionToString(numeric::Direction::increasing), 90.0}};
};
