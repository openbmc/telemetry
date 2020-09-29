#pragma once

#include "interfaces/sensor.hpp"

#include <gmock/gmock.h>

inline auto sensorIdEq(interfaces::Sensor::Id id)
{
    using namespace testing;

    return AllOf(Field(&interfaces::Sensor::Id::type, Eq(id.type)),
                 Field(&interfaces::Sensor::Id::service, Eq(id.service)),
                 Field(&interfaces::Sensor::Id::path, Eq(id.path)));
}
