#pragma once

#include "interfaces/report_manager.hpp"
#include "interfaces/trigger_action.hpp"
#include "interfaces/trigger_types.hpp"

namespace action
{

class LogToJournalNumeric : public interfaces::TriggerAction
{
  public:
    LogToJournalNumeric(numeric::Type type, double val) :
        type(type), threshold(val)
    {}

    void commit(const std::string& id, uint64_t timestamp,
                double value) override;

  private:
    numeric::Type type;
    double threshold;

    const char* getType() const;
};

class LogToRedfishNumeric : public interfaces::TriggerAction
{
  public:
    LogToRedfishNumeric(numeric::Type type, double val) :
        type(type), threshold(val)
    {}

    void commit(const std::string& id, uint64_t timestamp,
                double value) override;

  private:
    numeric::Type type;
    double threshold;

    const char* getMessageId() const;
};

class LogToJournalDiscrete : public interfaces::TriggerAction
{
  public:
    LogToJournalDiscrete(discrete::Severity severity) : severity(severity)
    {}

    void commit(const std::string& id, uint64_t timestamp,
                double value) override;

  private:
    discrete::Severity severity;

    const char* getSeverity() const;
};

class LogToRedfishDiscrete : public interfaces::TriggerAction
{
  public:
    LogToRedfishDiscrete(discrete::Severity severity) : severity(severity)
    {}

    void commit(const std::string& id, uint64_t timestamp,
                double value) override;

  private:
    discrete::Severity severity;

    const char* getMessageId() const;
};

class UpdateReport : public interfaces::TriggerAction
{
  public:
    UpdateReport(interfaces::ReportManager& reportManager,
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
} // namespace action
