#pragma once

#include "interfaces/sensor.hpp"
#include "mocks/generate_unique_mock_id.hpp"

#include <gmock/gmock.h>

class SensorMock : public interfaces::Sensor
{
  public:
    explicit SensorMock(Id sensorId) : mockSensorId(sensorId)
    {
        ON_CALL(*this, id()).WillByDefault(testing::Invoke([this] {
            return this->mockSensorId;
        }));
    }

    static Id makeId(std::string_view name)
    {
        return Id("SensorMock", name);
    }

    MOCK_CONST_METHOD0(id, Id());

    const uint64_t mockId = generateUniqueMockId();

    Id mockSensorId = Id("", "");
};
