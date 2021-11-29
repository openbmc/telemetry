#pragma once

#include "interfaces/sensor.hpp"
#include "utils/generate_unique_mock_id.hpp"

#include <gmock/gmock.h>

class SensorMock : public interfaces::Sensor
{
  public:
    explicit SensorMock()
    {
        initialize();
    }

    explicit SensorMock(Id sensorId) : mockSensorId(sensorId)
    {
        initialize();
    }

    static Id makeId(std::string_view service, std::string_view path)
    {
        return Id("SensorMock", service, path);
    }

    MOCK_METHOD(Id, id, (), (const, override));
    MOCK_METHOD(std::string, metadata, (), (const, override));
    MOCK_METHOD(void, registerForUpdates,
                (const std::weak_ptr<interfaces::SensorListener>&), (override));
    MOCK_METHOD(void, unregisterFromUpdates,
                (const std::weak_ptr<interfaces::SensorListener>&), (override));

    const uint64_t mockId = generateUniqueMockId();

    Id mockSensorId = Id("SensorMock", "", "");

  private:
    void initialize()
    {
        ON_CALL(*this, id()).WillByDefault(testing::Invoke([this] {
            return this->mockSensorId;
        }));
    }
};
