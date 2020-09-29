#pragma once

#include "interfaces/sensor_listener.hpp"

#include <gmock/gmock.h>

class SensorListenerMock : public interfaces::SensorListener
{
  public:
    void delegateIgnoringArgs()
    {
        using namespace testing;

        ON_CALL(*this, sensorUpdated(_)).WillByDefault(Invoke([this] {
            sensorUpdated();
        }));

        ON_CALL(*this, sensorUpdated(_, _)).WillByDefault(Invoke([this] {
            sensorUpdated();
        }));
    }

    MOCK_METHOD1(sensorUpdated, void(interfaces::Sensor&));
    MOCK_METHOD2(sensorUpdated, void(interfaces::Sensor&, double));

    MOCK_METHOD0(sensorUpdated, void());
};
