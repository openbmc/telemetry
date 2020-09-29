#pragma once

#include "interfaces/sensor_listener.hpp"

#include <gmock/gmock.h>

class SensorListenerMock : public interfaces::SensorListener
{
  public:
    MOCK_METHOD1(sensorUpdated, void(interfaces::Sensor&));
    MOCK_METHOD2(sensorUpdated, void(interfaces::Sensor&, double));
};
