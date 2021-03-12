#pragma once

#include "interfaces/report_manager.hpp"
#include "interfaces/trigger_action.hpp"
#include "interfaces/trigger_types.hpp"

namespace action
{

namespace numeric
{
class LogToJournal : public interfaces::TriggerAction
{
  public:
    LogToJournal(::numeric::Type type, double val) : type(type), threshold(val)
    {}

    void commit(const std::string& id, uint64_t timestamp,
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

    void commit(const std::string& id, uint64_t timestamp,
                double value) override;

  private:
    ::numeric::Type type;
    double threshold;

    const char* getMessageId() const;
};
} // namespace numeric

namespace discrete
{
class LogToJournal : public interfaces::TriggerAction
{
  public:
    LogToJournal(::discrete::Severity severity) : severity(severity)
    {}

    void commit(const std::string& id, uint64_t timestamp,
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

    void commit(const std::string& id, uint64_t timestamp,
                double value) override;

  private:
    ::discrete::Severity severity;

    const char* getMessageId() const;
};
} // namespace discrete

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
