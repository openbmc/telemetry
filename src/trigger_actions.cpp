#include "trigger_actions.hpp"

#include "messages/update_report_ind.hpp"
#include "types/trigger_types.hpp"
#include "utils/clock.hpp"
#include "utils/messanger.hpp"
#include "utils/to_short_enum.hpp"

#include <phosphor-logging/log.hpp>

#include <ctime>
#include <iomanip>
#include <sstream>

namespace action
{

namespace
{
std::string timestampToString(Milliseconds timestamp)
{
    std::time_t t = static_cast<time_t>(
        std::chrono::duration_cast<std::chrono::seconds>(timestamp).count());
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&t), "%FT%T.") << std::setw(3)
       << std::setfill('0') << timestamp.count() % 1000 << 'Z';
    return ss.str();
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
                          const ThresholdName thresholdNameInIn,
                          const std::string& sensorName,
                          const Milliseconds timestamp,
                          const TriggerValue triggerValue)
{
    double value = std::get<double>(triggerValue);
    std::string thresholdName = ::numeric::typeToString(type);
    auto direction = getDirection(value, threshold);

    std::string msg =
        "Numeric threshold '" + std::string(utils::toShortEnum(thresholdName)) +
        "' of trigger '" + triggerId + "' is crossed on sensor " + sensorName +
        ", recorded value: " + std::to_string(value) +
        ", crossing direction: " + std::string(utils::toShortEnum(direction)) +
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
                                  const ThresholdName thresholdNameInIn,
                                  const std::string& sensorName,
                                  const Milliseconds timestamp,
                                  const TriggerValue triggerValue)
{
    double value = std::get<double>(triggerValue);
    std::string thresholdName = ::numeric::typeToString(type);
    auto direction = getDirection(value, threshold);
    auto timestampStr = timestampToString(timestamp);

    phosphor::logging::log<phosphor::logging::level::INFO>(
        "Logging numeric trigger action to Redfish Event Log.",
        phosphor::logging::entry("REDFISH_MESSAGE_ID=%s",
                                 getRedfishMessageId()),
        phosphor::logging::entry("REDFISH_MESSAGE_ARGS=%s,%s,%s,%f,%s,%s",
                                 thresholdName.c_str(), triggerId.c_str(),
                                 sensorName.c_str(), value, direction,
                                 timestampStr.c_str()));
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
                          const ThresholdName thresholdNameIn,
                          const std::string& sensorName,
                          const Milliseconds timestamp,
                          const TriggerValue triggerValue)
{
    auto value = std::get<std::string>(triggerValue);

    std::string msg =
        "Discrete condition '" + thresholdNameIn->get() + "' of trigger '" +
        triggerId + "' is met on sensor " + sensorName +
        ", recorded value: " + value + ", severity: " +
        std::string(utils::toShortEnum(utils::enumToString(severity))) +
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
                                  const ThresholdName thresholdNameIn,
                                  const std::string& sensorName,
                                  const Milliseconds timestamp,
                                  const TriggerValue triggerValue)
{
    auto value = std::get<std::string>(triggerValue);
    auto timestampStr = timestampToString(timestamp);

    phosphor::logging::log<phosphor::logging::level::INFO>(
        "Logging discrete trigger action to Redfish Event Log.",
        phosphor::logging::entry("REDFISH_MESSAGE_ID=%s",
                                 getRedfishMessageId()),
        phosphor::logging::entry("REDFISH_MESSAGE_ARGS=%s,%s,%s,%s,%s",
                                 thresholdNameIn->get().c_str(),
                                 triggerId.c_str(), sensorName.c_str(),
                                 value.c_str(), timestampStr.c_str()));
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
                          const ThresholdName thresholdNameIn,
                          const std::string& sensorName,
                          const Milliseconds timestamp,
                          const TriggerValue triggerValue)
{
    auto value = triggerValueToString(triggerValue);
    std::string msg = "Discrete condition 'OnChange' of trigger '" + triggerId +
                      "' is met on sensor: " + sensorName +
                      ", recorded value: " + value +
                      ", timestamp: " + timestampToString(timestamp);

    phosphor::logging::log<phosphor::logging::level::INFO>(msg.c_str());
}

void LogToRedfishEventLog::commit(const std::string& triggerId,
                                  const ThresholdName thresholdNameIn,
                                  const std::string& sensorName,
                                  const Milliseconds timestamp,
                                  const TriggerValue triggerValue)
{
    auto value = triggerValueToString(triggerValue);
    auto timestampStr = timestampToString(timestamp);

    phosphor::logging::log<phosphor::logging::level::INFO>(
        "Logging onChange discrete trigger action to Redfish Event Log.",
        phosphor::logging::entry("REDFISH_MESSAGE_ID=%s",
                                 redfish_message_ids::TriggerDiscreteOK),
        phosphor::logging::entry("REDFISH_MESSAGE_ARGS=%s,%s,%s,%s,%s",
                                 "OnChange", triggerId.c_str(),
                                 sensorName.c_str(), value.c_str(),
                                 timestampStr.c_str()));
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
                          const ThresholdName thresholdNameIn,
                          const std::string& sensorName,
                          const Milliseconds timestamp,
                          const TriggerValue triggerValue)
{
    if (reportIds->empty())
    {
        return;
    }

    utils::Messanger messanger(ioc);
    messanger.send(messages::UpdateReportInd{*reportIds});
}
} // namespace action
