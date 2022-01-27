#include "fakes/clock_fake.hpp"
#include "helpers.hpp"
#include "metric.hpp"
#include "mocks/sensor_listener_mock.hpp"
#include "mocks/sensor_mock.hpp"
#include "params/metric_params.hpp"
#include "utils/conv_container.hpp"
#include "utils/conversion.hpp"
#include "utils/tstring.hpp"

#include <gmock/gmock.h>

using namespace testing;
using namespace std::chrono_literals;

namespace tstring = utils::tstring;

constexpr Milliseconds systemTimestamp = 42ms;

class TestMetric : public Test
{
  public:
    TestMetric()
    {
        clockFake.steady.reset();
        clockFake.system.set(systemTimestamp);
    }

    static std::vector<std::shared_ptr<SensorMock>>
        makeSensorMocks(size_t amount)
    {
        std::vector<std::shared_ptr<SensorMock>> result;
        for (size_t i = 0; i < amount; ++i)
        {
            auto& metricMock =
                result.emplace_back(std::make_shared<NiceMock<SensorMock>>());
            ON_CALL(*metricMock, metadata()).WillByDefault(Return("metadata"));
        }
        return result;
    }

    std::shared_ptr<Metric> makeSut(const MetricParams& p)
    {
        return std::make_shared<Metric>(
            utils::convContainer<std::shared_ptr<interfaces::Sensor>>(
                sensorMocks),
            p.operationType(), p.id(), p.collectionTimeScope(),
            p.collectionDuration(), std::move(clockFakePtr));
    }

    MetricParams params = MetricParams()
                              .id("id")
                              .operationType(OperationType::avg)
                              .collectionTimeScope(CollectionTimeScope::point)
                              .collectionDuration(CollectionDuration(0ms));
    std::vector<std::shared_ptr<SensorMock>> sensorMocks = makeSensorMocks(1u);
    std::unique_ptr<ClockFake> clockFakePtr = std::make_unique<ClockFake>();
    ClockFake& clockFake = *clockFakePtr;
    std::shared_ptr<Metric> sut;
};

TEST_F(TestMetric, subscribesForSensorDuringInitialization)
{
    sut = makeSut(params);

    EXPECT_CALL(*sensorMocks.front(),
                registerForUpdates(Truly([sut = sut.get()](const auto& a0) {
                    return a0.lock().get() == sut;
                })));

    sut->initialize();
}

TEST_F(TestMetric, unsubscribesForSensorDuringDeinitialization)
{
    sut = makeSut(params);

    EXPECT_CALL(*sensorMocks.front(),
                unregisterFromUpdates(Truly([sut = sut.get()](const auto& a0) {
                    return a0.lock().get() == sut;
                })));

    sut->deinitialize();
}

TEST_F(TestMetric, containsEmptyReadingAfterCreated)
{
    sut = makeSut(params);

    ASSERT_THAT(sut->getReadings(),
                ElementsAre(MetricValue({"id", "metadata", 0., 0u})));
}

class TestMetricAfterInitialization : public TestMetric
{
  public:
    void SetUp() override
    {
        sut = makeSut(params);
        sut->initialize();
    }

    NiceMock<SensorListenerMock> listenerMock;
};

TEST_F(TestMetricAfterInitialization, containsEmptyReading)
{
    ASSERT_THAT(sut->getReadings(),
                ElementsAre(MetricValue({"id", "metadata", 0., 0u})));
}

TEST_F(TestMetricAfterInitialization, updatesMetricValuesOnSensorUpdate)
{
    sut->sensorUpdated(*sensorMocks.front(), Milliseconds{18}, 31.2);

    ASSERT_THAT(
        sut->getReadings(),
        ElementsAre(MetricValue{"id", "metadata", 31.2,
                                std::chrono::duration_cast<Milliseconds>(
                                    clockFake.system.timestamp())
                                    .count()}));
}

TEST_F(TestMetricAfterInitialization,
       throwsWhenUpdateIsPerformedOnUnknownSensor)
{
    auto sensor = std::make_shared<StrictMock<SensorMock>>();
    EXPECT_THROW(sut->sensorUpdated(*sensor, Milliseconds{10}, 20.0),
                 std::out_of_range);
}

TEST_F(TestMetricAfterInitialization, dumpsConfiguration)
{
    namespace ts = utils::tstring;

    ON_CALL(*sensorMocks.front(), id())
        .WillByDefault(Return(SensorMock::makeId("service1", "path1")));
    ON_CALL(*sensorMocks.front(), metadata())
        .WillByDefault(Return("metadata1"));

    const auto conf = sut->dumpConfiguration();

    LabeledMetricParameters expected = {};
    expected.at_label<ts::Id>() = params.id();
    expected.at_label<ts::OperationType>() = params.operationType();
    expected.at_label<ts::CollectionTimeScope>() = params.collectionTimeScope();
    expected.at_label<ts::CollectionDuration>() = params.collectionDuration();
    expected.at_label<ts::SensorPath>() = {
        LabeledSensorParameters("service1", "path1", "metadata1")};

    EXPECT_THAT(conf, Eq(expected));
}

TEST_F(TestMetricAfterInitialization, notifiesRegisteredListeners)
{
    EXPECT_CALL(listenerMock, sensorUpdated(Ref(*sensorMocks.front()),
                                            Milliseconds{18}, 31.2));

    sut->registerForUpdates(listenerMock);
    sut->sensorUpdated(*sensorMocks.front(), Milliseconds{18}, 31.2);
}

TEST_F(TestMetricAfterInitialization,
       doesntNotifyRegisteredListenersWhenValueDoesntChange)
{
    EXPECT_CALL(listenerMock, sensorUpdated(Ref(*sensorMocks.front()),
                                            Milliseconds{18}, 31.2));

    sut->registerForUpdates(listenerMock);
    sut->sensorUpdated(*sensorMocks.front(), Milliseconds{18}, 31.2);
    sut->sensorUpdated(*sensorMocks.front(), Milliseconds{70}, 31.2);
}

TEST_F(TestMetricAfterInitialization, doesntNotifyAfterUnRegisterListener)
{
    EXPECT_CALL(listenerMock, sensorUpdated(_, _, _)).Times(0);

    sut->registerForUpdates(listenerMock);
    sut->unregisterFromUpdates(listenerMock);
    sut->sensorUpdated(*sensorMocks.front(), Milliseconds{18}, 31.2);
}

class TestMetricCalculationFunctions :
    public TestMetric,
    public WithParamInterface<MetricParams>
{
  public:
    void SetUp() override
    {
        sut = makeSut(params.operationType(GetParam().operationType())
                          .collectionTimeScope(GetParam().collectionTimeScope())
                          .collectionDuration(GetParam().collectionDuration()));
    }

    static std::vector<std::pair<Milliseconds, double>> defaultReadings()
    {
        std::vector<std::pair<Milliseconds, double>> ret;
        ret.emplace_back(0ms, std::numeric_limits<double>::quiet_NaN());
        ret.emplace_back(10ms, 14.);
        ret.emplace_back(1ms, 3.);
        ret.emplace_back(5ms, 7.);
        return ret;
    }
};

MetricParams defaultCollectionFunctionParams()
{
    return MetricParams()
        .readings(TestMetricCalculationFunctions::defaultReadings())
        .expectedReading(systemTimestamp + 16ms, 7.0);
}

MetricParams defaultPointParams()
{
    return defaultCollectionFunctionParams().collectionTimeScope(
        CollectionTimeScope::point);
}

INSTANTIATE_TEST_SUITE_P(
    TimeScopePointReturnsLastReading, TestMetricCalculationFunctions,
    Values(defaultPointParams().operationType(OperationType::min),
           defaultPointParams().operationType(OperationType::max),
           defaultPointParams().operationType(OperationType::sum),
           defaultPointParams().operationType(OperationType::avg)));

MetricParams defaultMinParams()
{
    return defaultCollectionFunctionParams().operationType(OperationType::min);
}

INSTANTIATE_TEST_SUITE_P(
    ReturnsMinForGivenTimeScope, TestMetricCalculationFunctions,
    Values(defaultMinParams()
               .collectionTimeScope(CollectionTimeScope::interval)
               .collectionDuration(CollectionDuration(100ms))
               .expectedReading(systemTimestamp + 16ms, 3.0),
           defaultMinParams()
               .collectionTimeScope(CollectionTimeScope::interval)
               .collectionDuration(CollectionDuration(3ms))
               .expectedReading(systemTimestamp + 16ms, 7.0),
           defaultMinParams()
               .collectionTimeScope(CollectionTimeScope::startup)
               .expectedReading(systemTimestamp + 16ms, 3.0)));

MetricParams defaultMaxParams()
{
    return defaultCollectionFunctionParams().operationType(OperationType::max);
}

INSTANTIATE_TEST_SUITE_P(
    ReturnsMaxForGivenTimeScope, TestMetricCalculationFunctions,
    Values(defaultMaxParams()
               .collectionTimeScope(CollectionTimeScope::interval)
               .collectionDuration(CollectionDuration(100ms))
               .expectedReading(systemTimestamp + 16ms, 14.0),
           defaultMaxParams()
               .collectionTimeScope(CollectionTimeScope::interval)
               .collectionDuration(CollectionDuration(6ms))
               .expectedReading(systemTimestamp + 16ms, 14.0),
           defaultMaxParams()
               .collectionTimeScope(CollectionTimeScope::interval)
               .collectionDuration(CollectionDuration(5ms))
               .expectedReading(systemTimestamp + 16ms, 7.0),
           defaultMaxParams()
               .collectionTimeScope(CollectionTimeScope::startup)
               .expectedReading(systemTimestamp + 16ms, 14.0)));

MetricParams defaultSumParams()
{
    return defaultCollectionFunctionParams().operationType(OperationType::sum);
}

INSTANTIATE_TEST_SUITE_P(
    ReturnsSumForGivenTimeScope, TestMetricCalculationFunctions,
    Values(defaultSumParams()
               .collectionTimeScope(CollectionTimeScope::interval)
               .collectionDuration(CollectionDuration(100ms))
               .expectedReading(systemTimestamp + 16ms,
                                14. * 0.01 + 3. * 0.001 + 7. * 0.005),
           defaultSumParams()
               .collectionTimeScope(CollectionTimeScope::interval)
               .collectionDuration(CollectionDuration(8ms))
               .expectedReading(systemTimestamp + 16ms,
                                14. * 0.002 + 3. * 0.001 + 7 * 0.005),
           defaultSumParams()
               .collectionTimeScope(CollectionTimeScope::interval)
               .collectionDuration(CollectionDuration(6ms))
               .expectedReading(systemTimestamp + 16ms, 3. * 0.001 + 7 * 0.005),
           defaultSumParams()
               .collectionTimeScope(CollectionTimeScope::startup)
               .expectedReading(systemTimestamp + 16ms,
                                14. * 0.01 + 3. * 0.001 + 7 * 0.005)));

MetricParams defaultAvgParams()
{
    return defaultCollectionFunctionParams().operationType(OperationType::avg);
}

INSTANTIATE_TEST_SUITE_P(
    ReturnsAvgForGivenTimeScope, TestMetricCalculationFunctions,
    Values(defaultAvgParams()
               .collectionTimeScope(CollectionTimeScope::interval)
               .collectionDuration(CollectionDuration(100ms))
               .expectedReading(systemTimestamp + 16ms,
                                (14. * 10 + 3. * 1 + 7 * 5) / 16.),
           defaultAvgParams()
               .collectionTimeScope(CollectionTimeScope::interval)
               .collectionDuration(CollectionDuration(8ms))
               .expectedReading(systemTimestamp + 16ms,
                                (14. * 2 + 3. * 1 + 7 * 5) / 8.),
           defaultAvgParams()
               .collectionTimeScope(CollectionTimeScope::interval)
               .collectionDuration(CollectionDuration(6ms))
               .expectedReading(systemTimestamp + 16ms, (3. * 1 + 7 * 5) / 6.),
           defaultAvgParams()
               .collectionTimeScope(CollectionTimeScope::startup)
               .expectedReading(systemTimestamp + 16ms,
                                (14. * 10 + 3. * 1 + 7 * 5) / 16.)));

TEST_P(TestMetricCalculationFunctions, calculatesReadingValue)
{
    for (auto [timestamp, reading] : GetParam().readings())
    {
        sut->sensorUpdated(*sensorMocks.front(), clockFake.steadyTimestamp(),
                           reading);
        clockFake.advance(timestamp);
    }

    const auto [expectedTimestamp, expectedReading] =
        GetParam().expectedReading();
    const auto readings = sut->getReadings();

    EXPECT_THAT(readings,
                ElementsAre(MetricValue{"id", "metadata", expectedReading,
                                        expectedTimestamp.count()}));
}

TEST_P(TestMetricCalculationFunctions,
       calculatedReadingValueWithIntermediateCalculations)
{
    for (auto [timestamp, reading] : GetParam().readings())
    {
        sut->sensorUpdated(*sensorMocks.front(), clockFake.steadyTimestamp(),
                           reading);
        clockFake.advance(timestamp);
        sut->getReadings();
    }

    const auto [expectedTimestamp, expectedReading] =
        GetParam().expectedReading();
    const auto readings = sut->getReadings();

    EXPECT_THAT(readings,
                ElementsAre(MetricValue{"id", "metadata", expectedReading,
                                        expectedTimestamp.count()}));
}
