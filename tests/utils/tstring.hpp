#pragma once

#include <array>
#include <string>

namespace utils
{

enum class TStringValue
{
    id,
    sensorPaths,
    operationType,
    metricMetadata
};

template <TStringValue V>
struct TString
{
    static std::string str()
    {
        switch (V)
        {
            case TStringValue::id:
                return "id";
            case TStringValue::sensorPaths:
                return "sensorPaths";
            case TStringValue::operationType:
                return "operationType";
            case TStringValue::metricMetadata:
                return "metricMetadata";
        }
    }
};

namespace tstring
{
using Id = utils::TString<TStringValue::id>;
using SensorPaths = utils::TString<TStringValue::sensorPaths>;
using OperationType = utils::TString<TStringValue::operationType>;
using MetricMetadata = utils::TString<TStringValue::metricMetadata>;
} // namespace tstring
} // namespace utils
