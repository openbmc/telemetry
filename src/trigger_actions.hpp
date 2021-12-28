#pragma once

#include "interfaces/report_manager.hpp"
#include "interfaces/trigger_action.hpp"
#include "types/trigger_types.hpp"

namespace action
{

namespace numeric
{
class LogToJournal : public interfaces::TriggerAction
{
  public:
    LogToJournal(::numeric::Type type, double val) : type(type), threshold(val)
    {}

    void commit(const std::string& id, Milliseconds timestamp,
                double value) override;

  private:
    ::numeric::Type type;
    double threshold;

    const char* getType() const;
};

class LogToRedfish : public interfaces::TriggerAction
{
  public:
    LogToRedfish(::numeric::Type type, double val) : type(type), threshold(val)
    {}

    void commit(const std::string& id, Milliseconds timestamp,
                double value) override;

  private:
    ::numeric::Type type;
    double threshold;

    const char* getMessageId() const;
};

void fillActions(
    std::vector<std::unique_ptr<interfaces::TriggerAction>>& actionsIf,
    const std::vector<TriggerAction>& ActionsEnum, ::numeric::Type type,
    double thresholdValue, interfaces::ReportManager& reportManager,
    const std::vector<std::string>& reportIds);
} // namespace numeric

namespace discrete
{
class LogToJournal : public interfaces::TriggerAction
{
  public:
    LogToJournal(::discrete::Severity severity) : severity(severity)
    {}

    void commit(const std::string& id, Milliseconds timestamp,
                double value) override;

  private:
    ::discrete::Severity severity;

    const char* getSeverity() const;
};

class LogToRedfish : public interfaces::TriggerAction
{
  public:
    LogToRedfish(::discrete::Severity severity) : severity(severity)
    {}

    void commit(const std::string& id, Milliseconds timestamp,
                double value) override;

  private:
    ::discrete::Severity severity;

    const char* getMessageId() const;
};

void fillActions(
    std::vector<std::unique_ptr<interfaces::TriggerAction>>& actionsIf,
    const std::vector<TriggerAction>& ActionsEnum,
    ::discrete::Severity severity, interfaces::ReportManager& reportManager,
    const std::vector<std::string>& reportIds);

namespace onChange
{
class LogToJournal : public interfaces::TriggerAction
{
  public:
    LogToJournal()
    {}

    void commit(const std::string& id, Milliseconds timestamp,
                double value) override;
};

class LogToRedfish : public interfaces::TriggerAction
{
  public:
    LogToRedfish()
    {}

    void commit(const std::string& id, Milliseconds timestamp,
                double value) override;
};

void fillActions(
    std::vector<std::unique_ptr<interfaces::TriggerAction>>& actionsIf,
    const std::vector<TriggerAction>& ActionsEnum,
    interfaces::ReportManager& reportManager,
    const std::vector<std::string>& reportIds);
} // namespace onChange

} // namespace discrete

class UpdateReport : public interfaces::TriggerAction
{
  public:
    UpdateReport(interfaces::ReportManager& reportManager,
                 std::vector<std::string> ids) :
        reportManager(reportManager),
        reportIds(std::move(ids))
    {}

    void commit(const std::string& id, Milliseconds timestamp,
                double value) override;

  private:
    interfaces::ReportManager& reportManager;
    std::vector<std::string> reportIds;
};
} // namespace action
