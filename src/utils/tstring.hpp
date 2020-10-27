#pragma once

#include <array>
#include <string>

namespace utils
{
namespace literals
{

constexpr char id[] = "id";
constexpr char sensorPaths[] = "sensorPaths";
constexpr char operationType[] = "operationType";
constexpr char metricMetadata[] = "metricMetadata";

} // namespace literals

template <const char* const V>
struct Label
{
    static std::string str()
    {
        return V;
    }
};

namespace tstring
{

using Id = utils::Label<utils::literals::id>;
using SensorPaths = utils::Label<utils::literals::sensorPaths>;
using OperationType = utils::Label<utils::literals::operationType>;
using MetricMetadata = utils::Label<utils::literals::metricMetadata>;

} // namespace tstring
} // namespace utils
