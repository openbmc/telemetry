#pragma once

#include "helpers/interfaces/labeled_sensor_parameter_helpers.hpp"
#include "interfaces/types.hpp"

#include <ostream>

#include <gmock/gmock.h>

namespace utils
{

inline void PrintTo(const LabeledMetricParameters& o, std::ostream* os)
{
    using testing::PrintToString;

    (*os) << "{ ";
    (*os) << utils::tstring::SensorPaths::str() << ": "
          << PrintToString(o.at_label<utils::tstring::SensorPaths>()) << ", ";
    (*os) << utils::tstring::OperationType::str() << ": "
          << PrintToString(o.at_label<utils::tstring::OperationType>()) << ", ";
    (*os) << utils::tstring::Id::str() << ": "
          << PrintToString(o.at_label<utils::tstring::Id>()) << ", ";
    (*os) << utils::tstring::MetricMetadata::str() << ": "
          << PrintToString(o.at_label<utils::tstring::MetricMetadata>());
    (*os) << " }";
}

} // namespace utils