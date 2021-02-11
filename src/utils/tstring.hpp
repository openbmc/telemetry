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

struct CollectionTimeScope
{
    static std::string str()
    {
        return "collectionTimeScope";
    }
};

struct CollectionDuration
{
    static std::string str()
    {
        return "collectionDuration";
    }
};

} // namespace tstring
} // namespace utils
