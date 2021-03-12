#include "dbus_environment.hpp"
#include "discrete_threshold.hpp"
#include "helpers.hpp"
#include "mocks/sensor_mock.hpp"
#include "mocks/trigger_action_mock.hpp"
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

    void makeThreshold(std::chrono::milliseconds dwellTime,
                       std::optional<double> thresholdValue)
    {
        std::vector<std::unique_ptr<interfaces::TriggerAction>> actions;
        actions.push_back(std::move(actionMockPtr));

        sut = std::make_shared<DiscreteThreshold>(
            DbusEnvironment::getIoc(),
            utils::convContainer<std::shared_ptr<interfaces::Sensor>>(
                sensorMocks),
            sensorNames, std::move(actions), dwellTime, thresholdValue,
            "treshold_name");
    }

    void SetUp() override
    {
        makeThreshold(0ms, 90.0);
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

struct DiscreteParams
{
    struct UpdateParams
    {
        size_t sensor;
        uint64_t timestamp;
        double value;
        std::chrono::milliseconds sleepAfter;

        UpdateParams(size_t sensor, uint64_t timestamp, double value,
                     std::chrono::milliseconds sleepAfter = 0ms) :
            sensor(sensor),
            timestamp(timestamp), value(value), sleepAfter(sleepAfter)
        {}
    };

    struct ExpectedParams
    {
        size_t sensor;
        uint64_t timestamp;
        double value;
        std::chrono::milliseconds waitMin;

        ExpectedParams(size_t sensor, uint64_t timestamp, double value,
                       std::chrono::milliseconds waitMin = 0ms) :
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

    DiscreteParams& ThresholdValue(std::optional<double> val)
    {
        thresholdValue = std::move(val);
        return *this;
    }

    DiscreteParams& DwellTime(std::chrono::milliseconds val)
    {
        dwellTime = std::move(val);
        return *this;
    }

    friend void PrintTo(const DiscreteParams& o, std::ostream* os)
    {
        *os << "{ DwellTime: " << o.dwellTime.count() << "ms ";
        if (o.thresholdValue.has_value())
        {
            *os << ", ThresholdValue: " << o.thresholdValue.value();
        }
        else
        {
            *os << ", NoThresholdValue";
        }
        *os << ", Updates: ";
        for (const auto& [index, timestamp, value, sleepAfter] : o.updates)
        {
            *os << "{ SensorIndex: " << index << ", Timestamp: " << timestamp
                << ", Value: " << value
                << ", SleepAfter: " << sleepAfter.count() << "ms }, ";
        }
        *os << "Expected: ";
        for (const auto& [index, timestamp, value, waitMin] : o.expected)
        {
            *os << "{ SensorIndex: " << index << ", Timestamp: " << timestamp
                << ", Value: " << value << ", waitMin: " << waitMin.count()
                << "ms }, ";
        }
        *os << " }";
    }

    std::vector<UpdateParams> updates;
    std::vector<ExpectedParams> expected;
    std::optional<double> thresholdValue = std::nullopt;
    std::chrono::milliseconds dwellTime = 0ms;
};

class TestDiscreteThresholdCommon :
    public TestDiscreteThreshold,
    public WithParamInterface<DiscreteParams>
{
  public:
    void sleep(std::chrono::milliseconds duration)
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
                    InvokeWithoutArgs([&] {
                        timestamps[index] =
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
        makeThreshold(0ms, GetParam().thresholdValue);
    }
};

INSTANTIATE_TEST_SUITE_P(
    _, TestDiscreteThresholdNoDwellTime,
    Values(
        DiscreteParams()
            .ThresholdValue(90.0)
            .Updates({{0, 1, 80.0}, {0, 2, 89.0}})
            .Expected({}),
        DiscreteParams()
            .ThresholdValue(90.0)
            .Updates({{0, 1, 80.0}, {0, 2, 90.0}, {0, 3, 80.0}, {0, 4, 90.0}})
            .Expected({{0, 2, 90.0}, {0, 4, 90.0}}),
        DiscreteParams()
            .ThresholdValue(90.0)
            .Updates({{0, 1, 90.0}, {0, 2, 99.0}, {1, 3, 100.0}, {1, 4, 90.0}})
            .Expected({{0, 1, 90.0}, {1, 4, 90.0}})));

TEST_P(TestDiscreteThresholdNoDwellTime, senorsIsUpdatedMultipleTimes)
{
    testBodySensorIsUpdatedMultipleTimes();
}

class TestDiscreteThresholdNoValueNoDwellTime :
    public TestDiscreteThresholdNoDwellTime
{
  public:
    void SetUp() override
    {
        makeThreshold(0ms, std::nullopt);
    }
};

INSTANTIATE_TEST_SUITE_P(
    _, TestDiscreteThresholdNoValueNoDwellTime,
    Values(
        DiscreteParams().Updates({{0, 1, 80.0}}).Expected({}),
        DiscreteParams().Updates({{0, 1, 80.0}, {1, 2, 81.0}}).Expected({}),
        DiscreteParams()
            .Updates({{0, 1, 80.0}, {0, 2, 90.0}})
            .Expected({{0, 2, 90.0}}),
        DiscreteParams()
            .Updates({{0, 1, 80.0}, {1, 2, 90.0}, {0, 3, 90.0}})
            .Expected({{0, 3, 90.0}}),
        DiscreteParams()
            .Updates({{0, 1, 80.0}, {1, 2, 80.0}, {1, 3, 90.0}, {0, 4, 90.0}})
            .Expected({{1, 3, 90.0}, {0, 4, 90.0}})));

TEST_P(TestDiscreteThresholdNoValueNoDwellTime, senorsIsUpdatedMultipleTimes)
{
    testBodySensorIsUpdatedMultipleTimes();
}

class TestDiscreteThresholdWithDwellTime : public TestDiscreteThresholdCommon
{
  public:
    void SetUp() override
    {
        makeThreshold(GetParam().dwellTime, GetParam().thresholdValue);
    }
};

INSTANTIATE_TEST_SUITE_P(
    _, TestDiscreteThresholdWithDwellTime,
    Values(DiscreteParams()
               .DwellTime(200ms)
               .ThresholdValue(90.0)
               .Updates({{0, 1, 90.0, 100ms}, {0, 2, 91.0}, {0, 3, 90.0}})
               .Expected({{0, 3, 90.0, 300ms}}),
           DiscreteParams()
               .DwellTime(100ms)
               .ThresholdValue(90.0)
               .Updates({{0, 1, 90.0, 100ms}})
               .Expected({{0, 1, 90.0, 100ms}}),
           DiscreteParams()
               .DwellTime(1000ms)
               .ThresholdValue(90.0)
               .Updates({{0, 1, 90.0, 700ms},
                         {0, 1, 91.0, 100ms},
                         {0, 1, 90.0, 300ms},
                         {0, 1, 91.0, 100ms}})
               .Expected({}),
           DiscreteParams()
               .DwellTime(200ms)
               .ThresholdValue(90.0)
               .Updates({{0, 1, 90.0},
                         {1, 2, 89.0, 100ms},
                         {1, 3, 90.0, 100ms},
                         {1, 4, 89.0, 100ms},
                         {1, 5, 90.0, 300ms},
                         {1, 6, 89.0, 100ms}})
               .Expected({{0, 1, 90, 200ms}, {1, 5, 90, 500ms}})));

TEST_P(TestDiscreteThresholdWithDwellTime, senorsIsUpdatedMultipleTimes)
{
    testBodySensorIsUpdatedMultipleTimes();
}

class TestDiscreteThresholdNoValueWithDwellTime :
    public TestDiscreteThresholdWithDwellTime
{
  public:
    void SetUp() override
    {
        makeThreshold(GetParam().dwellTime, std::nullopt);
    }
};

INSTANTIATE_TEST_SUITE_P(
    _, TestDiscreteThresholdNoValueWithDwellTime,
    Values(DiscreteParams()
               .DwellTime(200ms)
               .Updates({{0, 1, 80.0, 300ms}})
               .Expected({}),
           DiscreteParams()
               .DwellTime(200ms)
               .Updates({{0, 1, 80.0, 300ms}, {0, 1, 81.0, 300ms}})
               .Expected({{0, 1, 81.0, 500ms}}),
           DiscreteParams()
               .DwellTime(500ms)
               .Updates({{0, 1, 80.0, 100ms}, {0, 1, 81.0, 100ms}})
               .Expected({{0, 1, 81.0, 600ms}}),
           DiscreteParams()
               .DwellTime(200ms)
               .Updates({{0, 1, 80.0},
                         {0, 2, 81.0},
                         {1, 3, 89.0, 100ms},
                         {1, 4, 90.0, 100ms},
                         {1, 5, 89.0, 100ms},
                         {1, 6, 90.0, 200ms},
                         {1, 7, 89.0, 200ms}})
               .Expected({{0, 2, 81.0, 200ms},
                          {1, 6, 90.0, 500ms},
                          {1, 7, 89.0, 700ms}}),
           DiscreteParams()
               .DwellTime(300ms)
               .Updates({{0, 1, 90.0},
                         {0, 2, 91.0, 100ms},
                         {0, 3, 92.0, 100ms},
                         {0, 4, 93.0, 100ms}})
               .Expected({{0, 4, 93.0, 300ms}})));

TEST_P(TestDiscreteThresholdNoValueWithDwellTime, senorsIsUpdatedMultipleTimes)
{
    testBodySensorIsUpdatedMultipleTimes();
}
