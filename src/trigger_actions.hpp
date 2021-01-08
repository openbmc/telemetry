#pragma once

#include "interfaces/report_manager.hpp"
#include "interfaces/trigger_action.hpp"
#include "interfaces/types.hpp"

#include <vector>

class LogToJournalAction : public interfaces::TriggerAction
{
  public:
    LogToJournalAction(numeric::Level level, numeric::Direction direction,
                       double val) :
        level(level),
        direction(direction), thresholdValue(val)
    {}

    void commit(const interfaces::Sensor::Id& id, uint64_t timestamp,
                double value) override;

  private:
    numeric::Level level;
    numeric::Direction direction;
    double thresholdValue;

    const char* getDirection() const;
    const char* getLevel() const;
};

class LogToRedfishAction : public interfaces::TriggerAction
{
  public:
    LogToRedfishAction(numeric::Level level, double val) :
        level(level), thresholdValue(val)
    {}

    void commit(const interfaces::Sensor::Id& id, uint64_t timestamp,
                double value) override;

  private:
    numeric::Level level;
    double thresholdValue;

    const char* getMessageId() const;
};

class UpdateReportAction : public interfaces::TriggerAction
{
  public:
    UpdateReportAction(interfaces::ReportManager& reportManager,
                       std::vector<std::string> names) :
        reportManager(reportManager),
        reportNames(std::move(names))
    {}

    void commit(const interfaces::Sensor::Id& id, uint64_t timestamp,
                double value) override;

  private:
    interfaces::ReportManager& reportManager;
    std::vector<std::string> reportNames;
};
