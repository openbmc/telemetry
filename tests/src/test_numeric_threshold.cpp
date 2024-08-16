#include "dbus_environment.hpp"
#include "helpers.hpp"
#include "mocks/clock_mock.hpp"
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
    std::unique_ptr<NiceMock<ClockMock>> clockMockPtr =
        std::make_unique<NiceMock<ClockMock>>();

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
            std::move(actions), dwellTime, direction, thresholdValue, type,
            std::move(clockMockPtr));
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
    struct UpdateParams
    {
        size_t sensor;
        double value;
        Milliseconds sleepAfter;

        UpdateParams(size_t sensor, double value,
                     Milliseconds sleepAfter = 0ms) :
            sensor(sensor), value(value), sleepAfter(sleepAfter)
        {}
    };

    struct ExpectedParams
    {
        size_t sensor;
        double value;
        Milliseconds waitMin;

        ExpectedParams(size_t sensor, double value,
                       Milliseconds waitMin = 0ms) :
            sensor(sensor), value(value), waitMin(waitMin)
        {}
    };

    NumericParams& Direction(numeric::Direction val)
    {
        direction = val;
        return *this;
    }

    NumericParams& Updates(std::vector<UpdateParams> val)
    {
        updates = std::move(val);
        return *this;
    }

    NumericParams& Expected(std::vector<ExpectedParams> val)
    {
        expected = std::move(val);
        return *this;
    }

    NumericParams& ThresholdValue(double val)
    {
        thresholdValue = val;
        return *this;
    }

    NumericParams& DwellTime(Milliseconds val)
    {
        dwellTime = std::move(val);
        return *this;
    }

    NumericParams& InitialValues(std::vector<double> val)
    {
        initialValues = std::move(val);
        return *this;
    }

    friend void PrintTo(const NumericParams& o, std::ostream* os)
    {
        *os << "{ DwellTime: " << o.dwellTime.count() << "ms ";
        *os << ", ThresholdValue: " << o.thresholdValue;
        *os << ", Direction: " << static_cast<int>(o.direction);
        *os << ", InitialValues: [ ";
        size_t idx = 0;
        for (const double value : o.initialValues)
        {
            *os << "{ SensorIndex: " << idx << ", Value: " << value << " }, ";
            idx++;
        }
        *os << " ], Updates: [ ";
        for (const auto& [index, value, sleepAfter] : o.updates)
        {
            *os << "{ SensorIndex: " << index << ", Value: " << value
                << ", SleepAfter: " << sleepAfter.count() << "ms }, ";
        }
        *os << " ], Expected: [ ";
        for (const auto& [index, value, waitMin] : o.expected)
        {
            *os << "{ SensorIndex: " << index << ", Value: " << value
                << ", waitMin: " << waitMin.count() << "ms }, ";
        }
        *os << " ] }";
    }

    numeric::Direction direction;
    double thresholdValue = 0.0;
    Milliseconds dwellTime = 0ms;
    std::vector<UpdateParams> updates;
    std::vector<ExpectedParams> expected;
    std::vector<double> initialValues;
};

class TestNumericThresholdCommon :
    public TestNumericThreshold,
    public WithParamInterface<NumericParams>
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

        for (const auto& [index, value, waitMin] : GetParam().expected)
        {
            EXPECT_CALL(actionMock,
                        commit(triggerId, Eq(std::nullopt), sensorNames[index],
                               _, TriggerValue(value)))
                .WillOnce(DoAll(
                    InvokeWithoutArgs([idx = index, &timestamps] {
                timestamps[idx] = std::chrono::high_resolution_clock::now();
            }),
                    InvokeWithoutArgs(DbusEnvironment::setPromise("commit"))));
        }

        auto start = std::chrono::high_resolution_clock::now();

        size_t idx = 0;
        for (const double value : GetParam().initialValues)
        {
            sut->sensorUpdated(*sensorMocks[idx], 0ms, value);
            idx++;
        }

        for (const auto& [index, value, sleepAfter] : GetParam().updates)
        {
            ASSERT_LT(index, GetParam().initialValues.size())
                << "Initial value was not specified for sensor with index: "
                << index;
            sut->sensorUpdated(*sensorMocks[index], 42ms, value);
            sleep(sleepAfter);
        }

        EXPECT_THAT(DbusEnvironment::waitForFutures("commit"), true);
        for (const auto& [index, value, waitMin] : GetParam().expected)
        {
            EXPECT_THAT(timestamps[index] - start, Ge(waitMin));
        }
    }
};

class TestNumericThresholdNoDwellTime : public TestNumericThresholdCommon
{
  public:
    void SetUp() override
    {
        for (size_t idx = 0; idx < sensorMocks.size(); idx++)
        {
            ON_CALL(*sensorMocks.at(idx), getName())
                .WillByDefault(Return(sensorNames[idx]));
        }

        makeThreshold(0ms, GetParam().direction, GetParam().thresholdValue);
    }
};

INSTANTIATE_TEST_SUITE_P(_, TestNumericThresholdNoDwellTime,
                         Values(NumericParams()
                                    .ThresholdValue(90.0)
                                    .Direction(numeric::Direction::increasing)
                                    .InitialValues({80.0})
                                    .Updates({{0, 89.0}})
                                    .Expected({}),
                                NumericParams()
                                    .ThresholdValue(90.0)
                                    .Direction(numeric::Direction::increasing)
                                    .InitialValues({80.0})
                                    .Updates({{0, 91.0}})
                                    .Expected({{0, 91.0}}),
                                NumericParams()
                                    .ThresholdValue(90.0)
                                    .Direction(numeric::Direction::increasing)
                                    .InitialValues({80.0})
                                    .Updates({{0, 99.0}, {0, 80.0}, {0, 98.0}})
                                    .Expected({{0, 99.0}, {0, 98.0}}),
                                NumericParams()
                                    .ThresholdValue(90.0)
                                    .Direction(numeric::Direction::increasing)
                                    .InitialValues({80.0, 100.0})
                                    .Updates({{0, 99.0}, {1, 98.0}})
                                    .Expected({{0, 99.0}}),
                                NumericParams()
                                    .ThresholdValue(90.0)
                                    .Direction(numeric::Direction::decreasing)
                                    .InitialValues({100.0})
                                    .Updates({{0, 91.0}})
                                    .Expected({}),
                                NumericParams()
                                    .ThresholdValue(90.0)
                                    .Direction(numeric::Direction::decreasing)
                                    .InitialValues({100.0})
                                    .Updates({{0, 80.0}})
                                    .Expected({{0, 80.0}}),
                                NumericParams()
                                    .ThresholdValue(90.0)
                                    .Direction(numeric::Direction::decreasing)
                                    .InitialValues({100.0})
                                    .Updates({{0, 80.0}, {0, 99.0}, {0, 85.0}})
                                    .Expected({{0, 80.0}, {0, 85.0}}),
                                NumericParams()
                                    .ThresholdValue(90.0)
                                    .Direction(numeric::Direction::decreasing)
                                    .InitialValues({100.0, 99.0})
                                    .Updates({{0, 80.0}, {1, 88.0}})
                                    .Expected({{0, 80.0}, {1, 88.0}}),
                                NumericParams()
                                    .ThresholdValue(90.0)
                                    .Direction(numeric::Direction::either)
                                    .InitialValues({98.0})
                                    .Updates({{0, 91.0}})
                                    .Expected({}),
                                NumericParams()
                                    .ThresholdValue(90.0)
                                    .Direction(numeric::Direction::either)
                                    .InitialValues({100.0})
                                    .Updates({{0, 80.0}, {0, 85.0}, {0, 91.0}})
                                    .Expected({{0, 80.0}, {0, 91.0}}),
                                NumericParams()
                                    .ThresholdValue(90.0)
                                    .Direction(numeric::Direction::either)
                                    .InitialValues({100.0, 80.0})
                                    .Updates({{0, 85.0}, {1, 91.0}})
                                    .Expected({{0, 85.0}, {1, 91.0}}),
                                NumericParams()
                                    .ThresholdValue(30.0)
                                    .Direction(numeric::Direction::decreasing)
                                    .InitialValues({40.0})
                                    .Updates({{0, 30.0}, {0, 20.0}})
                                    .Expected({{0, 20.0}}),
                                NumericParams()
                                    .ThresholdValue(30.0)
                                    .Direction(numeric::Direction::decreasing)
                                    .InitialValues({20.0})
                                    .Updates({{0, 30.0}, {0, 20.0}})
                                    .Expected({}),
                                NumericParams()
                                    .ThresholdValue(30.0)
                                    .Direction(numeric::Direction::either)
                                    .InitialValues({20.0})
                                    .Updates({{0, 30.0}, {0, 20.0}})
                                    .Expected({}),
                                NumericParams()
                                    .ThresholdValue(30.0)
                                    .Direction(numeric::Direction::increasing)
                                    .InitialValues({20.0})
                                    .Updates({{0, 30.0}, {0, 40.0}})
                                    .Expected({{0, 40.0}}),
                                NumericParams()
                                    .ThresholdValue(30.0)
                                    .Direction(numeric::Direction::increasing)
                                    .InitialValues({40.0})
                                    .Updates({{0, 30.0}, {0, 40.0}})
                                    .Expected({}),
                                NumericParams()
                                    .ThresholdValue(30.0)
                                    .Direction(numeric::Direction::either)
                                    .InitialValues({40.0})
                                    .Updates({{0, 30.0}, {0, 40.0}})
                                    .Expected({})));

TEST_P(TestNumericThresholdNoDwellTime, senorsIsUpdatedMultipleTimes)
{
    testBodySensorIsUpdatedMultipleTimes();
}

class TestNumericThresholdWithDwellTime : public TestNumericThresholdCommon
{
  public:
    void SetUp() override
    {
        for (size_t idx = 0; idx < sensorMocks.size(); idx++)
        {
            ON_CALL(*sensorMocks.at(idx), getName())
                .WillByDefault(Return(sensorNames[idx]));
        }

        makeThreshold(GetParam().dwellTime, GetParam().direction,
                      GetParam().thresholdValue);
    }
};

INSTANTIATE_TEST_SUITE_P(
    SleepAfterEveryUpdate, TestNumericThresholdWithDwellTime,
    Values(NumericParams()
               .DwellTime(200ms)
               .ThresholdValue(90.0)
               .Direction(numeric::Direction::increasing)
               .InitialValues({80.0})
               .Updates({{0, 89.0, 200ms}})
               .Expected({}),
           NumericParams()
               .DwellTime(200ms)
               .ThresholdValue(90.0)
               .Direction(numeric::Direction::increasing)
               .InitialValues({80.0})
               .Updates({{0, 91.0, 200ms}})
               .Expected({{0, 91.0, 200ms}}),
           NumericParams()
               .DwellTime(200ms)
               .ThresholdValue(90.0)
               .Direction(numeric::Direction::increasing)
               .InitialValues({80.0})
               .Updates({{0, 99.0, 200ms}, {0, 80.0, 100ms}, {0, 98.0, 200ms}})
               .Expected({{0, 99.0, 200ms}, {0, 98.0, 500ms}}),
           NumericParams()
               .DwellTime(200ms)
               .ThresholdValue(90.0)
               .Direction(numeric::Direction::increasing)
               .InitialValues({80.0, 99.0})
               .Updates({{0, 100.0, 100ms}, {1, 86.0, 100ms}})
               .Expected({{0, 100.0, 200ms}}),
           NumericParams()
               .DwellTime(200ms)
               .ThresholdValue(90.0)
               .Direction(numeric::Direction::decreasing)
               .InitialValues({100.0})
               .Updates({{0, 91.0, 200ms}})
               .Expected({}),
           NumericParams()
               .DwellTime(200ms)
               .ThresholdValue(90.0)
               .Direction(numeric::Direction::decreasing)
               .InitialValues({100.0})
               .Updates({{0, 80.0, 200ms}})
               .Expected({{0, 80.0, 200ms}}),
           NumericParams()
               .DwellTime(200ms)
               .ThresholdValue(90.0)
               .Direction(numeric::Direction::decreasing)
               .InitialValues({100.0})
               .Updates({{0, 80.0, 200ms}, {0, 99.0, 100ms}, {0, 85.0, 200ms}})
               .Expected({{0, 80.0, 200ms}, {0, 85.0, 500ms}}),
           NumericParams()
               .DwellTime(200ms)
               .ThresholdValue(90.0)
               .Direction(numeric::Direction::decreasing)
               .InitialValues({100.0, 99.0})
               .Updates({{0, 80.0, 200ms}, {1, 88.0, 200ms}})
               .Expected({{0, 80.0, 200ms}, {1, 88.0, 400ms}}),
           NumericParams()
               .DwellTime(200ms)
               .ThresholdValue(90.0)
               .Direction(numeric::Direction::either)
               .InitialValues({98.0})
               .Updates({{0, 91.0, 200ms}})
               .Expected({}),
           NumericParams()
               .DwellTime(200ms)
               .ThresholdValue(90.0)
               .Direction(numeric::Direction::either)
               .InitialValues({100.0})
               .Updates({{0, 80.0, 100ms}, {0, 85.0, 100ms}, {0, 91.0, 200ms}})
               .Expected({{0, 80.0, 200ms}, {0, 91.0, 400ms}}),
           NumericParams()
               .DwellTime(200ms)
               .ThresholdValue(90.0)
               .Direction(numeric::Direction::either)
               .InitialValues({100.0, 80.0})
               .Updates({{0, 85.0, 100ms}, {1, 91.0, 200ms}})
               .Expected({{0, 85.0, 200ms}, {1, 91.0, 300ms}})));

INSTANTIATE_TEST_SUITE_P(
    SleepAfterLastUpdate, TestNumericThresholdWithDwellTime,
    Values(NumericParams()
               .DwellTime(200ms)
               .ThresholdValue(90.0)
               .Direction(numeric::Direction::increasing)
               .InitialValues({80.0})
               .Updates({{0, 89.0, 300ms}})
               .Expected({}),
           NumericParams()
               .DwellTime(200ms)
               .ThresholdValue(90.0)
               .Direction(numeric::Direction::increasing)
               .InitialValues({80.0})
               .Updates({{0, 91.0, 300ms}})
               .Expected({{0, 91.0, 200ms}}),
           NumericParams()
               .DwellTime(200ms)
               .ThresholdValue(90.0)
               .Direction(numeric::Direction::increasing)
               .InitialValues({80.0})
               .Updates({{0, 99.0}, {0, 80.0}, {0, 98.0, 300ms}})
               .Expected({{0, 98.0, 200ms}}),
           NumericParams()
               .DwellTime(200ms)
               .ThresholdValue(90.0)
               .Direction(numeric::Direction::increasing)
               .InitialValues({80.0, 99.0})
               .Updates({{0, 100.0}, {1, 98.0, 300ms}})
               .Expected({{0, 100.0, 200ms}}),
           NumericParams()
               .DwellTime(200ms)
               .ThresholdValue(90.0)
               .Direction(numeric::Direction::decreasing)
               .InitialValues({100.0})
               .Updates({{0, 91.0, 300ms}})
               .Expected({}),
           NumericParams()
               .DwellTime(200ms)
               .ThresholdValue(90.0)
               .Direction(numeric::Direction::decreasing)
               .InitialValues({100.0})
               .Updates({{0, 80.0, 300ms}})
               .Expected({{0, 80.0, 200ms}}),
           NumericParams()
               .DwellTime(200ms)
               .ThresholdValue(90.0)
               .Direction(numeric::Direction::decreasing)
               .InitialValues({100.0})
               .Updates({{0, 80.0}, {0, 99.0}, {0, 85.0, 300ms}})
               .Expected({{0, 85.0, 200ms}}),
           NumericParams()
               .DwellTime(200ms)
               .ThresholdValue(90.0)
               .Direction(numeric::Direction::decreasing)
               .InitialValues({100.0, 99.0})
               .Updates({{0, 80.0}, {1, 88.0, 300ms}})
               .Expected({{0, 80.0, 200ms}, {1, 88.0, 200ms}}),
           NumericParams()
               .DwellTime(200ms)
               .ThresholdValue(90.0)
               .Direction(numeric::Direction::either)
               .InitialValues({98.0})
               .Updates({{0, 91.0, 300ms}})
               .Expected({}),
           NumericParams()
               .DwellTime(200ms)
               .ThresholdValue(90.0)
               .Direction(numeric::Direction::either)
               .InitialValues({100.0})
               .Updates({{0, 80.0}, {0, 85.0}, {0, 91.0, 300ms}})
               .Expected({{0, 91.0, 200ms}}),
           NumericParams()
               .DwellTime(200ms)
               .ThresholdValue(90.0)
               .Direction(numeric::Direction::either)
               .InitialValues({100.0, 80.0})
               .Updates({{0, 85.0}, {1, 91.0, 300ms}})
               .Expected({{0, 85.0, 200ms}, {1, 91.0, 200ms}})));

TEST_P(TestNumericThresholdWithDwellTime, senorsIsUpdatedMultipleTimes)
{
    testBodySensorIsUpdatedMultipleTimes();
}
