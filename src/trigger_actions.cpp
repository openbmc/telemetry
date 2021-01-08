#include "trigger_actions.hpp"

#include <phosphor-logging/log.hpp>

#include <ctime>

const char* LogToJournalAction::getDirection() const
{
    if (direction == numeric::Direction::increasing)
    {
        return "increasing";
    }
    if (direction == numeric::Direction::decreasing)
    {
        return "decreasing";
    }
    if (direction == numeric::Direction::either)
    {
        return "either";
    }
    throw std::runtime_error("Invalid direction");
}

const char* LogToJournalAction::getLevel() const
{
    if (level == numeric::Level::upperCritical)
    {
        return "UpperCritical";
    }
    if (level == numeric::Level::lowerCritical)
    {
        return "LowerCritical";
    }
    if (level == numeric::Level::upperWarning)
    {
        return "UpperWarning";
    }
    if (level == numeric::Level::lowerWarning)
    {
        return "LowerWarning";
    }
    throw std::runtime_error("Invalid level");
}

void LogToJournalAction::commit(const interfaces::Sensor::Id& id,
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

    std::string msg = "Threshold is crossed in direction " +
                      std::string(getDirection()) + " when value reached " +
                      std::to_string(value) + " at " +
                      std::string(buf.data(), size);

    phosphor::logging::log<phosphor::logging::level::INFO>(msg.c_str());
}

const char* LogToRedfishAction::getMessageId() const
{
    if (level == numeric::Level::upperCritical)
    {
        return "OpenBMC.0.1.0.NumericThresholdUpperCritical";
    }
    if (level == numeric::Level::lowerCritical)
    {
        return "OpenBMC.0.1.0.NumericThresholdLowerCritical";
    }
    if (level == numeric::Level::upperWarning)
    {
        return "OpenBMC.0.1.0.NumericThresholdUpperWarning";
    }
    if (level == numeric::Level::lowerWarning)
    {
        return "OpenBMC.0.1.0.NumericThresholdLowerWarning";
    }
    throw std::runtime_error("Invalid level");
}

void LogToRedfishAction::commit(const interfaces::Sensor::Id& id,
                                uint64_t timestamp, double value)
{
    phosphor::logging::log<phosphor::logging::level::INFO>(
        "Threshold value is exceeded",
        phosphor::logging::entry("REDFISH_MESSAGE_ID=%s", getMessageId()),
        phosphor::logging::entry("REDFISH_MESSAGE_ARGS=%s,%f,%f,%llu",
                                 id.path.c_str(), value, thresholdValue,
                                 timestamp));
}

void UpdateReportAction::commit(const interfaces::Sensor::Id& id,
                                uint64_t timestamp, double value)
{
    for (const auto& name : reportNames)
    {
        reportManager.updateReport(name);
    }
}
