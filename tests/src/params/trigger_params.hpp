#pragma once

#include "interfaces/types.hpp"

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

    const TriggerThresholdParams& thresholds() const
    {
        return thresholdsProperty;
    }

  private:
    std::string nameProperty = "Trigger1";
    bool discreteProperty = false;
    bool logToJournalProperty = false;
    bool logToRedfishProperty = false;
    bool updateReportProperty = false;
    std::vector<sdbusplus::message::object_path> sensorsProperty = {};
    std::vector<std::string> reportNamesProperty = {};
    TriggerThresholdParams thresholdsProperty = {};
};
