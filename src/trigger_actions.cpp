#include "trigger_actions.hpp"

#include <phosphor-logging/log.hpp>

#include <ctime>

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

const char* LogToJournal::getType() const
{
    switch (type)
    {
        case ::numeric::Type::upperCritical:
            return "UpperCritical";
        case ::numeric::Type::lowerCritical:
            return "LowerCritical";
        case ::numeric::Type::upperWarning:
            return "UpperWarning";
        case ::numeric::Type::lowerWarning:
            return "LowerWarning";
    }
    throw std::runtime_error("Invalid type");
}

void LogToJournal::commit(const std::string& sensorName, Milliseconds timestamp,
                          double value)
{
    std::string msg = std::string(getType()) +
                      " numeric threshold condition is met on sensor " +
                      sensorName + ", recorded value " + std::to_string(value) +
                      ", timestamp " + timestampToString(timestamp) +
                      ", direction " +
                      std::string(getDirection(value, threshold));

    phosphor::logging::log<phosphor::logging::level::INFO>(msg.c_str());
}

const char* LogToRedfish::getMessageId() const
{
    switch (type)
    {
        case ::numeric::Type::upperCritical:
            return "OpenBMC.0.1.0.NumericThresholdUpperCritical";
        case ::numeric::Type::lowerCritical:
            return "OpenBMC.0.1.0.NumericThresholdLowerCritical";
        case ::numeric::Type::upperWarning:
            return "OpenBMC.0.1.0.NumericThresholdUpperWarning";
        case ::numeric::Type::lowerWarning:
            return "OpenBMC.0.1.0.NumericThresholdLowerWarning";
    }
    throw std::runtime_error("Invalid type");
}

void LogToRedfish::commit(const std::string& sensorName, Milliseconds timestamp,
                          double value)
{
    phosphor::logging::log<phosphor::logging::level::INFO>(
        "Threshold value is exceeded",
        phosphor::logging::entry("REDFISH_MESSAGE_ID=%s", getMessageId()),
        phosphor::logging::entry("REDFISH_MESSAGE_ARGS=%s,%f,%llu,%s",
                                 sensorName.c_str(), value, timestamp.count(),
                                 getDirection(value, threshold)));
}

void fillActions(
    std::vector<std::unique_ptr<interfaces::TriggerAction>>& actionsIf,
    const std::vector<TriggerAction>& ActionsEnum, ::numeric::Type type,
    double thresholdValue, interfaces::ReportManager& reportManager,
    const std::vector<std::string>& reportIds)
{
    actionsIf.reserve(ActionsEnum.size());
    for (auto actionType : ActionsEnum)
    {
        switch (actionType)
        {
            case TriggerAction::LogToLogService:
            {
                actionsIf.emplace_back(
                    std::make_unique<LogToJournal>(type, thresholdValue));
                break;
            }
            case TriggerAction::RedfishEvent:
            {
                actionsIf.emplace_back(
                    std::make_unique<LogToRedfish>(type, thresholdValue));
                break;
            }
            case TriggerAction::UpdateReport:
            {
                actionsIf.emplace_back(
                    std::make_unique<UpdateReport>(reportManager, reportIds));
                break;
            }
        }
    }
}

} // namespace numeric

namespace discrete
{
const char* LogToJournal::getSeverity() const
{
    switch (severity)
    {
        case ::discrete::Severity::ok:
            return "OK";
        case ::discrete::Severity::warning:
            return "Warning";
        case ::discrete::Severity::critical:
            return "Critical";
    }
    throw std::runtime_error("Invalid severity");
}

void LogToJournal::commit(const std::string& sensorName, Milliseconds timestamp,
                          double value)
{
    std::string msg = std::string(getSeverity()) +
                      " discrete threshold condition is met on sensor " +
                      sensorName + ", recorded value " + std::to_string(value) +
                      ", timestamp " + timestampToString(timestamp);

    phosphor::logging::log<phosphor::logging::level::INFO>(msg.c_str());
}

const char* LogToRedfish::getMessageId() const
{
    switch (severity)
    {
        case ::discrete::Severity::ok:
            return "OpenBMC.0.1.0.DiscreteThresholdOk";
        case ::discrete::Severity::warning:
            return "OpenBMC.0.1.0.DiscreteThresholdWarning";
        case ::discrete::Severity::critical:
            return "OpenBMC.0.1.0.DiscreteThresholdCritical";
    }
    throw std::runtime_error("Invalid severity");
}

void LogToRedfish::commit(const std::string& sensorName, Milliseconds timestamp,
                          double value)
{
    phosphor::logging::log<phosphor::logging::level::INFO>(
        "Discrete treshold condition is met",
        phosphor::logging::entry("REDFISH_MESSAGE_ID=%s", getMessageId()),
        phosphor::logging::entry("REDFISH_MESSAGE_ARGS=%s,%f,%llu",
                                 sensorName.c_str(), value, timestamp.count()));
}

void fillActions(
    std::vector<std::unique_ptr<interfaces::TriggerAction>>& actionsIf,
    const std::vector<TriggerAction>& ActionsEnum,
    ::discrete::Severity severity, interfaces::ReportManager& reportManager,
    const std::vector<std::string>& reportIds)
{
    actionsIf.reserve(ActionsEnum.size());
    for (auto actionType : ActionsEnum)
    {
        switch (actionType)
        {
            case TriggerAction::LogToLogService:
            {
                actionsIf.emplace_back(
                    std::make_unique<LogToJournal>(severity));
                break;
            }
            case TriggerAction::RedfishEvent:
            {
                actionsIf.emplace_back(
                    std::make_unique<LogToRedfish>(severity));
                break;
            }
            case TriggerAction::UpdateReport:
            {
                actionsIf.emplace_back(
                    std::make_unique<UpdateReport>(reportManager, reportIds));
                break;
            }
        }
    }
}

namespace onChange
{
void LogToJournal::commit(const std::string& sensorName, Milliseconds timestamp,
                          double value)
{
    std::string msg = "Value changed on sensor " + sensorName +
                      ", recorded value " + std::to_string(value) +
                      ", timestamp " + timestampToString(timestamp);

    phosphor::logging::log<phosphor::logging::level::INFO>(msg.c_str());
}

void LogToRedfish::commit(const std::string& sensorName, Milliseconds timestamp,
                          double value)
{
    const char* messageId = "OpenBMC.0.1.0.DiscreteThresholdOnChange";
    phosphor::logging::log<phosphor::logging::level::INFO>(
        "Uncondtional discrete threshold triggered",
        phosphor::logging::entry("REDFISH_MESSAGE_ID=%s", messageId),
        phosphor::logging::entry("REDFISH_MESSAGE_ARGS=%s,%f,%llu",
                                 sensorName.c_str(), value, timestamp.count()));
}

void fillActions(
    std::vector<std::unique_ptr<interfaces::TriggerAction>>& actionsIf,
    const std::vector<TriggerAction>& ActionsEnum,
    interfaces::ReportManager& reportManager,
    const std::vector<std::string>& reportIds)
{
    actionsIf.reserve(ActionsEnum.size());
    for (auto actionType : ActionsEnum)
    {
        switch (actionType)
        {
            case TriggerAction::LogToLogService:
            {
                actionsIf.emplace_back(std::make_unique<LogToJournal>());
                break;
            }
            case TriggerAction::RedfishEvent:
            {
                actionsIf.emplace_back(std::make_unique<LogToRedfish>());
                break;
            }
            case TriggerAction::UpdateReport:
            {
                actionsIf.emplace_back(
                    std::make_unique<UpdateReport>(reportManager, reportIds));
                break;
            }
        }
    }
}
} // namespace onChange
} // namespace discrete

void UpdateReport::commit(const std::string&, Milliseconds, double)
{
    for (const auto& name : reportIds)
    {
        reportManager.updateReport(name);
    }
}
} // namespace action
