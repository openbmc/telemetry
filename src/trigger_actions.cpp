#include "trigger_actions.hpp"

#include <phosphor-logging/log.hpp>

#include <ctime>

const char* LogToJournalAction::getLevel() const
{
    if (type == numeric::Type::upperCritical)
    {
        return "UpperCritical";
    }
    if (type == numeric::Type::lowerCritical)
    {
        return "LowerCritical";
    }
    if (type == numeric::Type::upperWarning)
    {
        return "UpperWarning";
    }
    if (type == numeric::Type::lowerWarning)
    {
        return "LowerWarning";
    }
    throw std::runtime_error("Invalid type");
}

void LogToJournalAction::commit(const std::string& sensorName,
                                uint64_t timestamp, double value)
{
    std::time_t t = static_cast<time_t>(timestamp);
    std::array<char, sizeof("YYYY-MM-DDThh:mm:ssZ")> buf = {};
    size_t size =
        std::strftime(buf.data(), buf.size(), "%FT%TZ", std::gmtime(&t));
    if (size == 0)
    {
        throw std::runtime_error("Failed to parse timestamp to string");
    }

    std::string msg = std::string(getLevel()) +
                      " numeric threshold condition is met on sensor " +
                      sensorName + ", recorded value " + std::to_string(value) +
                      ", timestamp " + std::string(buf.data(), size);

    phosphor::logging::log<phosphor::logging::level::INFO>(msg.c_str());
}

const char* LogToRedfishAction::getMessageId() const
{
    if (type == numeric::Type::upperCritical)
    {
        return "OpenBMC.0.1.0.NumericThresholdUpperCritical";
    }
    if (type == numeric::Type::lowerCritical)
    {
        return "OpenBMC.0.1.0.NumericThresholdLowerCritical";
    }
    if (type == numeric::Type::upperWarning)
    {
        return "OpenBMC.0.1.0.NumericThresholdUpperWarning";
    }
    if (type == numeric::Type::lowerWarning)
    {
        return "OpenBMC.0.1.0.NumericThresholdLowerWarning";
    }
    throw std::runtime_error("Invalid type");
}

void LogToRedfishAction::commit(const std::string& sensorName,
                                uint64_t timestamp, double value)
{
    phosphor::logging::log<phosphor::logging::level::INFO>(
        "Threshold value is exceeded",
        phosphor::logging::entry("REDFISH_MESSAGE_ID=%s", getMessageId()),
        phosphor::logging::entry("REDFISH_MESSAGE_ARGS=%s,%f,%llu",
                                 sensorName.c_str(), value, timestamp));
}

void UpdateReportAction::commit(const std::string&, uint64_t, double)
{
    for (const auto& name : reportNames)
    {
        reportManager.updateReport(name);
    }
}
