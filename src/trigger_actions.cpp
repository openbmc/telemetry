#include "trigger_actions.hpp"

#include <phosphor-logging/log.hpp>

#include <ctime>

namespace action
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
        case numeric::Type::upperCritical:
            return "UpperCritical";
        case numeric::Type::lowerCritical:
            return "LowerCritical";
        case numeric::Type::upperWarning:
            return "UpperWarning";
        case numeric::Type::lowerWarning:
            return "LowerWarning";
    }
    throw std::runtime_error("Invalid type");
}

void LogToJournal::commit(const std::string& sensorName, uint64_t timestamp,
                          double value)
{
    std::time_t t = static_cast<time_t>(timestamp);
    std::array<char, sizeof("YYYY-MM-DDThh:mm:ssZ")> buf = {};
    size_t size =
        std::strftime(buf.data(), buf.size(), "%FT%TZ", std::gmtime(&t));
    if (size == 0)
    {
        throw std::runtime_error("Failed to parse timestamp to string");
    }

    std::string msg = std::string(getType()) +
                      " numeric threshold condition is met on sensor " +
                      sensorName + ", recorded value " + std::to_string(value) +
                      ", timestamp " + std::string(buf.data(), size) +
                      ", direction " +
                      std::string(getDirection(value, threshold));

    phosphor::logging::log<phosphor::logging::level::INFO>(msg.c_str());
}

const char* LogToRedfish::getMessageId() const
{
    switch (type)
    {
        case numeric::Type::upperCritical:
            return "OpenBMC.0.1.0.NumericThresholdUpperCritical";
        case numeric::Type::lowerCritical:
            return "OpenBMC.0.1.0.NumericThresholdLowerCritical";
        case numeric::Type::upperWarning:
            return "OpenBMC.0.1.0.NumericThresholdUpperWarning";
        case numeric::Type::lowerWarning:
            return "OpenBMC.0.1.0.NumericThresholdLowerWarning";
    }
    throw std::runtime_error("Invalid type");
}

void LogToRedfish::commit(const std::string& sensorName, uint64_t timestamp,
                          double value)
{
    phosphor::logging::log<phosphor::logging::level::INFO>(
        "Threshold value is exceeded",
        phosphor::logging::entry("REDFISH_MESSAGE_ID=%s", getMessageId()),
        phosphor::logging::entry("REDFISH_MESSAGE_ARGS=%s,%f,%llu,%s",
                                 sensorName.c_str(), value, timestamp,
                                 getDirection(value, threshold)));
}

void UpdateReport::commit(const std::string&, uint64_t, double)
{
    for (const auto& name : reportNames)
    {
        reportManager.updateReport(name);
    }
}
} // namespace action
