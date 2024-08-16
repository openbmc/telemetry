#pragma once

#include "interfaces/trigger_action.hpp"
#include "types/trigger_types.hpp"

#include <boost/asio/io_context.hpp>

namespace action
{

namespace redfish_message_ids
{
constexpr const char* TriggerDiscreteConditionMet =
    "Telemetry.1.0.TriggerDiscreteConditionMet";
constexpr const char* TriggerNumericAboveLowerCritical =
    "Telemetry.1.0.TriggerNumericAboveLowerCritical";
constexpr const char* TriggerNumericAboveUpperCritical =
    "Telemetry.1.0.TriggerNumericAboveUpperCritical";
constexpr const char* TriggerNumericAboveUpperWarning =
    "Telemetry.1.0.TriggerNumericAboveUpperWarning";
constexpr const char* TriggerNumericBelowLowerCritical =
    "Telemetry.1.0.TriggerNumericBelowLowerCritical";
constexpr const char* TriggerNumericBelowLowerWarning =
    "Telemetry.1.0.TriggerNumericBelowLowerWarning";
constexpr const char* TriggerNumericBelowUpperCritical =
    "Telemetry.1.0.TriggerNumericBelowUpperCritical";
constexpr const char* TriggerNumericReadingNormal =
    "Telemetry.1.0.TriggerNumericReadingNormal";
} // namespace redfish_message_ids

namespace numeric
{
class LogToJournal : public interfaces::TriggerAction
{
  public:
    LogToJournal(::numeric::Type type, double val) : type(type), threshold(val)
    {}

    void commit(const std::string& triggerId, const ThresholdName thresholdName,
                const std::string& sensorName, const Milliseconds timestamp,
                const TriggerValue value) override;

  private:
    const ::numeric::Type type;
    const double threshold;
};

class LogToRedfishEventLog : public interfaces::TriggerAction
{
  public:
    LogToRedfishEventLog(::numeric::Type type, double val) :
        type(type), threshold(val)
    {}

    void commit(const std::string& triggerId, const ThresholdName thresholdName,
                const std::string& sensorName, const Milliseconds timestamp,
                const TriggerValue value) override;

  private:
    const ::numeric::Type type;
    const double threshold;

    const char* getRedfishMessageId(const double value) const;
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
    explicit LogToJournal(::discrete::Severity severity) : severity(severity) {}

    void commit(const std::string& triggerId, const ThresholdName thresholdName,
                const std::string& sensorName, const Milliseconds timestamp,
                const TriggerValue value) override;

  private:
    const ::discrete::Severity severity;
};

class LogToRedfishEventLog : public interfaces::TriggerAction
{
  public:
    explicit LogToRedfishEventLog() {}

    void commit(const std::string& triggerId, const ThresholdName thresholdName,
                const std::string& sensorName, const Milliseconds timestamp,
                const TriggerValue value) override;
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
    LogToJournal() {}

    void commit(const std::string& triggerId, const ThresholdName thresholdName,
                const std::string& sensorName, const Milliseconds timestamp,
                const TriggerValue value) override;
};

class LogToRedfishEventLog : public interfaces::TriggerAction
{
  public:
    LogToRedfishEventLog() {}

    void commit(const std::string& triggerId, const ThresholdName thresholdName,
                const std::string& sensorName, const Milliseconds timestamp,
                const TriggerValue value) override;
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
        ioc(ioc), reportIds(std::move(ids))
    {}

    void commit(const std::string& triggerId, const ThresholdName thresholdName,
                const std::string& sensorName, const Milliseconds timestamp,
                const TriggerValue value) override;

  private:
    boost::asio::io_context& ioc;
    std::shared_ptr<std::vector<std::string>> reportIds;
};
} // namespace action
