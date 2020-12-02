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

struct SensorPaths
{
    static std::string str()
    {
        return "sensorPaths";
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

} // namespace tstring
} // namespace utils
