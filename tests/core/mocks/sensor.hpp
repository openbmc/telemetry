#pragma once

#include "../../actions/return_str_view.hpp"
#include "core/interfaces/sensor.hpp"

namespace core
{
class SensorMock : public interfaces::Sensor
{
  public:
    struct IdMock : public interfaces::Sensor::Id
    {
        IdMock()
        {
            ON_CALL(*this, type()).WillByDefault(ReturnStrView("SensorMock"));
            ON_CALL(*this, str()).WillByDefault(ReturnStrView(""));
        }

        bool
            operator==(const interfaces::Sensor::Id& other) const override final
        {
            return this == &other;
        }

        bool operator<(const interfaces::Sensor::Id& other) const override final
        {
            return this < &other;
        }

        MOCK_CONST_METHOD0(type, std::string_view());
        MOCK_CONST_METHOD0(str, std::string_view());
    };

    SensorMock()
    {
        ON_CALL(*this, id()).WillByDefault(Return(id_));
    }

    bool operator==(const interfaces::Sensor& other) const override final
    {
        return this == &other;
    }

    bool operator<(const interfaces::Sensor& other) const override final
    {
        return this < &other;
    }

    MOCK_CONST_METHOD0(id, std::shared_ptr<const Id>());
    MOCK_METHOD1(async_read, void(interfaces::Sensor::ReadCallback&&));
    MOCK_METHOD1(registerForUpdates, void(interfaces::Sensor::ReadCallback&&));

    std::shared_ptr<IdMock> id_ = std::make_shared<NiceMock<IdMock>>();
};
} // namespace core
