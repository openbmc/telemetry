#include "trigger_actions.hpp"

#include "messages/update_report_ind.hpp"
#include "types/trigger_types.hpp"
#include "utils/messanger.hpp"

#include <phosphor-logging/log.hpp>

#include <ctime>
#include <format>

namespace action
{

namespace
{
std::string timestampToString(Milliseconds timestamp)
{
    std::time_t t = static_cast<time_t>(timestamp.count());
    std::array<char, sizeof("YYYY-MM-DDThh:mm:ssZ")> buf = {};
    size_t size =
        std::strftime(buf.data(), buf.size(), "%FT%TZ", std::gmtime(&t));
    if (size == 0)
    {
        throw std::runtime_error("Failed to parse timestamp to string");
    }
    return std::string(buf.data(), size);
}
} // namespace

namespace numeric
{

static const char* getDirection(double value, double threshold)
{
    if (value < threshold)
    {
        return "decreasing";
    }
    if (value > threshold)
    {
        return "increasing";
    }
    throw std::runtime_error("Invalid value");
}

void LogToJournal::commit(const std::string& triggerId,
                          ThresholdName thresholdNameInIn,
                          const std::string& sensorName, Milliseconds timestamp,
                          TriggerValue triggerValue)
{
    double value = std::get<double>(triggerValue);
    std::string thresholdName = ::numeric::typeToString(type);
    auto direction = getDirection(value, threshold);

    std::string msg = "Numeric threshold '" + thresholdName + "' of trigger '" +
                      triggerId + "' is crossed on sensor " + sensorName +
                      ", recorded value: " + std::to_string(value) +
                      ", crossing direction: " + direction +
                      ", timestamp: " + timestampToString(timestamp);

    phosphor::logging::log<phosphor::logging::level::INFO>(msg.c_str());
}

const char* LogToRedfishEventLog::getRedfishMessageId() const
{
    switch (type)
    {
        case ::numeric::Type::upperCritical:
            return redfish_message_ids::TriggerNumericCritical;
        case ::numeric::Type::lowerCritical:
            return redfish_message_ids::TriggerNumericCritical;
        case ::numeric::Type::upperWarning:
            return redfish_message_ids::TriggerNumericWarning;
        case ::numeric::Type::lowerWarning:
            return redfish_message_ids::TriggerNumericWarning;
    }
    throw std::runtime_error("Invalid type");
}

void LogToRedfishEventLog::commit(const std::string& triggerId,
                                  ThresholdName thresholdNameInIn,
                                  const std::string& sensorName,
                                  Milliseconds timestamp,
                                  TriggerValue triggerValue)
{
    double value = std::get<double>(triggerValue);
    std::string thresholdName = ::numeric::typeToString(type);
    auto direction = getDirection(value, threshold);

    phosphor::logging::log<phosphor::logging::level::INFO>(
        "Logging numeric trigger action to Redfish Event Log.",
        phosphor::logging::entry("REDFISH_MESSAGE_ID=%s",
                                 getRedfishMessageId()),
        phosphor::logging::entry("REDFISH_MESSAGE_ARGS=%s,%s,%s,%f,%s,%llu",
                                 triggerId.c_str(), thresholdName.c_str(),
                                 sensorName.c_str(), value, direction,
                                 timestamp.count()));
}

void fillActions(
    std::vector<std::unique_ptr<interfaces::TriggerAction>>& actionsIf,
    const std::vector<TriggerAction>& ActionsEnum, ::numeric::Type type,
    double thresholdValue, boost::asio::io_context& ioc,
    const std::shared_ptr<std::vector<std::string>>& reportIds)
{
    actionsIf.reserve(ActionsEnum.size());
    for (auto actionType : ActionsEnum)
    {
        switch (actionType)
        {
            case TriggerAction::LogToJournal:
            {
                actionsIf.emplace_back(
                    std::make_unique<LogToJournal>(type, thresholdValue));
                break;
            }
            case TriggerAction::LogToRedfishEventLog:
            {
                actionsIf.emplace_back(std::make_unique<LogToRedfishEventLog>(
                    type, thresholdValue));
                break;
            }
            case TriggerAction::UpdateReport:
            {
                actionsIf.emplace_back(
                    std::make_unique<UpdateReport>(ioc, reportIds));
                break;
            }
        }
    }
}

} // namespace numeric

namespace discrete
{

void LogToJournal::commit(const std::string& triggerId,
                          ThresholdName thresholdNameIn,
                          const std::string& sensorName, Milliseconds timestamp,
                          TriggerValue triggerValue)
{
    auto value = std::get<std::string>(triggerValue);

    std::string msg = "Discrete condition '" + thresholdNameIn->get() +
                      "' of trigger '" + triggerId + "' is met on sensor " +
                      sensorName + ", recorded value: " + value +
                      ", severity: " + ::discrete::severityToString(severity) +
                      ", timestamp: " + timestampToString(timestamp);

    phosphor::logging::log<phosphor::logging::level::INFO>(msg.c_str());
}

const char* LogToRedfishEventLog::getRedfishMessageId() const
{
    switch (severity)
    {
        case ::discrete::Severity::ok:
            return redfish_message_ids::TriggerDiscreteOK;
        case ::discrete::Severity::warning:
            return redfish_message_ids::TriggerDiscreteWarning;
        case ::discrete::Severity::critical:
            return redfish_message_ids::TriggerDiscreteCritical;
    }
    throw std::runtime_error("Invalid severity");
}

void LogToRedfishEventLog::commit(const std::string& triggerId,
                                  ThresholdName thresholdNameIn,
                                  const std::string& sensorName,
                                  Milliseconds timestamp,
                                  TriggerValue triggerValue)
{
    auto value = std::get<std::string>(triggerValue);

    phosphor::logging::log<phosphor::logging::level::INFO>(
        "Logging discrete trigger action to Redfish Event Log.",
        phosphor::logging::entry("REDFISH_MESSAGE_ID=%s",
                                 getRedfishMessageId()),
        phosphor::logging::entry(
            "REDFISH_MESSAGE_ARGS=%s,%s,%s,%s,%llu", triggerId.c_str(),
            thresholdNameIn->get().c_str(), sensorName.c_str(), value.c_str(),
            timestamp.count()));
}

void fillActions(
    std::vector<std::unique_ptr<interfaces::TriggerAction>>& actionsIf,
    const std::vector<TriggerAction>& ActionsEnum,
    ::discrete::Severity severity, boost::asio::io_context& ioc,
    const std::shared_ptr<std::vector<std::string>>& reportIds)
{
    actionsIf.reserve(ActionsEnum.size());
    for (auto actionType : ActionsEnum)
    {
        switch (actionType)
        {
            case TriggerAction::LogToJournal:
            {
                actionsIf.emplace_back(
                    std::make_unique<LogToJournal>(severity));
                break;
            }
            case TriggerAction::LogToRedfishEventLog:
            {
                actionsIf.emplace_back(
                    std::make_unique<LogToRedfishEventLog>(severity));
                break;
            }
            case TriggerAction::UpdateReport:
            {
                actionsIf.emplace_back(
                    std::make_unique<UpdateReport>(ioc, reportIds));
                break;
            }
        }
    }
}

namespace onChange
{
void LogToJournal::commit(const std::string& triggerId,
                          ThresholdName thresholdNameIn,
                          const std::string& sensorName, Milliseconds timestamp,
                          TriggerValue triggerValue)
{
    auto value = triggerValueToString(triggerValue);
    std::string msg = "Discrete condition 'OnChange' of trigger '" + triggerId +
                      " is met on sensor: " + sensorName + ", recorded value " +
                      value + ", timestamp " + timestampToString(timestamp);

    phosphor::logging::log<phosphor::logging::level::INFO>(msg.c_str());
}

void LogToRedfishEventLog::commit(const std::string& triggerId,
                                  ThresholdName thresholdNameIn,
                                  const std::string& sensorName,
                                  Milliseconds timestamp,
                                  TriggerValue triggerValue)
{
    auto value = triggerValueToString(triggerValue);

    phosphor::logging::log<phosphor::logging::level::INFO>(
        "Logging onChange discrete trigger action to Redfish Event Log.",
        phosphor::logging::entry("REDFISH_MESSAGE_ID=%s",
                                 redfish_message_ids::TriggerDiscreteOK),
        phosphor::logging::entry(
            "REDFISH_MESSAGE_ARGS=%s,%s,%s,%s,%llu", triggerId.c_str(),
            "OnChange", sensorName.c_str(), value.c_str(), timestamp.count()));
}

void fillActions(
    std::vector<std::unique_ptr<interfaces::TriggerAction>>& actionsIf,
    const std::vector<TriggerAction>& ActionsEnum, boost::asio::io_context& ioc,
    const std::shared_ptr<std::vector<std::string>>& reportIds)
{
    actionsIf.reserve(ActionsEnum.size());
    for (auto actionType : ActionsEnum)
    {
        switch (actionType)
        {
            case TriggerAction::LogToJournal:
            {
                actionsIf.emplace_back(std::make_unique<LogToJournal>());
                break;
            }
            case TriggerAction::LogToRedfishEventLog:
            {
                actionsIf.emplace_back(
                    std::make_unique<LogToRedfishEventLog>());
                break;
            }
            case TriggerAction::UpdateReport:
            {
                actionsIf.emplace_back(
                    std::make_unique<UpdateReport>(ioc, reportIds));
                break;
            }
        }
    }
}
} // namespace onChange
} // namespace discrete

void UpdateReport::commit(const std::string& triggerId,
                          ThresholdName thresholdNameIn,
                          const std::string& sensorName, Milliseconds timestamp,
                          TriggerValue triggerValue)
{
    if (reportIds->empty())
    {
        return;
    }

    utils::Messanger messanger(ioc);
    messanger.send(messages::UpdateReportInd{*reportIds});
}
} // namespace action
