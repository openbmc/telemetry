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

struct Metadata
{
    static std::string str()
    {
        return "metadata";
    }
};

struct OperationType
{
    static std::string str()
    {
        return "operationType";
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

struct MetricProperties
{
    static std::string str()
    {
        return "MetricProperties";
    }
};

struct SensorDbusPath
{
    static std::string str()
    {
        return "SensorDbusPath";
    }
};

struct SensorRedfishUri
{
    static std::string str()
    {
        return "SensorRedfishUri";
    }
};

} // namespace tstring
} // namespace utils
