#pragma once

#include "interfaces/sensor_listener.hpp"

#include <gmock/gmock.h>

class SensorListenerMock : public interfaces::SensorListener
{
  public:
    MOCK_METHOD(void, sensorUpdated,
                (interfaces::Sensor&, Milliseconds, double), (override));
};
