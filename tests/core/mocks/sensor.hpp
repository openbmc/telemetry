#pragma once

#include "../../actions/return_str_view.hpp"
#include "core/interfaces/sensor.hpp"

namespace core
{
class SensorMock : public interfaces::Sensor
{
  public:
    SensorMock()
    {
        ON_CALL(*this, globalId())
            .WillByDefault(Return(
                interfaces::Sensor::GlobalId("core::SensorMock::Id", "0")));
        ON_CALL(*this, path()).WillByDefault(ReturnStrView(""));
    }

    bool operator==(const interfaces::Sensor& other) const override final
    {
        return this == &other;
    }

    bool operator<(const interfaces::Sensor& other) const override final
    {
        return this < &other;
    }

    MOCK_CONST_METHOD0(globalId, interfaces::Sensor::GlobalId());
    MOCK_CONST_METHOD0(path, std::string_view());
    MOCK_METHOD1(async_read, void(interfaces::Sensor::ReadCallback&&));
    MOCK_METHOD1(registerForUpdates, void(interfaces::Sensor::ReadCallback&&));
};
} // namespace core
