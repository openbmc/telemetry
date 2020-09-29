#pragma once

#include "interfaces/sensor.hpp"
#include "utils/generate_unique_mock_id.hpp"

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

    static Id makeId(std::string_view service, std::string_view path)
    {
        return Id("SensorMock", service, path);
    }

    MOCK_CONST_METHOD0(id, Id());
    MOCK_METHOD0(async_read, void());
    MOCK_METHOD1(registerForUpdates,
                 void(const std::weak_ptr<interfaces::SensorListener>&));
    MOCK_METHOD1(schedule, void(std::chrono::milliseconds));
    MOCK_METHOD0(stop, void());

    const uint64_t mockId = generateUniqueMockId();

    Id mockSensorId = Id("SensorMock", "", "");
};
