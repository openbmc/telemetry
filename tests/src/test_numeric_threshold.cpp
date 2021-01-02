#include "dbus_environment.hpp"
#include "helpers.hpp"
#include "mocks/sensor_mock.hpp"
#include "mocks/trigger_action_mock.hpp"
#include "numeric_threshold.hpp"
#include "utils/conv_container.hpp"

#include <gmock/gmock.h>

using namespace testing;
using namespace std::chrono_literals;

class TestNumericThreshold : public Test
{
  public:
    std::vector<std::shared_ptr<SensorMock>> sensorMocks = {
        std::make_shared<NiceMock<SensorMock>>(
            interfaces::Sensor::Id("SensorMock", "ServiceMock1", "/Sensor1")),
        std::make_shared<NiceMock<SensorMock>>(
            interfaces::Sensor::Id("SensorMock", "ServiceMock2", "/Sensor2"))};
    std::unique_ptr<TriggerActionMock> actionMockPtr =
        std::make_unique<StrictMock<TriggerActionMock>>();
    TriggerActionMock& actionMock = *actionMockPtr;
    std::shared_ptr<NumericThreshold> sut;

    void makeThreshold(numeric::Level level,
                       std::chrono::milliseconds dwellTime,
                       numeric::Direction direction, double thresholdValue)
    {
        std::vector<std::unique_ptr<interfaces::TriggerAction>> actions;
        actions.push_back(std::move(actionMockPtr));

        sut = std::make_shared<NumericThreshold>(
            DbusEnvironment::getIoc(),
            utils::convContainer<std::shared_ptr<interfaces::Sensor>>(
                sensorMocks),
            std::move(actions), level, dwellTime, direction, thresholdValue);
    }

    void SetUp() override
    {
        makeThreshold(numeric::Level::upperCritical, 0ms,
                      numeric::Direction::increasing, 90.0);
    }
};

TEST_F(TestNumericThreshold, initializeThresholdExpectAllSensorsAreRegistered)
{
    for (auto& sensor : sensorMocks)
    {
        EXPECT_CALL(*sensor,
                    registerForUpdates(Truly([sut = sut.get()](const auto& x) {
                        return x.lock().get() == sut;
                    })));
    }

    sut->initialize();
}

TEST_F(TestNumericThreshold, thresholdIsNotInitializeExpectNoActionCommit)
{
    EXPECT_CALL(actionMock, commit(_, _, _)).Times(0);
}

struct NumericParams
{
    NumericParams& Direction(numeric::Direction val)
    {
        direction = val;
        return *this;
    }

    NumericParams&
        Updates(std::vector<std::tuple<size_t, uint64_t, double>> val)
    {
        updates = std::move(val);
        return *this;
    }

    NumericParams&
        Expected(std::vector<std::tuple<size_t, uint64_t, double>> val)
    {
        expected = std::move(val);
        return *this;
    }

    friend void PrintTo(const NumericParams& o, std::ostream* os)
    {
        *os << "{ Direction: " << o.direction << ", Updates: ";
        for (const auto& [index, timestamp, value] : o.updates)
        {
            *os << "{ SensorIndex: " << index << ", Timestamp: " << timestamp
                << ", Value: " << value << " }, ";
        }
        *os << "Expected: ";
        for (const auto& [index, timestamp, value] : o.expected)
        {
            *os << "{ SensorIndex: " << index << ", Timestamp: " << timestamp
                << ", Value: " << value << " }, ";
        }
        *os << " }";
    }

    numeric::Direction direction;
    std::vector<std::tuple<size_t, uint64_t, double>> updates;
    std::vector<std::tuple<size_t, uint64_t, double>> expected;
};

class TestNumericThresholdNoDwellTime :
    public TestNumericThreshold,
    public WithParamInterface<NumericParams>
{
  public:
    void SetUp() override
    {
        makeThreshold(numeric::Level::upperCritical, 0ms, GetParam().direction,
                      90.0);
    }
};

INSTANTIATE_TEST_SUITE_P(
    _, TestNumericThresholdNoDwellTime,
    Values(
        NumericParams()
            .Direction(numeric::Direction::increasing)
            .Updates({{0, 1, 80.0}, {0, 2, 89.0}})
            .Expected({}),
        NumericParams()
            .Direction(numeric::Direction::increasing)
            .Updates({{0, 1, 80.0}, {0, 2, 91.0}})
            .Expected({{0, 2, 91.0}}),
        NumericParams()
            .Direction(numeric::Direction::increasing)
            .Updates({{0, 1, 80.0}, {0, 2, 99.0}, {0, 3, 80.0}, {0, 4, 98.0}})
            .Expected({{0, 2, 99.0}, {0, 4, 98.0}}),
        NumericParams()
            .Direction(numeric::Direction::increasing)
            .Updates({{0, 1, 80.0}, {0, 2, 99.0}, {1, 3, 100.0}, {1, 4, 98.0}})
            .Expected({{0, 2, 99.0}}),
        NumericParams()
            .Direction(numeric::Direction::decreasing)
            .Updates({{0, 1, 100.0}, {0, 2, 91.0}})
            .Expected({}),
        NumericParams()
            .Direction(numeric::Direction::decreasing)
            .Updates({{0, 1, 100.0}, {0, 2, 80.0}})
            .Expected({{0, 2, 80.0}}),
        NumericParams()
            .Direction(numeric::Direction::decreasing)
            .Updates({{0, 1, 100.0}, {0, 2, 80.0}, {0, 3, 99.0}, {0, 4, 85.0}})
            .Expected({{0, 2, 80.0}, {0, 4, 85.0}}),
        NumericParams()
            .Direction(numeric::Direction::decreasing)
            .Updates({{0, 1, 100.0}, {0, 2, 80.0}, {1, 3, 99.0}, {1, 4, 88.0}})
            .Expected({{0, 2, 80.0}, {1, 4, 88.0}}),
        NumericParams()
            .Direction(numeric::Direction::either)
            .Updates({{0, 1, 98.0}, {0, 2, 91.0}})
            .Expected({}),
        NumericParams()
            .Direction(numeric::Direction::either)
            .Updates({{0, 1, 100.0}, {0, 2, 80.0}, {0, 3, 85.0}, {0, 4, 91.0}})
            .Expected({{0, 2, 80.0}, {0, 4, 91.0}}),
        NumericParams()
            .Direction(numeric::Direction::either)
            .Updates({{0, 1, 100.0}, {1, 2, 80.0}, {0, 3, 85.0}, {1, 4, 91.0}})
            .Expected({{0, 3, 85.0}, {1, 4, 91.0}})));

TEST_P(TestNumericThresholdNoDwellTime, senorsIsUpdatedMultipleTimes)
{
    InSequence seq;
    for (const auto& [index, timestamp, value] : GetParam().expected)
    {
        EXPECT_CALL(actionMock,
                    commit(sensorMocks[index]->mockSensorId, timestamp, value));
    }

    sut->initialize();
    for (const auto& [index, timestamp, value] : GetParam().updates)
    {
        sut->sensorUpdated(*sensorMocks[index], timestamp, value);
    }
}

class TestNumericThresholdWithDwellTime :
    public TestNumericThreshold,
    public WithParamInterface<NumericParams>
{
  public:
    void SetUp() override
    {
        makeThreshold(numeric::Level::upperCritical, 2ms, GetParam().direction,
                      90.0);
    }

    void sleep()
    {
        DbusEnvironment::sleepFor(4ms);
    }
};

INSTANTIATE_TEST_SUITE_P(
    _, TestNumericThresholdWithDwellTime,
    Values(
        NumericParams()
            .Direction(numeric::Direction::increasing)
            .Updates({{0, 1, 80.0}, {0, 2, 89.0}})
            .Expected({}),
        NumericParams()
            .Direction(numeric::Direction::increasing)
            .Updates({{0, 1, 80.0}, {0, 2, 91.0}})
            .Expected({{0, 2, 91.0}}),
        NumericParams()
            .Direction(numeric::Direction::increasing)
            .Updates({{0, 1, 80.0}, {0, 2, 99.0}, {0, 3, 80.0}, {0, 4, 98.0}})
            .Expected({{0, 2, 99.0}, {0, 4, 98.0}}),
        NumericParams()
            .Direction(numeric::Direction::increasing)
            .Updates({{0, 1, 80.0}, {1, 2, 99.0}, {0, 3, 100.0}, {1, 4, 86.0}})
            .Expected({{0, 3, 100.0}}),
        NumericParams()
            .Direction(numeric::Direction::decreasing)
            .Updates({{0, 1, 100.0}, {0, 2, 91.0}})
            .Expected({}),
        NumericParams()
            .Direction(numeric::Direction::decreasing)
            .Updates({{0, 1, 100.0}, {0, 2, 80.0}})
            .Expected({{0, 2, 80.0}}),
        NumericParams()
            .Direction(numeric::Direction::decreasing)
            .Updates({{0, 1, 100.0}, {0, 2, 80.0}, {0, 3, 99.0}, {0, 4, 85.0}})
            .Expected({{0, 2, 80.0}, {0, 4, 85.0}}),
        NumericParams()
            .Direction(numeric::Direction::decreasing)
            .Updates({{0, 1, 100.0}, {0, 2, 80.0}, {1, 3, 99.0}, {1, 4, 88.0}})
            .Expected({{0, 2, 80.0}, {1, 4, 88.0}}),
        NumericParams()
            .Direction(numeric::Direction::either)
            .Updates({{0, 1, 98.0}, {0, 2, 91.0}})
            .Expected({}),
        NumericParams()
            .Direction(numeric::Direction::either)
            .Updates({{0, 1, 100.0}, {0, 2, 80.0}, {0, 3, 85.0}, {0, 4, 91.0}})
            .Expected({{0, 2, 80.0}, {0, 4, 91.0}}),
        NumericParams()
            .Direction(numeric::Direction::either)
            .Updates({{0, 1, 100.0}, {1, 2, 80.0}, {0, 3, 85.0}, {1, 4, 91.0}})
            .Expected({{0, 3, 85.0}, {1, 4, 91.0}})));

TEST_P(TestNumericThresholdWithDwellTime,
       senorsIsUpdatedMultipleTimesSleepAfterEveryUpdate)
{
    InSequence seq;
    for (const auto& [index, timestamp, value] : GetParam().expected)
    {
        EXPECT_CALL(actionMock,
                    commit(sensorMocks[index]->mockSensorId, timestamp, value));
    }

    sut->initialize();
    for (const auto& [index, timestamp, value] : GetParam().updates)
    {
        sut->sensorUpdated(*sensorMocks[index], timestamp, value);
        sleep();
    }
}

class TestNumericThresholdWithDwellTime2 :
    public TestNumericThreshold,
    public WithParamInterface<NumericParams>
{
  public:
    void SetUp() override
    {
        makeThreshold(numeric::Level::upperCritical, 2ms, GetParam().direction,
                      90.0);
    }

    void sleep()
    {
        DbusEnvironment::sleepFor(4ms);
    }
};

INSTANTIATE_TEST_SUITE_P(
    _, TestNumericThresholdWithDwellTime2,
    Values(
        NumericParams()
            .Direction(numeric::Direction::increasing)
            .Updates({{0, 1, 80.0}, {0, 2, 89.0}})
            .Expected({}),
        NumericParams()
            .Direction(numeric::Direction::increasing)
            .Updates({{0, 1, 80.0}, {0, 2, 91.0}})
            .Expected({{0, 2, 91.0}}),
        NumericParams()
            .Direction(numeric::Direction::increasing)
            .Updates({{0, 1, 80.0}, {0, 2, 99.0}, {0, 3, 80.0}, {0, 4, 98.0}})
            .Expected({{0, 4, 98.0}}),
        NumericParams()
            .Direction(numeric::Direction::increasing)
            .Updates({{0, 1, 80.0}, {1, 2, 99.0}, {0, 3, 100.0}, {1, 4, 98.0}})
            .Expected({{0, 3, 100.0}}),
        NumericParams()
            .Direction(numeric::Direction::decreasing)
            .Updates({{0, 1, 100.0}, {0, 2, 91.0}})
            .Expected({}),
        NumericParams()
            .Direction(numeric::Direction::decreasing)
            .Updates({{0, 1, 100.0}, {0, 2, 80.0}})
            .Expected({{0, 2, 80.0}}),
        NumericParams()
            .Direction(numeric::Direction::decreasing)
            .Updates({{0, 1, 100.0}, {0, 2, 80.0}, {0, 3, 99.0}, {0, 4, 85.0}})
            .Expected({{0, 4, 85.0}}),
        NumericParams()
            .Direction(numeric::Direction::decreasing)
            .Updates({{0, 1, 100.0}, {0, 2, 80.0}, {1, 3, 99.0}, {1, 4, 88.0}})
            .Expected({{0, 2, 80.0}, {1, 4, 88.0}}),
        NumericParams()
            .Direction(numeric::Direction::either)
            .Updates({{0, 1, 98.0}, {0, 2, 91.0}})
            .Expected({}),
        NumericParams()
            .Direction(numeric::Direction::either)
            .Updates({{0, 1, 100.0}, {0, 2, 80.0}, {0, 3, 85.0}, {0, 4, 91.0}})
            .Expected({{0, 4, 91.0}}),
        NumericParams()
            .Direction(numeric::Direction::either)
            .Updates({{0, 1, 100.0}, {1, 2, 80.0}, {0, 3, 85.0}, {1, 4, 91.0}})
            .Expected({{0, 3, 85.0}, {1, 4, 91.0}})));

TEST_P(TestNumericThresholdWithDwellTime2,
       senorsIsUpdatedMultipleTimesSleepAfterLastUpdate)
{
    InSequence seq;
    for (const auto& [index, timestamp, value] : GetParam().expected)
    {
        EXPECT_CALL(actionMock,
                    commit(sensorMocks[index]->mockSensorId, timestamp, value));
    }

    sut->initialize();
    for (const auto& [index, timestamp, value] : GetParam().updates)
    {
        sut->sensorUpdated(*sensorMocks[index], timestamp, value);
    }
    sleep();
}
