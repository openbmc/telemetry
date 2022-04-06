#include "dbus_environment.hpp"
#include "helpers.hpp"
#include "helpers/matchers.hpp"
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
        std::make_shared<NiceMock<SensorMock>>(),
        std::make_shared<NiceMock<SensorMock>>()};
    std::vector<std::string> sensorNames = {"Sensor1", "Sensor2"};
    std::unique_ptr<TriggerActionMock> actionMockPtr =
        std::make_unique<StrictMock<TriggerActionMock>>();
    TriggerActionMock& actionMock = *actionMockPtr;
    std::shared_ptr<NumericThreshold> sut;
    std::string triggerId = "MyTrigger";

    void makeThreshold(Milliseconds dwellTime, numeric::Direction direction,
                       double thresholdValue,
                       numeric::Type type = numeric::Type::lowerWarning)
    {
        std::vector<std::unique_ptr<interfaces::TriggerAction>> actions;
        actions.push_back(std::move(actionMockPtr));

        sut = std::make_shared<NumericThreshold>(
            DbusEnvironment::getIoc(), triggerId,
            utils::convContainer<std::shared_ptr<interfaces::Sensor>>(
                sensorMocks),
            std::move(actions), dwellTime, direction, thresholdValue, type);
    }

    void SetUp() override
    {
        for (size_t idx = 0; idx < sensorMocks.size(); idx++)
        {
            ON_CALL(*sensorMocks.at(idx), getName())
                .WillByDefault(Return(sensorNames[idx]));
        }

        makeThreshold(0ms, numeric::Direction::increasing, 90.0,
                      numeric::Type::upperCritical);
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
    EXPECT_CALL(actionMock, commit(_, _, _, _, _)).Times(0);
}

TEST_F(TestNumericThreshold, getLabeledParamsReturnsCorrectly)
{
    LabeledThresholdParam expected = numeric::LabeledThresholdParam(
        numeric::Type::upperCritical, 0, numeric::Direction::increasing, 90.0);
    EXPECT_EQ(sut->getThresholdParam(), expected);
}

struct NumericParams
{
    NumericParams& Direction(numeric::Direction val)
    {
        direction = val;
        return *this;
    }

    NumericParams&
        Updates(std::vector<std::tuple<size_t, Milliseconds, double>> val)
    {
        updates = std::move(val);
        return *this;
    }

    NumericParams&
        Expected(std::vector<std::tuple<size_t, Milliseconds, double>> val)
    {
        expected = std::move(val);
        return *this;
    }

    friend void PrintTo(const NumericParams& o, std::ostream* os)
    {
        *os << "{ Direction: " << static_cast<int>(o.direction)
            << ", Updates: ";
        for (const auto& [index, timestamp, value] : o.updates)
        {
            *os << "{ SensorIndex: " << index
                << ", Timestamp: " << timestamp.count() << ", Value: " << value
                << " }, ";
        }
        *os << "Expected: ";
        for (const auto& [index, timestamp, value] : o.expected)
        {
            *os << "{ SensorIndex: " << index
                << ", Timestamp: " << timestamp.count() << ", Value: " << value
                << " }, ";
        }
        *os << " }";
    }

    numeric::Direction direction;
    std::vector<std::tuple<size_t, Milliseconds, double>> updates;
    std::vector<std::tuple<size_t, Milliseconds, double>> expected;
};

class TestNumericThresholdNoDwellTime :
    public TestNumericThreshold,
    public WithParamInterface<NumericParams>
{
  public:
    void SetUp() override
    {
        for (size_t idx = 0; idx < sensorMocks.size(); idx++)
        {
            ON_CALL(*sensorMocks.at(idx), getName())
                .WillByDefault(Return(sensorNames[idx]));
        }

        makeThreshold(0ms, GetParam().direction, 90.0);
    }
};

INSTANTIATE_TEST_SUITE_P(_, TestNumericThresholdNoDwellTime,
                         Values(NumericParams()
                                    .Direction(numeric::Direction::increasing)
                                    .Updates({{0, 1ms, 80.0}, {0, 2ms, 89.0}})
                                    .Expected({}),
                                NumericParams()
                                    .Direction(numeric::Direction::increasing)
                                    .Updates({{0, 1ms, 80.0}, {0, 2ms, 91.0}})
                                    .Expected({{0, 2ms, 91.0}}),
                                NumericParams()
                                    .Direction(numeric::Direction::increasing)
                                    .Updates({{0, 1ms, 80.0},
                                              {0, 2ms, 99.0},
                                              {0, 3ms, 80.0},
                                              {0, 4ms, 98.0}})
                                    .Expected({{0, 2ms, 99.0}, {0, 4ms, 98.0}}),
                                NumericParams()
                                    .Direction(numeric::Direction::increasing)
                                    .Updates({{0, 1ms, 80.0},
                                              {0, 2ms, 99.0},
                                              {1, 3ms, 100.0},
                                              {1, 4ms, 98.0}})
                                    .Expected({{0, 2ms, 99.0}}),
                                NumericParams()
                                    .Direction(numeric::Direction::decreasing)
                                    .Updates({{0, 1ms, 100.0}, {0, 2ms, 91.0}})
                                    .Expected({}),
                                NumericParams()
                                    .Direction(numeric::Direction::decreasing)
                                    .Updates({{0, 1ms, 100.0}, {0, 2ms, 80.0}})
                                    .Expected({{0, 2ms, 80.0}}),
                                NumericParams()
                                    .Direction(numeric::Direction::decreasing)
                                    .Updates({{0, 1ms, 100.0},
                                              {0, 2ms, 80.0},
                                              {0, 3ms, 99.0},
                                              {0, 4ms, 85.0}})
                                    .Expected({{0, 2ms, 80.0}, {0, 4ms, 85.0}}),
                                NumericParams()
                                    .Direction(numeric::Direction::decreasing)
                                    .Updates({{0, 1ms, 100.0},
                                              {0, 2ms, 80.0},
                                              {1, 3ms, 99.0},
                                              {1, 4ms, 88.0}})
                                    .Expected({{0, 2ms, 80.0}, {1, 4ms, 88.0}}),
                                NumericParams()
                                    .Direction(numeric::Direction::either)
                                    .Updates({{0, 1ms, 98.0}, {0, 2ms, 91.0}})
                                    .Expected({}),
                                NumericParams()
                                    .Direction(numeric::Direction::either)
                                    .Updates({{0, 1ms, 100.0},
                                              {0, 2ms, 80.0},
                                              {0, 3ms, 85.0},
                                              {0, 4ms, 91.0}})
                                    .Expected({{0, 2ms, 80.0}, {0, 4ms, 91.0}}),
                                NumericParams()
                                    .Direction(numeric::Direction::either)
                                    .Updates({{0, 1ms, 100.0},
                                              {1, 2ms, 80.0},
                                              {0, 3ms, 85.0},
                                              {1, 4ms, 91.0}})
                                    .Expected({{0, 3ms, 85.0},
                                               {1, 4ms, 91.0}})));

TEST_P(TestNumericThresholdNoDwellTime, senorsIsUpdatedMultipleTimes)
{
    InSequence seq;
    for (const auto& [index, timestamp, value] : GetParam().expected)
    {
        EXPECT_CALL(actionMock,
                    commit(triggerId, helpers::IsNoValueOfOptionalRef(),
                           sensorNames[index], timestamp, TriggerValue(value)));
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
        for (size_t idx = 0; idx < sensorMocks.size(); idx++)
        {
            ON_CALL(*sensorMocks.at(idx), getName())
                .WillByDefault(Return(sensorNames[idx]));
        }

        makeThreshold(2ms, GetParam().direction, 90.0);
    }

    void sleep()
    {
        DbusEnvironment::sleepFor(4ms);
    }
};

INSTANTIATE_TEST_SUITE_P(_, TestNumericThresholdWithDwellTime,
                         Values(NumericParams()
                                    .Direction(numeric::Direction::increasing)
                                    .Updates({{0, 1ms, 80.0}, {0, 2ms, 89.0}})
                                    .Expected({}),
                                NumericParams()
                                    .Direction(numeric::Direction::increasing)
                                    .Updates({{0, 1ms, 80.0}, {0, 2ms, 91.0}})
                                    .Expected({{0, 2ms, 91.0}}),
                                NumericParams()
                                    .Direction(numeric::Direction::increasing)
                                    .Updates({{0, 1ms, 80.0},
                                              {0, 2ms, 99.0},
                                              {0, 3ms, 80.0},
                                              {0, 4ms, 98.0}})
                                    .Expected({{0, 2ms, 99.0}, {0, 4ms, 98.0}}),
                                NumericParams()
                                    .Direction(numeric::Direction::increasing)
                                    .Updates({{0, 1ms, 80.0},
                                              {1, 2ms, 99.0},
                                              {0, 3ms, 100.0},
                                              {1, 4ms, 86.0}})
                                    .Expected({{0, 3ms, 100.0}}),
                                NumericParams()
                                    .Direction(numeric::Direction::decreasing)
                                    .Updates({{0, 1ms, 100.0}, {0, 2ms, 91.0}})
                                    .Expected({}),
                                NumericParams()
                                    .Direction(numeric::Direction::decreasing)
                                    .Updates({{0, 1ms, 100.0}, {0, 2ms, 80.0}})
                                    .Expected({{0, 2ms, 80.0}}),
                                NumericParams()
                                    .Direction(numeric::Direction::decreasing)
                                    .Updates({{0, 1ms, 100.0},
                                              {0, 2ms, 80.0},
                                              {0, 3ms, 99.0},
                                              {0, 4ms, 85.0}})
                                    .Expected({{0, 2ms, 80.0}, {0, 4ms, 85.0}}),
                                NumericParams()
                                    .Direction(numeric::Direction::decreasing)
                                    .Updates({{0, 1ms, 100.0},
                                              {0, 2ms, 80.0},
                                              {1, 3ms, 99.0},
                                              {1, 4ms, 88.0}})
                                    .Expected({{0, 2ms, 80.0}, {1, 4ms, 88.0}}),
                                NumericParams()
                                    .Direction(numeric::Direction::either)
                                    .Updates({{0, 1ms, 98.0}, {0, 2ms, 91.0}})
                                    .Expected({}),
                                NumericParams()
                                    .Direction(numeric::Direction::either)
                                    .Updates({{0, 1ms, 100.0},
                                              {0, 2ms, 80.0},
                                              {0, 3ms, 85.0},
                                              {0, 4ms, 91.0}})
                                    .Expected({{0, 2ms, 80.0}, {0, 4ms, 91.0}}),
                                NumericParams()
                                    .Direction(numeric::Direction::either)
                                    .Updates({{0, 1ms, 100.0},
                                              {1, 2ms, 80.0},
                                              {0, 3ms, 85.0},
                                              {1, 4ms, 91.0}})
                                    .Expected({{0, 3ms, 85.0},
                                               {1, 4ms, 91.0}})));

TEST_P(TestNumericThresholdWithDwellTime,
       senorsIsUpdatedMultipleTimesSleepAfterEveryUpdate)
{
    InSequence seq;
    for (const auto& [index, timestamp, value] : GetParam().expected)
    {
        EXPECT_CALL(actionMock,
                    commit(triggerId, helpers::IsNoValueOfOptionalRef(),
                           sensorNames[index], timestamp, TriggerValue(value)));
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
        for (size_t idx = 0; idx < sensorMocks.size(); idx++)
        {
            ON_CALL(*sensorMocks.at(idx), getName())
                .WillByDefault(Return(sensorNames[idx]));
        }

        makeThreshold(2ms, GetParam().direction, 90.0);
    }

    void sleep()
    {
        DbusEnvironment::sleepFor(4ms);
    }
};

INSTANTIATE_TEST_SUITE_P(_, TestNumericThresholdWithDwellTime2,
                         Values(NumericParams()
                                    .Direction(numeric::Direction::increasing)
                                    .Updates({{0, 1ms, 80.0}, {0, 2ms, 89.0}})
                                    .Expected({}),
                                NumericParams()
                                    .Direction(numeric::Direction::increasing)
                                    .Updates({{0, 1ms, 80.0}, {0, 2ms, 91.0}})
                                    .Expected({{0, 2ms, 91.0}}),
                                NumericParams()
                                    .Direction(numeric::Direction::increasing)
                                    .Updates({{0, 1ms, 80.0},
                                              {0, 2ms, 99.0},
                                              {0, 3ms, 80.0},
                                              {0, 4ms, 98.0}})
                                    .Expected({{0, 4ms, 98.0}}),
                                NumericParams()
                                    .Direction(numeric::Direction::increasing)
                                    .Updates({{0, 1ms, 80.0},
                                              {1, 2ms, 99.0},
                                              {0, 3ms, 100.0},
                                              {1, 4ms, 98.0}})
                                    .Expected({{0, 3ms, 100.0}}),
                                NumericParams()
                                    .Direction(numeric::Direction::decreasing)
                                    .Updates({{0, 1ms, 100.0}, {0, 2ms, 91.0}})
                                    .Expected({}),
                                NumericParams()
                                    .Direction(numeric::Direction::decreasing)
                                    .Updates({{0, 1ms, 100.0}, {0, 2ms, 80.0}})
                                    .Expected({{0, 2ms, 80.0}}),
                                NumericParams()
                                    .Direction(numeric::Direction::decreasing)
                                    .Updates({{0, 1ms, 100.0},
                                              {0, 2ms, 80.0},
                                              {0, 3ms, 99.0},
                                              {0, 4ms, 85.0}})
                                    .Expected({{0, 4ms, 85.0}}),
                                NumericParams()
                                    .Direction(numeric::Direction::decreasing)
                                    .Updates({{0, 1ms, 100.0},
                                              {0, 2ms, 80.0},
                                              {1, 3ms, 99.0},
                                              {1, 4ms, 88.0}})
                                    .Expected({{0, 2ms, 80.0}, {1, 4ms, 88.0}}),
                                NumericParams()
                                    .Direction(numeric::Direction::either)
                                    .Updates({{0, 1ms, 98.0}, {0, 2ms, 91.0}})
                                    .Expected({}),
                                NumericParams()
                                    .Direction(numeric::Direction::either)
                                    .Updates({{0, 1ms, 100.0},
                                              {0, 2ms, 80.0},
                                              {0, 3ms, 85.0},
                                              {0, 4ms, 91.0}})
                                    .Expected({{0, 4ms, 91.0}}),
                                NumericParams()
                                    .Direction(numeric::Direction::either)
                                    .Updates({{0, 1ms, 100.0},
                                              {1, 2ms, 80.0},
                                              {0, 3ms, 85.0},
                                              {1, 4ms, 91.0}})
                                    .Expected({{0, 3ms, 85.0},
                                               {1, 4ms, 91.0}})));

TEST_P(TestNumericThresholdWithDwellTime2,
       senorsIsUpdatedMultipleTimesSleepAfterLastUpdate)
{
    InSequence seq;
    for (const auto& [index, timestamp, value] : GetParam().expected)
    {
        EXPECT_CALL(actionMock,
                    commit(triggerId, helpers::IsNoValueOfOptionalRef(),
                           sensorNames[index], timestamp, TriggerValue(value)));
    }

    sut->initialize();
    for (const auto& [index, timestamp, value] : GetParam().updates)
    {
        sut->sensorUpdated(*sensorMocks[index], timestamp, value);
    }
    sleep();
}
