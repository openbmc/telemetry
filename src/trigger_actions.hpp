#pragma once

#include "interfaces/trigger_action.hpp"
#include "types/trigger_types.hpp"

#include <boost/asio/io_context.hpp>

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
    double thresholdValue, boost::asio::io_context& ioc,
    const std::shared_ptr<std::vector<std::string>>& reportIds);
} // namespace numeric

namespace discrete
{
class LogToJournal : public interfaces::TriggerAction
{
  public:
    explicit LogToJournal(::discrete::Severity severity) : severity(severity)
    {}

    void commit(const std::string& id, Milliseconds timestamp,
                double value) override;

  private:
    ::discrete::Severity severity;
};

class LogToRedfish : public interfaces::TriggerAction
{
  public:
    explicit LogToRedfish(::discrete::Severity severity) : severity(severity)
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
    ::discrete::Severity severity, boost::asio::io_context& ioc,
    const std::shared_ptr<std::vector<std::string>>& reportIds);

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
    const std::vector<TriggerAction>& ActionsEnum, boost::asio::io_context& ioc,
    const std::shared_ptr<std::vector<std::string>>& reportIds);
} // namespace onChange

} // namespace discrete

class UpdateReport : public interfaces::TriggerAction
{
  public:
    UpdateReport(boost::asio::io_context& ioc,
                 std::shared_ptr<std::vector<std::string>> ids) :
        ioc(ioc),
        reportIds(std::move(ids))
    {}

    void commit(const std::string& id, Milliseconds timestamp,
                double value) override;

  private:
    boost::asio::io_context& ioc;
    std::shared_ptr<std::vector<std::string>> reportIds;
};
} // namespace action
