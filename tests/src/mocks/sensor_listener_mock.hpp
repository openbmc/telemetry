#pragma once

#include "interfaces/sensor_listener.hpp"

#include <gmock/gmock.h>

class SensorListenerMock : public interfaces::SensorListener
{
  public:
    void delegateIgnoringArgs()
    {
        using namespace testing;

        ON_CALL(*this, sensorUpdated(_, _))
            .WillByDefault(InvokeWithoutArgs([this] { sensorUpdated(); }));

        ON_CALL(*this, sensorUpdated(_, _, _))
            .WillByDefault(InvokeWithoutArgs([this] { sensorUpdated(); }));
    }

    MOCK_METHOD(void, sensorUpdated, (interfaces::Sensor&, uint64_t),
                (override));
    MOCK_METHOD(void, sensorUpdated, (interfaces::Sensor&, uint64_t, double),
                (override));

    MOCK_METHOD(void, sensorUpdated, (), ());
};
