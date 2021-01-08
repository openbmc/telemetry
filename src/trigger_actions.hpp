#pragma once

#include "interfaces/report_manager.hpp"
#include "interfaces/trigger_action.hpp"
#include "interfaces/trigger_types.hpp"

class LogToJournalAction : public interfaces::TriggerAction
{
  public:
    LogToJournalAction(numeric::Type type) : type(type)
    {}

    void commit(const std::string& id, uint64_t timestamp,
                double value) override;

  private:
    numeric::Type type;

    const char* getDirection() const;
    const char* getLevel() const;
};

class LogToRedfishAction : public interfaces::TriggerAction
{
  public:
    LogToRedfishAction(numeric::Type type) : type(type)
    {}

    void commit(const std::string& id, uint64_t timestamp,
                double value) override;

  private:
    numeric::Type type;

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

    void commit(const std::string& id, uint64_t timestamp,
                double value) override;

  private:
    interfaces::ReportManager& reportManager;
    std::vector<std::string> reportNames;
};
