#pragma once

#include <array>
#include <string>

namespace utils
{
namespace tstring
{

struct Id
{
    static std::string str()
    {
        return "id";
    }
};

struct SensorPath
{
    static std::string str()
    {
        return "sensorPath";
    }
};

struct SensorMetadata
{
    static std::string str()
    {
        return "sensorMetadata";
    }
};

struct OperationType
{
    static std::string str()
    {
        return "operationType";
    }
};

struct MetricMetadata
{
    static std::string str()
    {
        return "metricMetadata";
    }
};

struct Service
{
    static std::string str()
    {
        return "service";
    }
};

struct Path
{
    static std::string str()
    {
        return "path";
    }
};

struct Type
{
    static std::string str()
    {
        return "type";
    }
};

struct DwellTime
{
    static std::string str()
    {
        return "dwellTime";
    }
};

struct Direction
{
    static std::string str()
    {
        return "direction";
    }
};

struct ThresholdValue
{
    static std::string str()
    {
        return "thresholdValue";
    }
};

struct UserId
{
    static std::string str()
    {
        return "userId";
    }
};

struct Severity
{
    static std::string str()
    {
        return "severity";
    }
};

} // namespace tstring
} // namespace utils
