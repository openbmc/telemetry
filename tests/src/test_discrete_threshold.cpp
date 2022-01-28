#include "dbus_environment.hpp"
#include "discrete_threshold.hpp"
#include "helpers.hpp"
#include "mocks/sensor_mock.hpp"
#include "mocks/trigger_action_mock.hpp"
#include "types/duration_types.hpp"
#include "utils/conv_container.hpp"

#include <gmock/gmock.h>

using namespace testing;
using namespace std::chrono_literals;

class TestDiscreteThreshold : public Test
{
  public:
    std::vector<std::shared_ptr<SensorMock>> sensorMocks = {
        std::make_shared<NiceMock<SensorMock>>(),
        std::make_shared<NiceMock<SensorMock>>()};
    std::vector<std::string> sensorNames = {"Sensor1", "Sensor2"};
    std::unique_ptr<TriggerActionMock> actionMockPtr =
        std::make_unique<StrictMock<TriggerActionMock>>();
    TriggerActionMock& actionMock = *actionMockPtr;
    std::shared_ptr<DiscreteThreshold> sut;

    std::shared_ptr<DiscreteThreshold>
        makeThreshold(Milliseconds dwellTime, double thresholdValue,
                      discrete::Severity severity = discrete::Severity::ok)
    {
        std::vector<std::unique_ptr<interfaces::TriggerAction>> actions;
        actions.push_back(std::move(actionMockPtr));

        return std::make_shared<DiscreteThreshold>(
            DbusEnvironment::getIoc(),
            utils::convContainer<std::shared_ptr<interfaces::Sensor>>(
                sensorMocks),
            std::move(actions), dwellTime, thresholdValue, "treshold name",
            severity);
    }

    void SetUp() override
    {
        for (size_t idx = 0; idx < sensorMocks.size(); idx++)
        {
            ON_CALL(*sensorMocks.at(idx), getName())
                .WillByDefault(Return(sensorNames[idx]));
        }

        sut = makeThreshold(0ms, 90.0, discrete::Severity::critical);
    }
};

TEST_F(TestDiscreteThreshold, initializeThresholdExpectAllSensorsAreRegistered)
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

TEST_F(TestDiscreteThreshold, thresholdIsNotInitializeExpectNoActionCommit)
{
    EXPECT_CALL(actionMock, commit(_, _, _)).Times(0);
}

TEST_F(TestDiscreteThreshold, getLabeledParamsReturnsCorrectly)
{
    LabeledThresholdParam expected = discrete::LabeledThresholdParam(
        "treshold name", discrete::Severity::critical, 0, std::to_string(90.0));
    EXPECT_EQ(sut->getThresholdParam(), expected);
}

struct DiscreteParams
{
    struct UpdateParams
    {
        size_t sensor;
        Milliseconds timestamp;
        double value;
        Milliseconds sleepAfter;

        UpdateParams(size_t sensor, Milliseconds timestamp, double value,
                     Milliseconds sleepAfter = 0ms) :
            sensor(sensor),
            timestamp(timestamp), value(value), sleepAfter(sleepAfter)
        {}
    };

    struct ExpectedParams
    {
        size_t sensor;
        Milliseconds timestamp;
        double value;
        Milliseconds waitMin;

        ExpectedParams(size_t sensor, Milliseconds timestamp, double value,
                       Milliseconds waitMin = 0ms) :
            sensor(sensor),
            timestamp(timestamp), value(value), waitMin(waitMin)
        {}
    };

    DiscreteParams& Updates(std::vector<UpdateParams> val)
    {
        updates = std::move(val);
        return *this;
    }

    DiscreteParams& Expected(std::vector<ExpectedParams> val)
    {
        expected = std::move(val);
        return *this;
    }

    DiscreteParams& ThresholdValue(double val)
    {
        thresholdValue = val;
        return *this;
    }

    DiscreteParams& DwellTime(Milliseconds val)
    {
        dwellTime = std::move(val);
        return *this;
    }

    friend void PrintTo(const DiscreteParams& o, std::ostream* os)
    {
        *os << "{ DwellTime: " << o.dwellTime.count() << "ms ";
        *os << ", ThresholdValue: " << o.thresholdValue;
        *os << ", Updates: ";
        for (const auto& [index, timestamp, value, sleepAfter] : o.updates)
        {
            *os << "{ SensorIndex: " << index
                << ", Timestamp: " << timestamp.count() << ", Value: " << value
                << ", SleepAfter: " << sleepAfter.count() << "ms }, ";
        }
        *os << "Expected: ";
        for (const auto& [index, timestamp, value, waitMin] : o.expected)
        {
            *os << "{ SensorIndex: " << index
                << ", Timestamp: " << timestamp.count() << ", Value: " << value
                << ", waitMin: " << waitMin.count() << "ms }, ";
        }
        *os << " }";
    }

    std::vector<UpdateParams> updates;
    std::vector<ExpectedParams> expected;
    double thresholdValue = 0.0;
    Milliseconds dwellTime = 0ms;
};

class TestDiscreteThresholdCommon :
    public TestDiscreteThreshold,
    public WithParamInterface<DiscreteParams>
{
  public:
    void sleep(Milliseconds duration)
    {
        if (duration != 0ms)
        {
            DbusEnvironment::sleepFor(duration);
        }
    }

    void testBodySensorIsUpdatedMultipleTimes()
    {
        std::vector<std::chrono::time_point<std::chrono::high_resolution_clock>>
            timestamps(sensorMocks.size());

        sut->initialize();

        InSequence seq;

        for (const auto& [index, timestamp, value, waitMin] :
             GetParam().expected)
        {
            EXPECT_CALL(actionMock,
                        commit(sensorNames[index], timestamp, value))
                .WillOnce(DoAll(
                    InvokeWithoutArgs([idx = index, &timestamps] {
                        timestamps[idx] =
                            std::chrono::high_resolution_clock::now();
                    }),
                    InvokeWithoutArgs(DbusEnvironment::setPromise("commit"))));
        }

        auto start = std::chrono::high_resolution_clock::now();

        for (const auto& [index, timestamp, value, sleepAfter] :
             GetParam().updates)
        {
            sut->sensorUpdated(*sensorMocks[index], timestamp, value);
            sleep(sleepAfter);
        }

        EXPECT_THAT(DbusEnvironment::waitForFutures("commit"), true);
        for (const auto& [index, timestamp, value, waitMin] :
             GetParam().expected)
        {
            EXPECT_THAT(timestamps[index] - start, Ge(waitMin));
        }
    }
};

class TestDiscreteThresholdNoDwellTime : public TestDiscreteThresholdCommon
{
  public:
    void SetUp() override
    {

        for (size_t idx = 0; idx < sensorMocks.size(); idx++)
        {
            ON_CALL(*sensorMocks.at(idx), getName())
                .WillByDefault(Return(sensorNames[idx]));
        }

        sut = makeThreshold(0ms, GetParam().thresholdValue);
    }
};

INSTANTIATE_TEST_SUITE_P(_, TestDiscreteThresholdNoDwellTime,
                         Values(DiscreteParams()
                                    .ThresholdValue(90.0)
                                    .Updates({{0, 1ms, 80.0}, {0, 2ms, 89.0}})
                                    .Expected({}),
                                DiscreteParams()
                                    .ThresholdValue(90.0)
                                    .Updates({{0, 1ms, 80.0},
                                              {0, 2ms, 90.0},
                                              {0, 3ms, 80.0},
                                              {0, 4ms, 90.0}})
                                    .Expected({{0, 2ms, 90.0}, {0, 4ms, 90.0}}),
                                DiscreteParams()
                                    .ThresholdValue(90.0)
                                    .Updates({{0, 1ms, 90.0},
                                              {0, 2ms, 99.0},
                                              {1, 3ms, 100.0},
                                              {1, 4ms, 90.0}})
                                    .Expected({{0, 1ms, 90.0},
                                               {1, 4ms, 90.0}})));

TEST_P(TestDiscreteThresholdNoDwellTime, senorsIsUpdatedMultipleTimes)
{
    testBodySensorIsUpdatedMultipleTimes();
}

class TestDiscreteThresholdWithDwellTime : public TestDiscreteThresholdCommon
{
  public:
    void SetUp() override
    {
        for (size_t idx = 0; idx < sensorMocks.size(); idx++)
        {
            ON_CALL(*sensorMocks.at(idx), getName())
                .WillByDefault(Return(sensorNames[idx]));
        }

        sut = makeThreshold(GetParam().dwellTime, GetParam().thresholdValue);
    }
};

INSTANTIATE_TEST_SUITE_P(
    _, TestDiscreteThresholdWithDwellTime,
    Values(DiscreteParams()
               .DwellTime(200ms)
               .ThresholdValue(90.0)
               .Updates({{0, 1ms, 90.0, 100ms}, {0, 2ms, 91.0}, {0, 3ms, 90.0}})
               .Expected({{0, 3ms, 90.0, 300ms}}),
           DiscreteParams()
               .DwellTime(100ms)
               .ThresholdValue(90.0)
               .Updates({{0, 1ms, 90.0, 100ms}})
               .Expected({{0, 1ms, 90.0, 100ms}}),
           DiscreteParams()
               .DwellTime(1000ms)
               .ThresholdValue(90.0)
               .Updates({{0, 1ms, 90.0, 700ms},
                         {0, 1ms, 91.0, 100ms},
                         {0, 1ms, 90.0, 300ms},
                         {0, 1ms, 91.0, 100ms}})
               .Expected({}),
           DiscreteParams()
               .DwellTime(200ms)
               .ThresholdValue(90.0)
               .Updates({{0, 1ms, 90.0},
                         {1, 2ms, 89.0, 100ms},
                         {1, 3ms, 90.0, 100ms},
                         {1, 4ms, 89.0, 100ms},
                         {1, 5ms, 90.0, 300ms},
                         {1, 6ms, 89.0, 100ms}})
               .Expected({{0, 1ms, 90, 200ms}, {1, 5ms, 90, 500ms}})));

TEST_P(TestDiscreteThresholdWithDwellTime, senorsIsUpdatedMultipleTimes)
{
    testBodySensorIsUpdatedMultipleTimes();
}
